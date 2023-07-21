/*
 * Written by Jackson Majewski for Arkhe
 * Some server code shamelessly """borrowed""" from Rui Santos (http://randomnerdtutorials.com)
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
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 10000; //ms
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

  Serial.println("Allocating image buffer...");
  imgBuffer = new unsigned char[ARR_SIZE]{0xFF};

  Serial.println("e-Paper Init and Clear...");
  EPD_7IN5B_V2_Init();
  EPD_7IN5B_V2_Clear();
  DEV_Delay_ms(500);  

  // show splash screen
  Serial.println("Displaying splash screen...");
  EPD_7IN5B_V2_Display(arkheLogo_b, arkheLogo_r);
  Serial.println("Clear...");
  EPD_7IN5B_V2_Clear();
  EPD_7IN5B_V2_Sleep();

  // Attempt to connect to WiFi
  Serial.print("Attempting to connect to \"");
  Serial.print(ssid);
  Serial.println("\"...");

  WiFi.begin(ssid, pwrd);
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, HIGH); // Flashes light while awaiting connection (in theory)
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Show WiFi address on screen
  EPD_7IN5B_V2_Init();
  Paint_NewImage(imgBuffer, EPD_7IN5B_V2_WIDTH, EPD_7IN5B_V2_HEIGHT, 0, WHITE);
  Paint_SelectImage(imgBuffer);
  Paint_Clear(WHITE);
  Paint_DrawString_EN(10, 0, WiFi.localIP().toString().c_str(), &Font24, WHITE, BLACK);
  EPD_7IN5B_V2_Display(imgBuffer, whiteBuffer);
  Paint_Clear(WHITE);
  EPD_7IN5B_V2_Sleep();

  server.begin();
}

/* The main loop -------------------------------------------------------------*/
void loop()
{
  // Listen for clients
  WiFiClient client = server.available();

  // On new client connection
  if (client)
  { 
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
        digitalWrite(LED_BUILTIN, HIGH); // Turn on LED while client is transmitting data
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

    if (index == ARR_SIZE)
    {
      if(shouldShutDown == true)
      {
        exit();
      }
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
  Serial.println("Preparing for shutdown...");
  EPD_7IN5B_V2_Init();

  Serial.println("Clearing display...");
  EPD_7IN5B_V2_Clear();

  Serial.println("Putting display to Sleep...");
  EPD_7IN5B_V2_Sleep();

  Serial.println("Deleting imgBuffer...");
  delete[] imgBuffer;
  imgBuffer = NULL;
  Serial.println("Goodbye!");

  esp_deep_sleep_start();
}
