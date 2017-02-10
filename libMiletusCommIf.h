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
#ifndef libMiletus_CommIf_H
#define libMiletus_CommIf_H

#include <string>

#define TCP_SERVER_PROT 80
#define MDNS_PROT 80
#define LIBMILETUS_MDNS_SUFIX "_miletus"

#define NOCLIENT            0
#define UNKNOWN             1
// Reserved       2...19
#define INFO                20
#define TRAITS              21
#define COMPONENTS          22
#define COMMANDS            23
#define COMMAND_EXECUTE     1
#define COMMAND_STATUS      2
#define COMMAND_LIST        3
#define COMMAND_CANCEL      4
#define TRANSPORT_WIFI      1
#define TRANSPORT_BLE       2


class MiletusDeviceCommIf;

struct RequestT
{
  int status;
  MiletusDeviceCommIf * media;
  int commandID;
  std::string commandJson;
};

class MiletusDeviceCommIf
{
 public:
  MiletusDeviceCommIf() {}

  bool initialized;
  std::string commName;

  /* Returns 0 if OK, != 0 otherwise.
   * -1 Unknown command received
   * -2...
   */

  virtual int handleEvent(RequestT *) = 0;
  virtual bool sendJsonToClient(std::string json) = 0;
  virtual bool sendErrorToClient() = 0;

 private:
};

#endif // libMiletus_CommIf_H
