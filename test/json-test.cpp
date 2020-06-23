#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include "ArduinoJson-v6.15.2.h"

/* 
Sample POC for json serialize / deserialize 
---
Command line compiled + run with:
> g++ -I. test.cpp -o test.exe
> ./test.exe 

Sample json for https://arduinojson.org/v6/assistant/
{
    "data": { "time": 1351824120, "temperature": 23, "humidity": 45}, 
    "zones": [
      { "name": "relay1", "pin": 123,  "status": 1 , "duration": 1000, "times": ["00:00", "01:00"]}, 
      { "name": "relay2", "pin": 234, "status": 1 , "duration": 3000, "times": ["00:00", "01:00"]}, 
      { "name": "relay3", "pin": 345, "status": 0, "duration": 2000, "times": ["00:00", "01:00"]}
    ]
  }
*/

using namespace std;

// global cache of current json
DynamicJsonDocument doc(2048);

// prototypes
void serialize();
void test(const char*);
void deserialize(const char*);
void loopOnce();

int main() { 

    // Write json to a file ...
    /*
    const char *output = "test.json";
    ofstream f;
    f.open (output);
    const char* json = "{\"data\":{\"time\":1351824120,\"temperature\":23,\"humidity\":45},\"relays\":[{\"name\":\"relay1\",\"pin\":123,\"status\":1,\"duration\":1000,\"times\":[\"00:00\",\"01:00\"]},{\"name\":\"relay2\",\"pin\":234,\"status\":1,\"duration\":3000,\"times\":[\"00:00\",\"01:00\"]},{\"name\":\"relay3\",\"pin\":345,\"status\":10,\"duration\":2000,\"times\":[\"00:00\",\"01:00\"]}]}";
    f <<  json;
    f.close();
    */

    // Read json from the file ...
    ostringstream sstream;
    ifstream fs("hello.txt");
    sstream << fs.rdbuf();
    const string str(sstream.str());
    const char* ptr = str.c_str();
    cout<<"Got json:\n"<<ptr<<"\n";

    // deserialize json and update some vales 
    //test(ptr);

    return 0;
}

// Confirm that we can take json, deserialize it, set new values, and reserialize it back to Json (yes)
void test(const char* json) {

    //Does this need to change whenever we alter our json ? :(. Can we just do a 1024 or some large enough value on it? (YES?)
    //const size_t capacity = 3*JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 3*JSON_OBJECT_SIZE(5) + 210; 
    //DynamicJsonDocument doc(2048);
    //json string to object
    deserializeJson(doc, json); 

    //we'll want to actually *set* values here then serialize back to json 
    JsonObject data = doc["data"];

    //Set current values:
    data["time"] = "111111"; // TODO: string or int?
    data["temperature"] = "2222222";  //TODO: string or ints?
    data["humidity"] = "333333"; //TODO: string or ints ?

    // For command line testing, just cout - but arduinojson can serialize to Serial strea, Serial, etc
    string output;
    serializeJson(doc, output);
    //deserializeJson(doc, Serial);
    cout<<"Got updated json:\n"<<output<<"\n";

    loopOnce();
}

void loopOnce() {

    //JsonObject data = doc["data"];
    //JsonArray relays = doc.createNestedArray("relays");
    JsonArray relays = doc["relays"];
    cout<<"Relays ...\n"<<relays.size()<<"\n";
    for(int i = 0; i < relays.size(); i++) {
        cout<<"-->"<<i<<"\n";
    }
}

void serialize() {

    //TODO: do we need to change this every time we change our json ? :(
    //const size_t capacity = 3*JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 3*JSON_OBJECT_SIZE(5);
    //DynamicJsonDocument doc(capacity);

    JsonObject data = doc["data"];
    data = doc.createNestedObject("data");
    data["time"] = 1351824120;
    data["temperature"] = 23;
    data["humidity"] = 45;

    JsonArray relays = doc.createNestedArray("relays");

    for(int i =0; i < relays.size(); i++){
        
        relays[i]["isEnabled"] = 1;

    }

    JsonObject relays_0 = relays.createNestedObject();
    relays_0["name"] = "relay1";
    relays_0["pin"] = 123;
    relays_0["status"] = 1;
    relays_0["duration"] = 1000;

    JsonArray relays_0_times = relays_0.createNestedArray("times");
    relays_0_times.add("00:00");
    relays_0_times.add("01:00");

    JsonObject relays_1 = relays.createNestedObject();
    relays_1["name"] = "relay2";
    relays_1["pin"] = 234;
    relays_1["status"] = 1;
    relays_1["duration"] = 3000;

    JsonArray relays_1_times = relays_1.createNestedArray("times");
    relays_1_times.add("00:00");
    relays_1_times.add("01:00");

    JsonObject relays_2 = relays.createNestedObject();
    relays_2["name"] = "relay3";
    relays_2["pin"] = 345;
    relays_2["status"] = 10;
    relays_2["duration"] = 2000;

    JsonArray relays_2_times = relays_2.createNestedArray("times");
    relays_2_times.add("00:00");
    relays_2_times.add("01:00");

    //File file = SD.open("out.json", FILE_WRITE);
    string output;
    serializeJson(doc, output);
    //deserializeJson(doc, Serial);
    cout<<output<<"\n";

}

void deserialize(const char* json) {

    const size_t capacity = 3*JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 3*JSON_OBJECT_SIZE(5) + 210;
    DynamicJsonDocument doc(capacity);
    // start with json string 
    deserializeJson(doc, json);

    JsonObject data = doc["data"];

    long data_time = data["time"]; // 1351824120
    int data_temperature = data["temperature"]; // 23
    int data_humidity = data["humidity"]; // 45

    JsonArray relays = doc["relays"];

    JsonObject relays_0 = relays[0];
    const char* relays_0_name = relays_0["name"]; // "relay1"
    int relays_0_pin = relays_0["pin"]; // 123
    int relays_0_status = relays_0["status"]; // 1
    int relays_0_duration = relays_0["duration"]; // 1000

    const char* relays_0_times_0 = relays_0["times"][0]; // "00:00"
    const char* relays_0_times_1 = relays_0["times"][1]; // "01:00"

    JsonObject relays_1 = relays[1];
    const char* relays_1_name = relays_1["name"]; // "relay2"
    int relays_1_pin = relays_1["pin"]; // 234
    int relays_1_status = relays_1["status"]; // 1
    int relays_1_duration = relays_1["duration"]; // 3000

    const char* relays_1_times_0 = relays_1["times"][0]; // "00:00"
    const char* relays_1_times_1 = relays_1["times"][1]; // "01:00"

    JsonObject relays_2 = relays[2];
    const char* relays_2_name = relays_2["name"]; // "relay3"
    int relays_2_pin = relays_2["pin"]; // 345
    int relays_2_status = relays_2["status"]; // 10
    int relays_2_duration = relays_2["duration"]; // 2000

    const char* relays_2_times_0 = relays_2["times"][0]; // "00:00"
    const char* relays_2_times_1 = relays_2["times"][1]; // "01:00"
}
