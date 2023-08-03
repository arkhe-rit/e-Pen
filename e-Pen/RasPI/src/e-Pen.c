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

#define REDIS_DEFAULT_IP "127.0.0.1";
#define REDIS_DEFAULT_PORT 6379;

#define REDIS_SUB_CMD "SUBSCRIBE projector/eink"

const char* redisIP = "127.0.0.1";
int redisPort = 6379;

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    printf("Received message!\r\n");
    redisReply *r = reply;
    if (reply == NULL) {
        printf("Message is NULL :(\r\n");
        return;
    }

    printf("Message is not NULL :)\r\n");
    if (r->type == REDIS_REPLY_STRING){
        printf("String received: %s\r\n", r->str);
    }

    if (r->type == REDIS_REPLY_ARRAY) {
        for (int j = 0; j < r->elements; j++) {
            printf("%u) %s\n", j, r->element[j]->str);
        }
    }
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

int main(int argc, char* argv[])
{
    // UNIX moment
    if(geteuid())
    {
        printf("ERROR: Please run this program as root.\r\n");
        return 1;
    }

    /*
    int bufsize = 16;
    char* redisIP = malloc(bufsize * sizeof(char));
    int redisPort;

    // Handle commandline args
    if(argc > 2){
        for(unsigned int i = 0; i < argc; i++){
            switch()
        }
    }
    else{
        redisIP = REDIS_DEFAULT_IP;
        redisPort = REDIS_DEFAULT_PORT;
    }

    printf("Redis IP: %s\r\n", redisIP);
    int i = 15553; // I'm just checking for an overflow
    i++;
    printf("Redis Port: %s\r\n", redisPort);
    sleep(10); // DEBUG for sure
    */

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

    // Create connect to redis (thanks to https://github.com/redis/hiredis/issues/55#issuecomment-4269731)
    printf("Attempting to connect to Redis...\r\n");
    struct event_base *base = event_base_new();

    redisAsyncContext *c = redisAsyncConnect(redisIP, redisPort);
    if (c->err){
        printf("Error: %s\n", c->errstr);
        redisAsyncFree(c);
        c = NULL;
        return 1;
    }
    printf("Success!\r\n");
    if(redisLibeventAttach(c, base) == REDIS_OK){
        printf("Attached to libevent!\r\n");
    }
    else{
        printf("ERROR: Unable to attach libevent!\r\n");
        return 1;
    }
    redisAsyncCommand(c, onMessage, NULL, REDIS_SUB_CMD);
    printf("Ran ");
    printf(REDIS_SUB_CMD);
    printf("\r\n");
    while(1){ // DEBUG
    event_base_dispatch(base);
    }

    printf("Initializing exit sequence...\r\n"); // DEBUG
    exitDisplay(blackImg, redImg);

    printf("Disconnecting from Redis...\r\n");
    redisAsyncFree(c);

    return 0;
}