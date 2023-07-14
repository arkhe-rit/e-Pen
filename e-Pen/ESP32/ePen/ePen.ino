/*
 * Written by Jackson Majewski for Arkhe
 * Lots of server code shamelessly """borrowed""" from Rui Santos (http://randomnerdtutorials.com)
 */

/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include <stdlib.h>
#include <WiFi.h>

/* Globals --------------------------------------------------------------------*/
const char *ssid = "apeiron";
const char *pwrd = "enantiodromia..";
WiFiServer server(80);
UBYTE *BlackImage, *RYImage;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;
unsigned char* imgBuffer;

void Exit();

/* Entry point ----------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);

  // EPD Init. and display splash screen
  printf("EPD_7IN5B_V2_test Demo (NEW)\r\n");
  DEV_Module_Init();

  printf("Allocating on heap...\r\n");
  imgBuffer = new unsigned char[48000]{0xFF};

  printf("e-Paper Init and Clear...\r\n");
  EPD_7IN5B_V2_Init();
  EPD_7IN5B_V2_Clear();
  DEV_Delay_ms(500);

  // Create a new image cache named IMAGE_BW and fill it with white
  UWORD Imagesize = ((EPD_7IN5B_V2_WIDTH % 8 == 0) ? (EPD_7IN5B_V2_WIDTH / 8) : (EPD_7IN5B_V2_WIDTH / 8 + 1)) * EPD_7IN5B_V2_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
  {
    printf("Failed to apply for black memory...\r\n");
    while (1)
      ;
  }
  if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL)
  {
    printf("Failed to apply for red memory...\r\n");
    while (1)
      ;
  }

  // Select Image
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_SelectImage(RYImage);
  Paint_Clear(WHITE);

  // show splash screen
  printf("Displaying splash screen...\r\n");
  EPD_7IN5B_V2_Display(arkheLogo_b, arkheLogo_r);
  Serial.println("Clear...");
  EPD_7IN5B_V2_Clear();
  EPD_7IN5B_V2_Sleep();

  // Attempt to connect to WiFi
  printf("Attempting to connect to \"%s\"...\r\n", ssid);
  WiFi.begin(ssid, pwrd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    printf(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

/* The main loop -------------------------------------------------------------*/
void loop()
{
  // Listen for clients
  WiFiClient client = server.available();

  if (client)
  { // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    printf("New Client.\r\n");

    unsigned int index = 0;
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (client.available())
      {
        //byte dataReceived[1024];
        //int bytesRead = client.read(dataReceived, sizeof(dataReceived)); // read a byte
          imgBuffer[index] = client.read();
          index++;
      }
    }
    // Handle client disconnect
    client.stop();
    printf("Client disconnected.\r\n");

    //if (index == (sizeof(imgBuffer) / sizeof(char) - 1))
    {
      //Serial.println("Size match!");
      Serial.println("Drawing received data!");
      EPD_7IN5B_V2_Init();
      EPD_7IN5B_V2_Clear();
      EPD_7IN5B_V2_Display(imgBuffer, whiteBuffer);
      EPD_7IN5B_V2_Sleep();
      delay(5000);
    }
    Exit();
  }
}

/* Exit func ------------------------------------------------------------------*/
void Exit()
{
  Serial.println("Exiting...");

  printf("Init...\r\n");
  EPD_7IN5B_V2_Init();

  printf("Clear...\r\n");
  EPD_7IN5B_V2_Clear();

  printf("Goto Sleep...\r\n");
  EPD_7IN5B_V2_Sleep();
  free(BlackImage);
  free(RYImage);
  BlackImage = NULL;
  RYImage = NULL;

  Serial.println("Deleting imgBuffer...");
  delete[] imgBuffer;
  Serial.println("Goodbye!");
}
