#include <WiFi.h>
#include "ImageData.h"

const char *ssid = "apeiron";
const char *pwrd = "enantiodromia..";
WiFiClient client;
IPAddress serverIP(192, 168, 1, 1);

void setup()
{
    Serial.begin(115200);

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

    if(client.connect(serverIP, 80)){
        Serial.println("Connected to server");

        client.write(travisGoblins_b, 48000);
        Serial.println("Data Sent!");

        client.stop();
        Serial.println("Connection Closed!");
    }
    else{
        Serial.println("Failed to connect to server!");
    }
}

void loop()
{
}