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
*************************************************************************/
#include "esp8266_ble.h"

class MiletusDeviceCommIf;

esp8266_ble::esp8266_ble(const char *deviceName) {
  yield();
  Serial.println("Starting HM-10 BLE device");
  bleSerial.begin(BAUD_RATE);
  bleSerial.setTimeout(TIMEOUT); // maybe change it!

  // AT+NAME[para1] OK+Set[para1]
  //     Para1: module name, Max length is 12.
  // AT+RESET OK+RESET
  for (int i = 0; i < ATTEMPTS; i++) {
    // cheking Serial connection
    Serial.println("Sending AT");
    bleSerial.print("AT");
    delay(100);
    String response = bleSerial.readString();
    if (strcmp(strdup(response.c_str()), "OK") != 0) {
      Serial.println("HM-10 startup fail.");
      Serial.print("---->> Received response: ");
      Serial.println(response);
      continue;
    }
    // Changing the device name
    // TODO put it in sendHM10Command
    String codeToSend = "AT+NAME";

    codeToSend += String(deviceName).substring(0, 6);

    // Get a randon number here
    codeToSend += String(123123, HEX).substring(0, 2);

    codeToSend += ".mi";
    bleSerial.print(codeToSend);
    delay(100);
    response = bleSerial.readString();
    if (response.charAt(0) != 'O' || response.charAt(1) != 'K') {
      Serial.println("HM-10 set name fail.");
      Serial.print("---->> Received response: ");
      Serial.println(response);
      continue;
    }
    Serial.println("HM-10 set name success.");
    Serial.print("---->> Received response: ");
    Serial.println(response);
    initialized = true;
    // Rebooting HM-10 with new name
    bleSerial.print("AT+RESET");
    deviceTransportClass = TransportClass::BLE;
    Serial.println("HM-10 BLE device up");
    return;
  }
  Serial.println("HM-10 startup fail, check baud-rate.");
}

int esp8266_ble::handleEvent(RequestT *request) {
  yield();
  if (!bleSerial.available()) {
    Serial.print(",");
    request->status = 0;
    return 1;
  }
  Serial.println("\nNew BLE client");
  String incomming_package = bleSerial.readString();
  if ((incomming_package.indexOf("length=") != 0) ||
      (incomming_package.charAt(11) != 'B') ||
      incomming_package.indexOf("offset=", incomming_package.length() - 15) <
          0 ||
      (incomming_package.charAt(incomming_package.length() - 1) != 'B')) {
    Serial.println("BLE package is corrupted.\nSize Header problem");
    return 2;
  }
  unsigned int package_sz_i;
  package_sz_i = incomming_package.substring(7, 11).toInt();
  Serial.print("package_sz_i: ");
  Serial.println(package_sz_i);
  if (package_sz_i !=
      incomming_package
          .substring(incomming_package.length() - 5,
                     incomming_package.length() - 1)
          .toInt()) {
    Serial.println("Start and end package size do not match!");
    return 2;
  }
  if (incomming_package.length() != package_sz_i + 12) {
    Serial.println("BLE package is corrupted.\nPackage size do not match");
    return 2;
  }

  String req_header = incomming_package.substring(
      12, incomming_package.indexOf(FIRST_SEPARATOR));
  String req_body = incomming_package.substring(
      12 + req_header.length(), incomming_package.length() - 13);

  Serial.print("req_header: ");
  Serial.println(req_header);
  Serial.print("req_body: ");
  Serial.println(req_body);

  if (req_header == "/info") {
    request->status = INFO;
    return 0;
  } else if (req_header == "/traits") {
    request->status = TRAITS;
    return 0;
  } else if (req_header == "/components") {
    request->status = COMPONENTS;
    return 0;
  } else if (req_header.indexOf("/commands") == 0) {
    // request example:
    // "/execute/command/?application=json&value={json_file_content}&end"
    if (req_body.indexOf("application=json") < 1) {
      Serial.println("BLE package is corrupted.\napplication=json < 1 ");
      return 2;
    }
    if (req_body.indexOf("value=") < 1) {
      Serial.println("BLE package is corrupted.\nvalue < 1");
      return 2;
    }
    req_body = req_body.substring(req_body.indexOf("value=") + 6);

    Serial.print("req_body: ");
    Serial.println(req_body);

    char *packageBody = new char[req_body.length() + 1];
    req_body.toCharArray(packageBody, req_body.length() + 1);
    // TODO: check if these +1 on toCharArray's second argument is mandatory.
    request->commandJson = packageBody;
    delete[] packageBody;

    request->status = COMMANDS;
    if (req_header.indexOf("/commands/execute") == 0) {
      request->commandID = COMMAND_EXECUTE;
    } else if (req_header.indexOf("/commands/status") == 0) {
      request->commandID = COMMAND_STATUS;
    } else if (req_header.indexOf("/commands/list") == 0) {
      request->commandID = COMMAND_LIST;
    } else if (req_header.indexOf("/commands/cancel") == 0) {
      request->commandID = COMMAND_CANCEL;
    }
  } else {
    request->status = UNKNOWN;
    return -1;
  }
  Serial.println("Done with BLE client");
  return 0;
}

bool esp8266_ble::sendJsonToClient(std::string json) {
  yield(); // yield to allow ESP8266 background functions
  static int pow10[10] = {1,      10,      100,      1000,      10000,
                          100000, 1000000, 10000000, 100000000, 1000000000};
  // concatenate zeros until get 4 digits
  String length_s = "";
  for (int i = 10; i <= pow10[(NUMBER_DIGITIS_SIZE - 1)]; i *= 10) {
    Serial.print("i: ");
    Serial.println(i);
    if ((strlen(json.c_str()) + 12) < i) {
      length_s += "0";
    }
  }
  length_s += (strlen(json.c_str()) + 12);
  bleSerial.print("length=");
  bleSerial.print(length_s);
  bleSerial.print("B");
  delay(30); // wait HM-10 split it in tow packages
  bleSerial.print(json.c_str());
  bleSerial.print("offset=");
  bleSerial.print(length_s);
  bleSerial.print("B");
  yield();
}

bool esp8266_ble::sendErrorToClient() {
  yield(); // yield to allow ESP8266 background functions
  bleSerial.print("ERROR_MESSAGE_HERE");
}
