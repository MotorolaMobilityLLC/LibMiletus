/*************************************************************************
The MIT License (MIT)
Copyright (c) 2016 Jeferson Rech Brunetta -- ra161253@students.ic.unicamp.br
Copyright (c) 2016 Edson Borin -- edson@ic.unicamp.br
Copyright (c) 2016 João Batista C. G. Moreira -- joao.moreira@lsc.ic.unicamp.br

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Description:

In this example, we demonstrate how to implement a IoT device that receives
commands to switch the NodeMCU builtin LED have no commands but expose the
state of an attached DHT sensor receives. You must adjust the code to reflect
the setting of your wifi network (Hint: look for MODIFY strings).

*************************************************************************/

#include <libMiletus.h>
#include <esp8266_wifi.h>
#include <esp8266_provider.h>
#include "DHT.h"

/* MODIFY: Modify this macro with your WiFi SSID */
#define WIFI_SSID "MY_WIFI_SSID"
/* MODIFY: Modify this macro with your WiFi password */
#define WIFI_PASSWORD "MY_WIFI_PASSWORD"
#define DEVICE_NAME "My_Favorite_IOT_Device"

/* MODIFY: Modify this macro with your DHT plugged pin */
#define DHTPIN D2
/* MODIFY: Modify this macro with your DHT model */
#define DHTTYPE DHT11

/* Global variable indicating the current LED state. False == off, true == on.*/
bool LEDstate = false;

/* Keep the least values to avoid unnecessary updates */
float temperature = 0;
float humidity = 0;

/* This is the LibMiletus IoT device. */
MiletusDevice _myDevice(DEVICE_NAME);

/* This is the DHT object. */
DHT dht(DHTPIN, DHTTYPE);

/* Component. In this example, the IoT device has a single component. */
const char * kComponentName = "sensor";

char kTraits[] = R"({
  "DHT": {
    "state": {
      "temperature": {
        "isRequired": true,
        "type": "number"
      },
      "humidity": {
        "isRequired": true,
        "type": "number"
      }
    }
  }
})";

char kTraits2[] = R"({
  "lamp": {
    "commands": {
      "toggleLED": {
        "minimalRole": "user",
        "parameters": {
        }
      }
    },
    "state": {
      "LEDstatus": {
        "isRequired": true,
        "type": "boolean"
      }
    }
  }
})";

void setup()
{
  /* Add basic hardware features provider. */
  _myDevice.setProvider(new ESP8266MiletusProvider());

  /* Load device traits. */
  _myDevice.loadJsonTraits(kTraits);
  _myDevice.loadJsonTraits(kTraits2);

  /* Add a component to the device. */
  Component* sensor = new Component(kComponentName);
  list<const char*> traits_list;
  traits_list.push_back("DHT");
  traits_list.push_back("lamp");
  _myDevice.addComponent(sensor, traits_list);

  /* Create a communication interface and add it to the device.
   * Using ESP8266 Wifi Interface. */
  esp8266_wifi *myWifiIf = new esp8266_wifi(DEVICE_NAME,
                                            WIFI_SSID,
                                            WIFI_PASSWORD);
  _myDevice.addCommInterface(myWifiIf);

  /* Defines LED pin as an putput */
  pinMode(LED_BUILTIN, OUTPUT);
}

void updateDHT(){
  float h = dht.readHumidity();
  float t = dht.readTemperature(false);
  if (!isnan(h) && !isnan(t)) {
    if (t != temperature){
      temperature = t;
      _myDevice.setState(kComponentName, "DHT", "temperature" , t);
    }
    if (humidity != h){
      humidity = h;
      _myDevice.setState(kComponentName, "DHT", "humidity" , h);
    }
  }
}

void loop()
{
  delay(100);
  /* Let libMiletus handle requests. */
  _myDevice.handleEvents();
  /* Check for updates on DHT sensor. */
  updateDHT();
}
