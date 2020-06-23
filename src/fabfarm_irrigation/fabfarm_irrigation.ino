/* --------
 * LucioPGN                             
 * --------
 *
 * Contributors:
 * - Jeffrey Knight http://github.com/jknight
 */

#include <fstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>


#include "Adafruit_Sensor.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "AsyncTCP.h"
#include "DHT.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "WiFi.h"

const char *dataFile = "data.json";
const char *ntpServer = "us.pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// Network Credentials
//const char *ssid = "rato";
// const char *password = "imakestuff";

//Start the Async Web Server listening on port 80
AsyncWebServer server(80);

// "the brains" - a json object
DynamicJsonDocument doc(2048); // from arduinoJson

// Digital pin connected to the DHT sensor
#define DHTPIN 32
// Uncomment the type of sensor in use:
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
//Send the pin and type of sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {

  // Serial port for debugging purposes
  Serial.begin(9600);

  // Initialize SPIFFS
  //That is the file system.
  if (!SPIFFS.begin(true)) {
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
  Serial.println("json deserialize - BEGIN");
  deserializeJson(doc, json);
  JsonObject data = doc["data"];
  const char* ssid = data["ssid"];
  const char* password = data["password"];

  // TODO: set OUTPUT for each relay
  // TODO: also set each to off initially
  //pinMode(relayGPIOs[i - 1], OUTPUT);
  //digitalWrite(relayGPIOs[i - 1], HIGH);

  //Lucio TODO: proactively disable everything / consider if we want to have it start in stopped state

  Serial.printf("Connecting to wifi ssid:%s / password: %s", ssid, password);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println("The Fabfarm Irrigation system network IP is:");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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
    data["currentTime"] = timeinfo.tm_hour; // TODO: why aren't times working ?
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

    Serial.println("Saving to disk - BEGIN");
    //NOTE: we've had some issues with files not saving (permissions?)
    File file = SPIFFS.open("/data.json", "w");
    if (!file {
      Serial.println("Faile to open file for writing");
    }

    int bytesWritten = file.print(jsonString);
    file.close();
    Serial.printf("Saving to disk - COMPLETE(%d bytes)\n", bytesWritten);

    request->send(200); // "application/json", jsonString);
    Serial.println("-------------------");
    Serial.println(jsonString);
    Serial.println("-------------------");
  });

  server.addHandler(handler);

  // Start server 
  server.begin();
}

// turn things on and off
void loop() {

  //sleep for a sec
  //delay(500);

  JsonArray data = doc["data"];
  int override = data["override"];
  // if(override) { .... }

  JsonArray relays = doc["relays"];

  for (int i = 0; i < relays.size(); i++) {

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

String readDHTTemperature() {

  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return "Failed to read temperator from sensor";
  } else { 
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity()
{
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "Failed to read humidity from sensor";
  } else { 
    Serial.println(h);
    return String(h);
  }
}

//this function was found here https://arduino.stackexchange.com/questions/52676/how-do-you-convert-a-formatted-print-statement-into-a-string-variable
//I did a minor change so instead of a void function it now returns a string to be used to show time in the webinterface
String printFarmTime() {

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

String printShortFarmTime() {

  time_t rawtime;
  struct tm timeinfo;

  getLocalTime(&timeinfo);
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "H:M:S", &timeinfo);

  Serial.println("Checking time - BEGIN");
  Serial.printf("Hour: %d\n", timeinfo.tm_hour);
  Serial.printf("Minute: %d\n", timeinfo.tm_min);
  Serial.println("Checking time - COMPLETE");
  
  //Construct to create the String object
  String timeAsAString(timeStringBuff);
  return timeAsAString;
}

// Function based on post in the https://forum.arduino.cc/index.php?topic=536464.0 Arduino Forum by user Abhay
void modifiedPrintLocalTime() {

  int onlyYear;
  int onlyMonth;
  int onlyDay;
  int onlyHour;
  int onlyMin;
  int onlySec;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
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

