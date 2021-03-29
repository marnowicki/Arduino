#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>

#include "../common/wifi-ssid.h"
#include "airly.h"
#include "images.h"

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINT(x) Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif

// struct DATA {
//     float PM1;
//     float PM25;
//     float PM10;
//     float Temp;
// }
static char str[15];

#pragma region display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_DC 0
#define OLED_RESET 2
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         &SPI, OLED_DC, OLED_RESET, -1);

static const String status[] = {"B. dobry", "Dobry", "Umiarkowan", "Dostateczny", "Zly", "Bardzo zly"};
static const uint8_t *images[] = {very_happy_bmp, happy_bmp, normal_bmp, sad_bmp, dead_bmp};
static const float PM10[] = {20, 50, 80, 110, 150, 99999};
static const float PM25[] = {13, 35, 55, 75, 110, 99999};

void initDisplay()
{
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC))
    {
        DEBUG_PRINTLN(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
}

void drawText(String text, int x, int y, int size = 2)
{
    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.print(text);
}

// void drawData(DATA data, int x, int y){
//     String message = "";
//     if(data.PM25 > -9999) message += "PM2.5:" + data.PM25;
//     if(data.PM10 > -9999) message += "PM10 :" + data.PM10;
//     if(data.Temp > -9999) message += "Temp :" + data.Temp;

//     drawText(message, x, y, 1);
// }

void drawBitmap(int x, int y)
{

    display.drawBitmap(x, y,
                       //(display.width()  - TEMP_WIDTH ) / 2,
                       //(d/isplay.height() - TEMP_HEIGHT) / 2,
                       temp_bmp, TEMP_WIDTH, TEMP_HEIGHT, 1);
}
void drawBitmap(const uint8_t *bmp, int x, int y)
{
    display.drawBitmap(x, y, bmp, TEMP_WIDTH, TEMP_HEIGHT, 1);
}

#pragma endregion display

#pragma region wifi

// IPAddress local_IP(192, 168, 1, 90);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);

unsigned long lastTime = 15000;
unsigned long timerDelay = 15000;

void initWIFI()
{
    // Configures static IP address
    // if (!WiFi.config(local_IP, gateway, subnet))
    // {
    //     DEBUG_PRINTLN("STA Failed to configure");
    // }
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    DEBUG_PRINTLN("Connecting");
    String message = "WIFI";
    drawText(message, 0, 16, 2);
    display.display();
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        message += ".";
        drawText(message, 0, 16, 2);
        display.display();
        DEBUG_PRINT(".");
    }
    DEBUG_PRINTLN("");
    DEBUG_PRINT("Connected to WiFi network with IP Address: ");
    DEBUG_PRINTLN(WiFi.localIP());
}

void endWIFI()
{
    WiFi.disconnect(true);
}

#pragma endregion wifi

int getIndex(const float *tab, size_t tabSize, double value)
{
    for (int i = 0; i < tabSize; i++)
    {
        if (tab[i] > value)
            return i;
    }
}

