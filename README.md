# FabLab Irrigation System

## Design

C++ for a Microprocessor that includes an embedded http server.

The http server displays controls for N zones, where each zone has:

- turn on time
- turn on duration

```
Valve1 \
Valve2  --- Microprocessor <==> Web Interface  
ValveN /
```

## Web server methods

| **Endpoint** | **Purpose**       |
|--------------|-------------------|
| /getData     | returns json      |
| /saveData    | gets updated json |

## Core Logic

1) start with our json object.
2) we get new data (temp/humidit/y/relay status)
3) update the json object
4) serialize ==> json
5) return json to html

**State Problem**  
When the microcontroller loses power, we lose state. We therefore need to persist settings to disk.

- Settings are *read* from disk on startup
- Settings are *saved* each time a user changes settings in the web ui
- Settings are kept in memory and we check against them

Psuedo code:
```
Startup: 
  - read json to doc obj
  - init wifi
  - init http server

/get 
  - serialize doc obj to Json
  - return Json to caller
  - no logic

/update 
  - take incoming Json and save it to disk
  - aduinoJson magically serializes incoming request to our doc object for free
  - no logic 

Loop (no disk i/o):
  - examine current state of in memory doc object and take action
```

## Data format

```json
{
    "data": {
        "currentTime": "Tuesday, June 23 2020 11:54:16",
        "temperature": "29.90",
        "humidity": "51.00",
        "override": 0,
        "ssid": "myssid",
        "pass": "my password"
    },
    "relays": [
        {
            "name": "Fruit Tree",
            "pin": 25,
            "isRunning": 1,
            "isEnabled": 0,
            "times": [
                {
                    "startTime": "03:00",
                    "duration": "0"
                }
            ]
        },
        {
            "name": "Vegie Garden",
            "pin": 26,
            "isRunning": 1,
            "isEnabled": 0,
            "status": 1,
            "times": [
                {
                    "startTime": "12:00",
                    "duration": "0"
                },
                {
                    "startTime": "12:30",
                    "duration": "0"
                }
            ]
        },
        {
            "name": "Cypress Hill",
            "pin": 33,
            "isRunning": 1,
            "isEnabled": 0,
            "status": 1,
            "times": [
                {
                    "startTime": "12:45",
                    "duration": "0"
                },
                {
                    "startTime": "10:00",
                    "duration": "0"
                },
                {
                    "startTime": "16:45",
                    "duration": "0"
                }
            ]
        }
    ]
}


```
