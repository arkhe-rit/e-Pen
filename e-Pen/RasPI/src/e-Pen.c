#include "EPD_7in5b_V2.h"

#define WHITE   0xFF
#define BLACK   0x00

int loadFromBMP(UBYTE *imgCache, const char *bmpPath)
{
    Paint_SelectImage(imgCache);
    Paint_Clear(WHITE);
    GUI_ReadBmp(bmpPath, 0, 0);
}

// Clears screen and safely exits the program
int exit(UBYTE *blackImage, UBYTE *redImage)
{
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

    return 0;
}

int main()
{
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
    printf("Creating image buffers...");
    Paint_NewImage(blackImg, EPD_7IN5B_V2_WIDTH, EPD_7IN5B_V2_HEIGHT, 0, WHITE);
    Paint_NewImage(redImg, EPD_7IN5B_V2_WIDTH, EPD_7IN5B_V2_HEIGHT, 0, WHITE);

    loadFromBMP(blackImg, "./imgs/Travis.bmp");
    loadFromBMP(redImg, "./imgs/Goblins.bmp");
    EPD_7IN5B_V2_Display(blackImg, redImg);
    
    printf("Press enter...\r\n");
    scanf("%s");

    exit(blackImg, redImg);

}