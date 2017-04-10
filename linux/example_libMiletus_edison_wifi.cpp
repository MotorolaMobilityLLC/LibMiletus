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

In this example, we demonstrate how to implement a Linux IoT device that has a
command that toggles that defines the Edson's built-in LED.

*************************************************************************/

#include "../libMiletus.h"
#include "linux_wifi.h"
#include "linux_provider.h"
#include <mraa.h>

#define DEVICE_NAME "My_Favorite_IOT_Device"
#define BUILT_IN_LED 8

/* Keep the least values */
bool led_state = false;
static mraa_gpio_context led_pin;

/* This is the LibMiletus IoT device. */
MiletusDevice _myDevice(DEVICE_NAME);

/* Component. In this example, the IoT device has a single component. */
const char * kComponentName = "sensor";

char kTraits[] = R"({
  "trait": {
    "commands": {
      "toggleLED": {
        "minimalRole": "user",
        "parameters": {
        }
      }
    }
  }
})";

bool toggleLED_handler(Command& command)
{
  printf("I'm called\n");
  led_state = !led_state;
  mraa_gpio_write(led_pin, led_state);
  return true;
}


void loop()
{
  /* Delay some ms */
  sleep(1);
  /* Let libMiletus handle requests. */
  _myDevice.handleEvents();
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
  traits_list.push_back("trait");
  _myDevice.addComponent(component, traits_list);

  /* Map commant to handler. */
  component->setCommand("trait", "toggleLED", toggleLED_handler);

  /* Create a communication interface and add it to the device. */
  linux_wifi *myWifiIf = new linux_wifi();
  _myDevice.addCommInterface(myWifiIf);

  led_pin = mraa_gpio_init(BUILT_IN_LED);
  mraa_gpio_dir(led_pin, MRAA_GPIO_OUT);

  while(1){
    loop();
  }
}
