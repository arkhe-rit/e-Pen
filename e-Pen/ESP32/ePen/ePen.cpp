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
String request;
UBYTE *BlackImage, *RYImage;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void parseRequest();
void Exit();

/* Entry point ----------------------------------------------------------------*/
void setup() {
  Serial.begin(115200);

  // EPD Init. and display splash screen
  printf("EPD_7IN5B_V2_test Demo\r\n");
  DEV_Module_Init();

  printf("e-Paper Init and Clear...\r\n");
  EPD_7IN5B_V2_Init();
  EPD_7IN5B_V2_Clear();
  DEV_Delay_ms(500);

  // Create a new image cache named IMAGE_BW and fill it with white
  UWORD Imagesize = ((EPD_7IN5B_V2_WIDTH % 8 == 0) ? (EPD_7IN5B_V2_WIDTH / 8) : (EPD_7IN5B_V2_WIDTH / 8 + 1)) * EPD_7IN5B_V2_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    printf("Failed to apply for black memory...\r\n");
    while (1)
      ;
  }
  if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL) {
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
  while (WiFi.status() != WL_CONNECTED) {
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
void loop() {
  // Listen for clients
  WiFiClient client = server.available();

  if (client) 
  {  // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    printf("New Client.\r\n");                                                 // print a message out in the serial port
    String currentLine = "";                                                   // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) 
    {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) 
      {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out the serial monitor
        request += c;
        if (c == '\n') 
        {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) 
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
          }
        }
      }
      // Handle request
      
    }
    // Handle client disconnect
    request = "";
    client.stop();
    printf("Client disconnected.\r\n");
  }
  // Want to specify endpoint path
  // 

  //Exit();
}

/* Exit func ------------------------------------------------------------------*/
void Exit() {
  printf("Clear...\r\n");
  EPD_7IN5B_V2_Clear();

  printf("Goto Sleep...\r\n");
  EPD_7IN5B_V2_Sleep();
  free(BlackImage);
  free(RYImage);
  BlackImage = NULL;
  RYImage = NULL;
}
