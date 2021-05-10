/****************************************************************************
 *                                  Aknowledments                           *
 *                                  by LucioPGN                             *
 ****************************************************************************/
/*  Up to this date: 07th of June 2020 I don't consider myself a programer
 *  so I need to stand on top of giants sholders for my programing projects:
 *  A Portion of this code was based on Rui Santos Code;
 *  Codes from Rui Santos mixed toghether:
 *  https://randomnerdtutorials.com/esp32-web-server-spiffs-spi-flash-file-system/
 *  https://randomnerdtutorials.com/esp32-relay-module-ac-web-server/
 *  https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/
 *  From Techtutorialsx.com
 *  https://techtutorialsx.com/2017/12/01/esp32-arduino-asynchronous-http-webserver/
 *  A Portion of this code was based on Shakeels code for ESP8266 ;
 *  My contributions:
 *     -So far I made it work on platformio :), that took me quite a lot of time
 *     -That means:
 *        +created a new project;
 *        +created a folder named data under the main folder (fabfarm_irrigation)
 *        +linked the platformio.ini to the folder of the project + the data folder
 *        +linked the needed libraries to their github repo in the platformio.ini
 *        +found a conflict with time library and A
 *  Things I still want to program for my final project:
 *    -so far I ported Shakeels code into ESP32;
 *    -separate functions from shakeels code into files;
 *    -separated functions from Rui Santos code into files.
 *    -simplify the code creating more functions;
 *    -try to separate the HTML part for a cleaner code;
 *    -Improve the appearance/Interface of the code
 *    -Add readings to HTML
 *    -Add a log of occurrences like over current
 *    -Add more safety for the equipment
 *    -Add a phone interface (APP)
 *    -Add function to set current time
 *    -Add renaming function to each relay so one can relate the relay to the area of interest or at least rename relays to actual areas of the farm.
 *
 ****************************************************************************/

#include <fstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>

//Required Libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "AsyncTCP.h"
#include "Adafruit_Sensor.h"
#include "DHT.h"

#include "AsyncJson.h"
#include "ArduinoJson.h"

// read / write json to save state
const char *dataFile = "data.json";
const char *ntpServer = "us.pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// Network Credentials
const char *ssid = "fabfarm_ele_container";
const char *password = "imakestuff";

//Start the Async Web Server listening on port 80
AsyncWebServer server(80);

// "the brains" - a json object
DynamicJsonDocument doc(2048); // from arduinoJson

// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO false

//TODO: this will go into the json
// Set number of relays, will be used in the array
#define NUM_RELAYS 4
// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {26, 25, 33, 27};

// Digital pin connected to the DHT sensor
#define DHTPIN 32

// Uncomment the type of sensor in use:
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

//Send the pin and type of sensor
DHT dht(DHTPIN, DHTTYPE);

//
const char *PARAM_INPUT_1 = "relay";
const char *PARAM_INPUT_2 = "state";

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(9600);

  // Initialize SPIFFS
  //That is the file system.
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // we always read the data.json from disk on startup (always!)
  // Read json from the file ...

  File f = SPIFFS.open("/data.json", "r");
  String json = f.readString();
  Serial.println("read file - BEGIN");
  Serial.println(json);
  Serial.println("read file - COMPLETE");
  f.close();

  // we take json from disk & create json object
  Serial.println("json deserialize test - BEGIN");
  deserializeJson(doc, json);

  // TODO: set OUTPUT for each relay
  // TODO: also set each to off initially
  //pinMode(relayGPIOs[i - 1], OUTPUT);
  //digitalWrite(relayGPIOs[i - 1], HIGH);

  //Lucio TODO: proactively disable everything / consider if we want to have it start in stopped state

  WiFi.softAP("softap", "imakestuff");
  IPAddress IP = WiFi.softAPIP();

  //here  start wifi sessions as a client.
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println("The Fabfarm Irrigation system network IP is:");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printShortFarmTime();
  // Get time from time server

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/all.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/all.css", "text/css");
  });
  server.on("/logo.jpeg", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/logo.jpeg", "image/jpeg");
  });

  server.on("/getData", HTTP_GET, [](AsyncWebServerRequest *request) {
    /* 
    1) start with our json object.
    2) we get new data (temp/humidit/y/relay status)
    3) update the json object
    4) serialize ==> json
    5) return json to html 
    */

    struct tm timeinfo;
    getLocalTime(&timeinfo);

    Serial.println("/getData");
    JsonObject data = doc["data"];
    data["currentTime"] = printShortFarmTime(); // TODO: why aren't times working ?
    data["temperature"] = readDHTTemperature(); // TODO: fix me
    data["humidity"] = readDHTHumidity();

    char json[1024];
    Serial.println("Serialize json & return to caller - BEGIN");

    serializeJson(doc, json);

    Serial.println("Serialize json & return to caller - COMPLETE");

    request->send(200, "application/json", json);
  });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/updateData", [](AsyncWebServerRequest *request, JsonVariant &json) {
    doc = json.as<JsonObject>();

    String jsonString;
    serializeJson(doc, jsonString);

    //write this to disk
    // Read json from the file ...
    Serial.println("Saving to disk - BEGIN");
    File f = SPIFFS.open("/data.json", "w");
    if (!f)
    {
      Serial.println("Faile to open file for writing");
    }

    int bytesWritten = f.print(jsonString);
    f.close();
    Serial.printf("Saving to disk - COMPLETE(%d bytes)\n", bytesWritten);

    request->send(200); // "application/json", jsonString);
    Serial.println("-------------------");
    Serial.println(jsonString);
    Serial.println("-------------------");
  });

  server.addHandler(handler);

  // Start server here
  server.begin();
}

