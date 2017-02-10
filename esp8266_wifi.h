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
#ifndef ESP8266_WIFI_H
#define ESP8266_WIFI_H

#include <libMiletusCommIf.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <string>

#ifndef HTTP_HEADER
#define HTTP_HEADER "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
#endif
#ifndef HTTP_HEADER_ERROR
#define HTTP_HEADER_ERROR "HTTP/1.1 404 Not Found\r\n\r\n"
#endif

class esp8266_wifi : public MiletusDeviceCommIf
{
public:
  esp8266_wifi(const char* deviceName,
               const char* ssid,
               const char *password,
               const uint8_t *ip_vet = 0,
               const uint8_t *gateway_vet = 0,
               const uint8_t *subnet_vet = 0);
  ~esp8266_wifi(){};
  int handleEvent(RequestT *);
  bool sendJsonToClient(std::string json);
  bool sendErrorToClient();

private:
  WiFiServer server{TCP_SERVER_PROT};
  WiFiClient client;
};

#endif //ESP8266_WIFI_H
