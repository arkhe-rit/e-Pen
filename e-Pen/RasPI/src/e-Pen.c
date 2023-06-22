#include "EPD_7in5b_V2.h"
#include <stdbool.h>

#define WHITE 0xFF
#define BLACK 0x00

int loadAndDrawBMP(UBYTE *blackImgCache, const char *blackBMPPath, UBYTE *redImgCache, const char *redBMPPath)
{
    EPD_7IN5B_V2_Init();

    Paint_SelectImage(blackImgCache);
    Paint_Clear(WHITE);
    GUI_ReadBmp(blackBMPPath, 0, 0);

    Paint_SelectImage(redImgCache);
    Paint_Clear(WHITE);
    GUI_ReadBmp(redBMPPath, 0, 0);

    EPD_7IN5B_V2_Display(blackImgCache, redImgCache);

    EPD_7IN5B_V2_Sleep();
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

    // Display Arkhe logo
    Paint_SelectImage(blackImg);
    Paint_Clear(WHITE);
    GUI_ReadBmp("./imgs/ArkheLogo_b.bmp");

    Paint_SelectImage(redImg);
    Paint_Clear(WHITE);
    GUI_ReadBmp("./imgs/ArkheLogo_r.bmp");

    EPD_7IN5B_V2_Display(blackImg, redImg);
    EPD_7IN5B_V2_Sleep(); // This is all an evil trick to ensure the e-Paper is in sleep mode for the loop >:)

    bool isRunning = true;
    while (isRunning = true)
    {
        printf("Enter selection (1, 2, q): ");

        char input;
        fgets(input, 1, stdin);

        switch (input)
        {
        case 1:
            loadAndDrawBMP(blackImg, "./imgs/TravisGoblins_b.bmp", redImg, "./imgs/TravisGoblins_r.bmp");
            break;

        case 2:
            loadAndDrawBMP(blackImg, "./img/HeWillDie_b.bmp", redImg, "./img/HeWillDie_r.bmp");
            break;

        default:
            isRunning = false;
        }
    }

    /* TODO:
     *  -Make blackImg and redImg vaguely global so i can stop passing them to every function
     */

    exit(blackImg, redImg);
}
