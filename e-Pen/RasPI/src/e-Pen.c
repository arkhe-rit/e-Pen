#include "EPD_7in5b_V2.h"
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <json.h>

#define WHITE 0xFF
#define BLACK 0x00

#define REDIS_DEFAULT_IP "192.168.2.10"
#define REDIS_DEFAULT_PORT 6379

#define REDIS_SUB_CMD "SUBSCRIBE channel projector/eink"

#define NUMBER_OF_IMAGES 3
// #define MAX_IMG_PATH_LENGTH 20
const char *imgs[NUMBER_OF_IMAGES] = {
    "./imgs/Beach_b.bmp",
    "./imgs/HeWillDie_b.bmp",
    "./imgs/TravisGoblins_b.bmp"};

UBYTE *blackImg, *redImg;      // Image buffers
const struct event_base *base; // event base

// Callback for received message
void onMessage(const redisAsyncContext *c, void *reply, void *privdata)
{
    redisReply *r = reply;
    if (reply == NULL)
    {
        return;
    }

    if (r->type == REDIS_REPLY_ARRAY)
    {
        printf("Array Received: \r\n");
        for (int j = 0; j < r->elements; j++)
        {
            printf("%u) %s\n", j, r->element[j]->str);
        }

        if (r->element[2]->str == NULL)
        {
            return;
        }

        printf("Attempting to parse\r\n");
        struct json_object *jobj;
        jobj = json_tokener_parse(r->element[2]->str);
        printf("Parsed Object:\n======\n%s\n======\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));

        struct json_object *tmp;

        // Parse type
        if (!json_object_object_get_ex(jobj, "type", &tmp))
            return;
        char *msgType[json_object_get_string_len(tmp)];
        memset(msgType, 0, sizeof(char) * json_object_get_string_len(tmp));
        strcpy(msgType, json_object_get_string(tmp));
        printf("Type: %s\r\n", msgType);

        // Parse command
        if (!json_object_object_get_ex(jobj, "command", &tmp))
            return;
        char *msgCmd[json_object_get_string_len(tmp)];
        memset(msgCmd, 0, sizeof(char) * json_object_get_string_len(tmp));
        strcpy(msgCmd, json_object_get_string(tmp));
        printf("Cmd: %s\r\n", msgCmd);

        if (!strcmp(msgCmd, "shutdown"))
        {
            event_base_loopbreak(base);
            return;
        }

        // Parse value
        if (!json_object_object_get_ex(jobj, "value", &tmp))
            return;
        int msgValue = json_object_get_int(tmp) % NUMBER_OF_IMAGES;
        if (errno == EINVAL)
            return;
        printf("Value: %i\r\n", msgValue);

        if (!strcmp(msgCmd, "change-image"))
            loadAndDrawBMP(imgs[msgValue]);
    }
}

// Callback for redis connect
int onConnect(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        printf("ERROR: %s\r\n", c->errstr);
        return 1;
    }
    printf("Connected successfully!\r\n");
    return 0;
}

// Callback for redis disconnect
void onDisconnect(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        printf("ERROR: %s\r\n", c->errstr);
        return;
    }
    printf("Disconnected successfully!\r\n");
}

// Loads BMP into black buffer and displays
void loadAndDrawBMP(const char *blackBMPPath)
{
    printf("Init.\r\n");
    EPD_7IN5B_V2_Init();

    printf("Reading black BMP...\r\n");
    Paint_SelectImage(blackImg);
    Paint_Clear(WHITE);
    GUI_ReadBmp(blackBMPPath, 0, 0);

    printf("Reading red BMP...\r\n");
    Paint_SelectImage(redImg);
    Paint_Clear(WHITE);

    printf("Displaying...\r\n");
    EPD_7IN5B_V2_Display(blackImg, redImg);

    printf("Putting display to sleep...\r\n");
    EPD_7IN5B_V2_Sleep();

    printf("Ready!\r\n\n");
}

// Clears screen and safely shutdown the EPD
void exitDisplay()
{
    printf("Wake display...\r\n");
    EPD_7IN5B_V2_Init();

    printf("Clearing screen...\r\n");
    EPD_7IN5B_V2_Clear();

    printf("Putting display to sleep...\r\n");
    EPD_7IN5B_V2_Sleep();

    printf("Clearing images from memory...\r\n");
    free(blackImg);
    blackImg = NULL;
    free(redImg);
    redImg = NULL;
    DEV_Delay_ms(2500); // Important apparently. Upped time cause 2 seemed iffy. YMMV

    printf("Closing power line...\r\n");
    DEV_Module_Exit();
}

int main(int argc, char *argv[])
{
    // UNIX moment
    signal(SIGPIPE, SIG_IGN);
    if (geteuid())
    {
        printf("ERROR: Please run this program as root.\r\n");
        return 1;
    }

    printf("Arkhe e-Pen for Waveshare 7.5\" (B)\r\n");

    // Init. device & pins
    if (DEV_Module_Init() != 0)
    {
        return -1;
    }

    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN5B_V2_Init();

    // Init image memory
    UWORD imageSize = ((EPD_7IN5B_V2_WIDTH % 8 == 0) ? (EPD_7IN5B_V2_WIDTH / 8) : (EPD_7IN5B_V2_WIDTH / 8 + 1)) * EPD_7IN5B_V2_HEIGHT;
    if ((blackImg = (UBYTE *)malloc(imageSize)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    if ((redImg = (UBYTE *)malloc(imageSize)) == NULL)
    {
        printf("Failed to apply for red memory...\r\n");
        return -1;
    }

    // Creating image buffers
    printf("Creating image buffers...\r\n");
    Paint_NewImage(blackImg, EPD_7IN5B_V2_WIDTH, EPD_7IN5B_V2_HEIGHT, 0, WHITE);
    Paint_NewImage(redImg, EPD_7IN5B_V2_WIDTH, EPD_7IN5B_V2_HEIGHT, 0, WHITE);

    // Display Arkhe logo
    Paint_SelectImage(blackImg);
    Paint_Clear(WHITE);
    GUI_ReadBmp("./imgs/ArkheLogo_b.bmp", 0, 0);

    Paint_SelectImage(redImg);
    Paint_Clear(WHITE);
    GUI_ReadBmp("./imgs/ArkheLogo_r.bmp", 0, 0);

    EPD_7IN5B_V2_Display(blackImg, redImg);

    EPD_7IN5B_V2_Sleep();

    // Create connection to Redis
    printf("Attempting to connect to Redis...\r\n");
    base = event_base_new();

    redisAsyncContext *c = redisAsyncConnect(REDIS_DEFAULT_IP, REDIS_DEFAULT_PORT);
    if (c->err)
    {
        printf("Error: %s\n", c->errstr);
        // DC & quit
        redisAsyncFree(c);
        c = NULL;
        exitDisplay(blackImg, redImg);
        return 1;
    }

    redisLibeventAttach(c, base);
    printf("Attached to event!\r\n");

    redisAsyncSetConnectCallback(c, onConnect);
    redisAsyncSetDisconnectCallback(c, onDisconnect);
    printf("Set callbacks!\r\n");

    redisAsyncCommand(c, onMessage, NULL, REDIS_SUB_CMD);
    event_base_dispatch(base);

    printf("Disconnecting from Redis...\r\n");
    redisAsyncFree(c);

    printf("Beginning to exit...\r\n");
    exitDisplay();

    printf("Shutting down...\r\n");
    execlp("poweroff", "poweroff", (char *)NULL);

    return 0;
}