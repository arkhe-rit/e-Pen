#include "EPD_7in5b_V2.h"

#define WHITE 0xFF
#define BLACK 0x00

int loadAndDrawBMP(UBYTE *blackImgCache, const char *blackBMPPath, UBYTE *redImgCache, const char *redBMPPath)
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

    int isRunning = 1;
    while (isRunning = 1)
    {
        char input;
        printf("Enter selection (1, 2, 3, or q): ");
        input = getchar();
        printf("\r\n");

        switch (input)
        {
        case '1':
            loadAndDrawBMP(blackImg, "./imgs/TravisGoblins_b.bmp", redImg, "./imgs/TravisGoblins_r.bmp");
            break;

        case '2':
            loadAndDrawBMP(blackImg, "./imgs/HeWillDie_b.bmp", redImg, "./imgs/HeWillDie_r.bmp");
            break;

        case '3':
            loadAndDrawBMP(blackImg, "./imgs/Beach_b.bmp", redImg, "./imgs/Beach_r.bmp");
            break;

        case 'q':
            EPD_7IN5B_V2_Init();
            return exit(blackImg, redImg);

        default:
            continue;
        }

        fflush(stdin); // I AM GOING TO KILL MYSELF
    }

    /* TODO:
     *  -Make blackImg and redImg vaguely global so i can stop passing them to every function
     */

    exit(blackImg, redImg);
}
