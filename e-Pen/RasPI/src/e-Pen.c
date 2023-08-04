#include "EPD_7in5b_V2.h"
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define WHITE 0xFF
#define BLACK 0x00

#define REDIS_DEFAULT_IP "172.20.10.3"
#define REDIS_DEFAULT_PORT 6379

#define REDIS_SUB_CMD "SUBSCRIBE channel projector/eink"

enum messageProperty = {type, command, value};

void onMessage(const redisAsyncContext *c, void *reply, void *privdata)
{
    redisReply *r = reply;
    if (reply == NULL)
    {
        return;
    }
    
    if (r->type == REDIS_REPLY_ARRAY)
    {
        printf("Array Received: \r\n")
        for (int j = 0; j < r->elements; j++)
        {
            printf("%u) %s\n", j, r->element[j]->str);
        }

    }
}

int onConnect(const redisAsyncContext *c, int status)
{
    if(status != REDIS_OK){
        printf("ERROR: %s\r\n", c->errstr);
        return 1;
    }
    printf("Connected successfully!\r\n");
    return 0;
}

void onDisconnect(const redisAsyncContext *c, int status)
{
    if(status != REDIS_OK){
        printf("ERROR: %s\r\n", c->errstr);
        return;
    }
    printf("Disconnected successfully!\r\n");
}

void loadAndDrawBMP(UBYTE *blackImgCache, const char *blackBMPPath, UBYTE *redImgCache, const char *redBMPPath)
{
    printf("Init.\r\n");
    EPD_7IN5B_V2_Init();

    printf("Reading black BMP...\r\n");
    Paint_SelectImage(blackImgCache);
    Paint_Clear(WHITE);
    GUI_ReadBmp(blackBMPPath, 0, 0);

    printf("Reading red BMP...\r\n");
    Paint_SelectImage(redImgCache);
    Paint_Clear(WHITE);
    GUI_ReadBmp(redBMPPath, 0, 0);

    printf("Displaying...\r\n");
    EPD_7IN5B_V2_Display(blackImgCache, redImgCache);

    printf("Going to sleep...\r\n");
    EPD_7IN5B_V2_Sleep();
}

// Clears screen and safely shutd down the EPD
void exitDisplay(UBYTE *blackImage, UBYTE *redImage)
{
    printf("Wake display...\r\n");
    EPD_7IN5B_V2_Init();

    printf("Clearing screen...\r\n");
    EPD_7IN5B_V2_Clear();

    printf("Going to sleep...\r\n");
    EPD_7IN5B_V2_Sleep();

    printf("Clearing images from memory...\r\n");
    free(blackImage);
    blackImage = NULL;
    free(redImage);
    redImage = NULL;
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

    // Stealing so much from Waveshare's test file
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN5B_V2_Init();

    // Create new image cache & init memory
    UBYTE *blackImg, *redImg;
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
    
    EPD_7IN5B_V2_Sleep(); // This is all an evil trick to ensure the e-Paper is in sleep mode for the loop >:)

    // Create connection to Redis
    printf("Attempting to connect to Redis...\r\n");
    const struct event_base *base = event_base_new();

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
    exitDisplay(blackImg, redImg);

    return 0;
}