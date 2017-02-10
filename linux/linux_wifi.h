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
#ifndef linux_WIFI_H
#define linux_WIFI_H

#include "../libMiletusCommIf.h"
#include "linux_wifi.h"
#include "linux_wrapper.h"
#include <string>

#ifndef HTTP_HEADER
#define HTTP_HEADER "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
#endif
#ifndef HTTP_HEADER_ERROR
#define HTTP_HEADER_ERROR "HTTP/1.1 404 Not Found\r\n\r\n"
#endif

using namespace std;

class linux_wifi : public MiletusDeviceCommIf
{
public:
  linux_wifi();
  ~linux_wifi(){};
  int handleEvent(RequestT *);
  bool sendJsonToClient(string json);
  bool sendErrorToClient();

private:
  LinuxServer server{TCP_SERVER_PROT};
  LinuxClient client;
};

#endif //linux_WIFI_H
