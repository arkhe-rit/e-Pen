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

#define ARR_SIZE 48000
#define LED_BUILTIN 2

/* Globals --------------------------------------------------------------------*/
const char *ssid = "apeiron";
const char *pwrd = "enantiodromia..";
WiFiServer server(80);
UBYTE *BlackImage, *RYImage;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 10000;
unsigned char* imgBuffer;
bool shouldShutDown = false;

void Exit();

/* Entry point ----------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);

  // Enable onboard LED
  pinMode(LED_BUILTIN, OUTPUT);

  // EPD Init. and display splash screen
  Serial.println("====================================");
  Serial.println("EPD_7IN5B_V2 Server v0.1");
  Serial.println("====================================");
  DEV_Module_Init();

  Serial.println("Allocating on heap...");
  imgBuffer = new unsigned char[ARR_SIZE]{0xFF};

  Serial.println("e-Paper Init and Clear...");
  EPD_7IN5B_V2_Init();
  EPD_7IN5B_V2_Clear();
  DEV_Delay_ms(500);

  // Create a new image cache named IMAGE_BW and fill it with white
  UWORD Imagesize = ((EPD_7IN5B_V2_WIDTH % 8 == 0) ? (EPD_7IN5B_V2_WIDTH / 8) : (EPD_7IN5B_V2_WIDTH / 8 + 1)) * EPD_7IN5B_V2_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
  {
    Serial.println("Failed to apply for black memory...");
    while (1)
      ;
  }
  if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL)
  {
    Serial.println("Failed to apply for red memory...");
    while (1)
      ;
  }

  // Select Image
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_SelectImage(RYImage);
  Paint_Clear(WHITE);

  // show splash screen
  Serial.println("Displaying splash screen...");
  EPD_7IN5B_V2_Display(arkheLogo_b, arkheLogo_r);
  Serial.println("Clear...");
  EPD_7IN5B_V2_Clear();
  EPD_7IN5B_V2_Sleep();

  // Attempt to connect to WiFi
  Serial.print("Attempting to connect to \"");
  Serial.print(ssid);
  Serial.println("...");

  WiFi.begin(ssid, pwrd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
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
    Serial.println("New Client.");

    shouldShutDown = true;

    unsigned int index = 0;
    while (client.connected() && currentTime - previousTime <= timeoutTime) // loop while the client's connected
    { 
      currentTime = millis();
      if (client.available())
      {
        digitalWrite(LED_BUILTIN, HIGH); // Turn on LED when client connects
        imgBuffer[index] = client.read();

        // Check if pure white
        if(shouldShutDown == true && imgBuffer[index] != 0xFF)
        {
          shouldShutDown = false;
        }
        index++;
      }
    }

    // Handle client disconnect
    client.stop();
    Serial.println("Client disconnected.");
    digitalWrite(LED_BUILTIN, LOW);


    if(shouldShutDown == true)
    {
      exit();
    }
    if (index == ARR_SIZE)
    {
      //Serial.println("Size match!");
      Serial.println("Drawing received data!");
      EPD_7IN5B_V2_Init();
      EPD_7IN5B_V2_Clear();
      EPD_7IN5B_V2_Display(imgBuffer, whiteBuffer);
      EPD_7IN5B_V2_Sleep();
      Serial.println("Drawing complete!");
    }
  }
}

/* Exit func ------------------------------------------------------------------*/
void exit()
{
  Serial.println("Exiting...");

  Serial.println("Init...");
  EPD_7IN5B_V2_Init();

  Serial.println("Clear...");
  EPD_7IN5B_V2_Clear();

  Serial.println("Goto Sleep...");
  EPD_7IN5B_V2_Sleep();
  free(BlackImage);
  free(RYImage);
  BlackImage = NULL;
  RYImage = NULL;

  Serial.println("Deleting imgBuffer...");
  delete[] imgBuffer;
  Serial.println("Goodbye!");

  esp_deep_sleep_start();
}
