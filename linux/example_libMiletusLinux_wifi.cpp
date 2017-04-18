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

In this example, we demonstrate how to implement a IoT device that
have no commands but expose the state of an attached DHT sensor receives.
You must adjust the code to reflect the setting of your wifi network (Hint:
look for MODIFY strings).

*************************************************************************/

#include "../libMiletus.h"
#include "linux_wifi.h"
#include "linux_provider.h"

#define DEVICE_NAME "My_Favorite_IOT_Device"

/* Keep the least values to avoid unnecessary updates */
float temperature = 0;
float humidity = 0;

/* This is the LibMiletus IoT device. */
MiletusDevice _myDevice(DEVICE_NAME);

/* Component. In this example, the IoT device has a single component. */
const char * kComponentName = "sensor";

char kTraits[] = R"({
  "sensors": {
    "commands": {
      "toggleLED": {
        "minimalRole": "user",
        "parameters": {
        }
      }
    },
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

bool toggleLED_handler(Command& command)
{
  printf("I'm called\n");
  command.appendResult("CurrentTemp", temperature);
  return true;
}

void updateSensors(){
  temperature+= 0.1;
  humidity+= 0.15;
  _myDevice.setState(kComponentName, "sensors", "temperature" , temperature);
  _myDevice.setState(kComponentName, "sensors", "humidity" , humidity);
}

void loop()
{
  /* Delay 50 ms */
  sleep(1);
  /* Let libMiletus handle requests. */
  _myDevice.handleEvents();
  /* Check for updates on DHT sensor. */
  updateSensors();
}

int main()
{
  /* Add basic hardware features provider. */
  _myDevice.setProvider(new linuxMiletusProvider());

  /* Load device traits. */
  _myDevice.loadJsonTraits(kTraits);

  /* Add a component to the device. */
  Component* component = new Component(kComponentName);
  list<const char*> traits_list;
  traits_list.push_back("sensors");
  _myDevice.addComponent(component, traits_list);

  /* Map commant to handler. */
  component->setCommand("sensors", "toggleLED", toggleLED_handler);

  /* Create a communication interface and add it to the device.
   * Using ESP8266 Wifi Interface. */
  linux_wifi *myWifiIf = new linux_wifi();
  _myDevice.addCommInterface(myWifiIf);

  while(1){
    loop();
  }
}
