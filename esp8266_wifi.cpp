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
#include <esp8266_wifi.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <string>

class MiletusDeviceCommIf;

esp8266_wifi::esp8266_wifi (const char* deviceName,
                            const char* ssid,
                            const char *password,
                            const uint8_t *ip_vet,
                            const uint8_t *gateway_vet,
                            const uint8_t *subnet_vet)
  {
  yield();
  if (ip_vet && gateway_vet && subnet_vet) {
    IPAddress ip(ip_vet);
    IPAddress gateway(gateway_vet);
    IPAddress subnet(subnet_vet);

    WiFi.config(ip, gateway, subnet);
  }
  Serial.print("Connecting...\n");
  WiFi.begin((char*)ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("-");
    delay(500);
  }
  Serial.print("\nConnected to ");
  Serial.print(ssid);
  Serial.print("\nIP address: ");
  Serial.println(WiFi.localIP());
  // Request2 now contains the body of HTTP POST
  String total = String(deviceName) + String(LIBMILETUS_MDNS_SUFIX);
  // Starts the mDNS responder
  if (!MDNS.begin(total.c_str())) {
    Serial.print("Fail to start mDNS service");
    return;
  }
  // Start TCP (HTTP) server
  server.begin();

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", MDNS_PROT);
  initialized = true;
  // TODO: Is a name useful??
  commName = "ESP8266";
  yield();
}

int esp8266_wifi::handleEvent(RequestT * request){
  yield();
  client = server.available();
  client.setNoDelay(false);
  if (!client) {
    client.stop();
    request->status = NOCLIENT;
    return 1;
  }
  
  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    yield();
  }
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  String req2 = "";
  if (req.indexOf("POST")==0){
    req2 = client.readString();
    Serial.println("Command:");
    Serial.println(req2);
  }
  Serial.println("REQ:");
  Serial.println(req);
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    request->status = UNKNOWN;
    return -1;
  }
  req = req.substring(addr_start + 1, addr_end);
  client.flush();
  if (req == "/info")
  {
    request->status = INFO;
    return 0;
  }
  else if (req == "/traits")
  {
    request->status = TRAITS;
    return 0;
  }
  else if (req == "/components")
  {
    request->status = COMPONENTS;
    return 0;
  }
  else if (req.indexOf("/commands")==0)
  {
    if (req2.indexOf("application/json") < 0){
      request->status = UNKNOWN;
      return -1;
    }
    req2 = req2.substring(req2.indexOf("\r\n\r\n")+4);
    // Request2 now contains the body of HTTP POST
    char * httpBody = new char [req2.length()+1];
    req2.toCharArray(httpBody, req2.length()+1);
    request->commandJson = httpBody;
    delete[] httpBody;

    request->status = COMMANDS;
    if (req.indexOf("/commands/execute")==0){
      request->commandID = COMMAND_EXECUTE;
    }
    else if (req.indexOf("/commands/status")==0){
      request->commandID = COMMAND_STATUS;
    }
    else if (req.indexOf("/commands/list")==0){
      request->commandID = COMMAND_LIST;
    }
    else if (req.indexOf("/commands/cancel")==0){
      request->commandID = COMMAND_CANCEL;
    }
    return 0;
  }
  else
  {
    request->status = UNKNOWN;
    return -1;
  }
  return 0;
}

bool esp8266_wifi::sendJsonToClient(std::string json){
  yield();// yield to allow ESP8266 background functions
  client.print(HTTP_HEADER);
  const char * jsonC = json.c_str();
  client.print(jsonC);
  client.flush();
  client.stop();
  yield();
  Serial.printf("sendJsonToClient heap size: %u\n", ESP.getFreeHeap());
}

bool esp8266_wifi::sendErrorToClient(){
  yield();// yield to allow ESP8266 background functions
  client.print(HTTP_HEADER_ERROR);
  client.flush();
  client.stop();
  yield();
  Serial.printf("sendErrorToClient heap size: %u\n", ESP.getFreeHeap());
}