void getData()
{
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    DEBUG_PRINTLN(http.begin(client, URL.c_str()));
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
        DEBUG_PRINT("HTTP Response code: ");
        DEBUG_PRINTLN(httpResponseCode);
        //DEBUG_PRINT("HTTP Size "); DEBUG_PRINTLN(http.getSize());
        //String payload = http.getString();

        //DEBUG_PRINT("Payload length "); DEBUG_PRINTLN(payload.length());
        //DEBUG_PRINT("Payload subtring: "); DEBUG_PRINTLN(payload.substring(0, 50));
        DEBUG_PRINTLN("Headers:");
        for (int i = 0; i < http.headers(); i++)
        {
            DEBUG_PRINT(http.headerName(i));
            DEBUG_PRINT(" : ");
            DEBUG_PRINTLN(http.header(i));
        }
        WiFiClient stream = http.getStream();
        stream.setTimeout(5000);
        display.clearDisplay();
        drawText("Dane", 0, 16, 2);
        display.display();
        DEBUG_PRINT("Stream readString: ");
        String payload = http.getString();
        //String payload = stream.readString();
        DEBUG_PRINTLN(payload.substring(0, 50));

        drawText("Dane.", 0, 16, 2);
        display.display();

        String startTag = "\"current\":";
        String endTag = "\"history\":";
        int start = payload.indexOf(startTag);
        int end = payload.indexOf(endTag);

        DEBUG_PRINT("Start ");
        DEBUG_PRINT(start);
        DEBUG_PRINT(" End ");
        DEBUG_PRINTLN(end);

        //get substring to parse less data (JSON.parse doesn't work with long strings)
        payload = payload.substring(start + startTag.length(), end - 1);

        drawText("Dane..", 0, 16, 2);
        display.display();

        DEBUG_PRINT("Payload length ");
        DEBUG_PRINTLN(payload.length());
        DEBUG_PRINT("Payload subtring head: ");
        DEBUG_PRINTLN(payload.substring(0, 50));
        DEBUG_PRINT("Payload subtring tail: ");
        DEBUG_PRINTLN(payload.substring(payload.length() - 50, payload.length()));

        JSONVar myObject = JSON.parse(payload);
        DEBUG_PRINT("JSON Type ");
        DEBUG_PRINTLN(JSON.typeof(myObject));
        drawText("Dane...", 0, 16, 2);
        display.display();

        if (myObject.hasOwnProperty("fromDateTime"))
        {
            DEBUG_PRINTLN(myObject["fromDateTime"]);
        }
        if (myObject.hasOwnProperty("values"))
        {
            JSONVar values = myObject["values"];
            DEBUG_PRINT("values length: ");
            DEBUG_PRINTLN(values.length());

            DEBUG_PRINTLN(JSON.typeof(values));
            //DATA data = {-9999,-9999,-9999,-9999};
            String message = "";
            String tempMessage = "";
            double pm25Value = 0;
            double pm10Value = 0;
            for (int i = 0; i < values.length(); i++)
            {
                JSONVar o = values[i];
                // if(o["name"] == "PM1") data.PM1 = o["value"];
                // if(o["name"] == "PM25") data.PM25 = o["value"];
                // if(o["name"] == "PM10") data.PM10 = o["value"];
                // if(o["name"] == "TEMPERATURE") data.Temp = o["value"];

                // if (JSON.stringify(o["name"]) == "\"PM1\"")
                //     message += "PM1 :" + JSON.stringify(o["value"]);
                if (JSON.stringify(o["name"]) == "\"PM25\"")
                {
                    dtostrf(o["value"], 4, 0, str);
                    message += "PM2.5:";
                    message += str;
                    pm25Value = (double)o["value"];
                }
                else if (JSON.stringify(o["name"]) == "\"PM10\"")
                {
                    dtostrf(o["value"], 4, 0, str);
                    message += "  PM10:";
                    message += str;
                    pm10Value = (double)o["value"];
                }
                else if (JSON.stringify(o["name"]) == "\"TEMPERATURE\"")
                {
                    dtostrf(o["value"], 4, 1, str);
                    tempMessage += str;
                }

                DEBUG_PRINT("name ");
                DEBUG_PRINT(o["name"]);

                DEBUG_PRINT("   value ");
                DEBUG_PRINTLN(o["value"]);

                //Serial.printf("name: %s  value: %s",o["name"], o["value"]);
            }
            DEBUG_PRINT("Message: ");
            DEBUG_PRINTLN(message);
            display.clearDisplay();
            int PM10Index = getIndex(PM10, sizeof(PM10) / sizeof(PM10[0]), pm10Value);
            int PM25Index = getIndex(PM25, sizeof(PM25) / sizeof(PM25[0]), pm25Value);
            int maxIndex = max(PM10Index, PM25Index);
            const uint8_t *image = images[maxIndex];
            drawBitmap(image, display.width() - TEMP_WIDTH, display.height() - TEMP_HEIGHT);
            drawText(status[maxIndex], 0, 0);
            drawText(message, 0, 16, 1);
            drawBitmap(0, 24);
            drawText(tempMessage, 16, 24, 2);

            display.display();

            //drawData(data, 0, 16);
        }
    }
    else
    {
        DEBUG_PRINT("Error code: ");
        DEBUG_PRINTLN(httpResponseCode);
        drawText("Error !!!", 0, 16, 2);
        display.display();
        //Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    // Free resources
    http.end();
    endWIFI();
}

void setup()
{
    Serial.begin(115200);
    DEBUG_PRINTLN("ESP8266-airly\\ESP8266-airly.ino");
    initDisplay();
    display.clearDisplay();
    // display.drawBitmap(0, 16, very_happy_bmp, TEMP_WIDTH, TEMP_HEIGHT, 1);
    // display.drawBitmap(16, 16, happy_bmp, TEMP_WIDTH, TEMP_HEIGHT, 1);
    // display.drawBitmap(32, 16, normal_bmp, TEMP_WIDTH, TEMP_HEIGHT, 1);
    // display.drawBitmap(48, 16, sad_bmp, TEMP_WIDTH, TEMP_HEIGHT, 1);
    // display.drawBitmap(0, 32, dead_bmp, TEMP_WIDTH, TEMP_HEIGHT, 1);
    // //drawText("test", 48, 0, 2);
    // display.display();
    initWIFI();
    getData();
}

void loop()
{
    // if ((millis() - lastTime) > timerDelay)
    // {
    //     lastTime = millis();
    // }
}