// turn things on and off
void loop()
{
  //sleep for a sec
  //delay(500);

  JsonArray data = doc["data"];
  int override = data["override"];
  // if(override) { .... }

  JsonArray relays = doc["relays"];

  for (int i = 0; i < relays.size(); i++)
  {
    const char *relayName = relays[i]["name"]; // "relay1"
    int pin = relays[i]["pin"];                // 123
    int isEnabled = relays[i]["isEnabled"];    // 1
    int duration = relays[i]["duration"];      // 1000
    /* 
    for (int j = 0; j < relays[i]["times"].size(); j++)
    {
      JsonObject times = relays[i][j].createNestedObject();
      const char *startTime = times["startTime"]; // = "12:45";
      int duration = times["duration"];           // = 25;
      ---> turn on and off in here <---
    }
    */

    pinMode(pin, OUTPUT);
    digitalWrite(pin, isEnabled ? HIGH : LOW);
  }

  //      Serial.println("looping ...");
  //turns things on and off: digitalWrite(relayGPIOs[i - 1], RELAY_NO ? HIGH : LOW);
  //if (digitalRead(relayGPIOs[valveRelayNum - 1]))

  /*
if override is manual
  for each relay
    if enabled turn on
    else turn off
else
    for each relay
      if time frame maches turn on
      else turn off

*/
}

String readDHTTemperature()
{
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return "Failed to read temperator from sensor";
  }
  else
  {
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity()
{
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h))
  {
    Serial.println("Failed to read from DHT sensor!");
    return "Failed to read humidity from sensor";
  }
  else
  {
    Serial.println(h);
    return String(h);
  }
}

//this function was found here https://arduino.stackexchange.com/questions/52676/how-do-you-convert-a-formatted-print-statement-into-a-string-variable
//I did a minor change so instead of a void function it now returns a string to be used to show time in the webinterface
String printFarmTime()
{
  time_t rawtime;
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "A,BdYH:M:S", &timeinfo);
  //print like "const char*"
  Serial.println(timeStringBuff);

  //Construct to create the String object
  String timeAsAString(timeStringBuff);
  return timeAsAString;
}

String printShortFarmTime()
{
  time_t rawtime;
  struct tm timeinfo;

  getLocalTime(&timeinfo);
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "H:M:S", &timeinfo);

  //print like "const char*"
  Serial.println("Checking time - BEGIN");
  Serial.printf("Hour: %d\n", timeinfo.tm_hour);
  Serial.printf("Minute: %d\n", timeinfo.tm_min);
  Serial.println(timeStringBuff);
  Serial.println("Checking time - COMPLETE");

  //Construct to create the String object
  String timeAsAString(timeStringBuff);
  return timeAsAString;
}

void modifiedPrintLocalTime()
// Function based on post in the https://forum.arduino.cc/index.php?topic=536464.0 Arduino Forum by user Abhay
{
  int onlyYear;
  int onlyMonth;
  int onlyDay;
  int onlyHour;
  int onlyMin;
  int onlySec;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "mdY /H:M:S");
  //scanf(&timeinfo, "mdY /H:M:S")
  onlyHour = timeinfo.tm_hour;
  onlyMin = timeinfo.tm_min;
  onlySec = timeinfo.tm_sec;
  onlyDay = timeinfo.tm_mday;
  onlyMonth = timeinfo.tm_mon + 1;
  onlyYear = timeinfo.tm_year + 1900;

  //test
  Serial.print("Print only Hour and Minutes...");
  Serial.print(onlyHour);
  Serial.print(":");
  Serial.print(onlyMin);
}

// Set all relays to off when the program starts - if set to Normally Open (NO), the relay is off when you set the relay to HIGH
// TODO: think about this
void initializeRelays()
{
  for (int i = 1; i <= NUM_RELAYS; i++)
  {
    pinMode(relayGPIOs[i - 1], OUTPUT);
    //digitalWrite(relayGPIOs[i - 1], RELAY_NO ? HIGH : LOW);
  }
}