#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>
#include "../common/wifi-ssid.h"

#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINT(x) Serial.print(x)
#else
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINT(x)
#endif

//Your Domain name with URL path or IP address with path

//String URL = "https://raw.githubusercontent.com/marnowicki/Arduino/master/test-data/airly-32811-part.json";
//String URL = "https://raw.githubusercontent.com/marnowicki/Arduino/master/test-data/airly-32811.json";
String URL = "http://192.168.1.187:5500/aitly-32811.json";
//String URL = "https://raw.githubusercontent.com/marnowicki/Arduino/master/test-data/user.json";

//String serverName = "https://airapi.airly.eu";
//String serverName = "https://jsonplaceholder.typicode.com";
//String serverName = "https://jigsaw.w3.org/HTTP/connection.html";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 6000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 15000;

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  DEBUG_PRINTLN("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN("");
  DEBUG_PRINT("Connected to WiFi network with IP Address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  DEBUG_PRINTLN("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop()
{
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay)
  {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      //WiFiClientSecure client;
      //client.setInsecure();

      HTTPClient http;

      //String serverPath = serverName + "/v2/measurements/installation?installationId=32811&indexType=AIRLY_CAQI&indexPollutant=PM&includeSubindexes=true&includeWind=true&apikey=AKfrckNq4WjlMTuD5cUs1yAeu76MErr0";
      //String serverPath = serverName + "/todos/1";
      //String serverPath ="https://jsonplaceholder.typicode.com/users/1";
      // Your Domain name with URL path or IP address with path
      //http.begin(serverPath.c_str());
      //DEBUG_PRINTLN(http.begin(client, URL.c_str()));
      DEBUG_PRINTLN(http.begin(URL));

      // Send HTTP GET request
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0)
      {
        DEBUG_PRINT("HTTP Response code: ");
        DEBUG_PRINTLN(httpResponseCode);
        //DEBUG_PRINT("HTTP Size "); DEBUG_PRINTLN(http.getSize());
        //String payload = http.getString();

        //DEBUG_PRINT("Payload length "); DEBUG_PRINTLN(payload.length());
        //DEBUG_PRINT("Payload subtring: "); DEBUG_PRINTLN(payload.substring(0, 50));

        WiFiClient stream = http.getStream();
        stream.setTimeout(5000);
        DEBUG_PRINT("Stream readString: ");
        String payload = stream.readString();
        DEBUG_PRINTLN(payload.substring(0, 50));
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

        DEBUG_PRINT("Payload length ");
        DEBUG_PRINTLN(payload.length());
        DEBUG_PRINT("Payload subtring head: ");
        DEBUG_PRINTLN(payload.substring(0, 50));
        DEBUG_PRINT("Payload subtring tail: ");
        DEBUG_PRINTLN(payload.substring(payload.length() - 50, payload.length()));

        JSONVar myObject = JSON.parse(payload);
        DEBUG_PRINT("JSON Type ");
        DEBUG_PRINTLN(JSON.typeof(myObject));

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
          for (int i = 0; i < values.length(); i++)
          {
            JSONVar o = values[i];
            DEBUG_PRINT("name ");
            DEBUG_PRINT(o["name"]);

            DEBUG_PRINT("   value ");
            DEBUG_PRINTLN(o["value"]);
            //Serial.printf("name: %s  value: %s",o["name"], o["value"]);
          }
        }
      }
      else
      {
        DEBUG_PRINT("Error code: ");
        DEBUG_PRINTLN(httpResponseCode);
        //Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      // Free resources
      http.end();
    }
    else
    {
      DEBUG_PRINTLN("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
