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
receives commands to switch the NodeMCU builtin LED. You must adjust
the code to reflect the setting of your wifi network (Hint: look for
MODIFY strings).

*************************************************************************/
#include "linux_wifi.h"

class MiletusDeviceCommIf;

linux_wifi::linux_wifi () {
  //TODO: Start mDNS

  // Start TCP (HTTP) server
  server.begin();

  initialized = true;

  // TODO: Is a name useful??
  commName = "linux";
}

/*
POST /commands/execute HTTP/1.1
Host: 127.0.0.1:8000
User-Agent: curl/7.47.0
Accept: * / *
Content-Type: application/json
Content-Length: 46

{"name":"lamp.toggleLED","component":"sensor"}*/

/*
GET /components HTTP/1.1
Host: 127.0.0.1:8000
User-Agent: curl/7.47.0
Accept: * / *
*/

int linux_wifi::handleEvent(RequestT * request){
  fprintf(stderr, ".");
  client = server.available();
  if(!client.available()){
    request->status = NOCLIENT;
    return 1;
  }
  string msg = client.readString();
  fprintf(stderr, "MSG: %s", msg.c_str());
  

  size_t get_pos = msg.find("GET /");
  size_t post_pos = msg.find("POST /");
  if (get_pos != string::npos){
    // It is a GET message
    if (msg.find("info", get_pos+5) != string::npos){
      request->status = INFO;
      return 0;
    }
    if (msg.find("traits", get_pos+5) != string::npos){
      request->status = TRAITS;
      return 0;
    }
    if (msg.find("components", get_pos+5) != string::npos){
      request->status = COMPONENTS;
      return 0;
    }
  }
  else if(post_pos != string::npos){
    // It is a POST message
    if (msg.find("application/json", get_pos+5) == string::npos){
      request->status = UNKNOWN;
      return -1;
    }
    msg = msg.substr(post_pos+5);
    size_t cmd_pos = msg.find(" ");
    string url = msg.substr(0, cmd_pos);
    
    if (url.find("commands") != string::npos){
      if (url.find("commands/execute") != string::npos){
        request->commandID = COMMAND_EXECUTE;
      }
      else if (url.find("commands/status") != string::npos){
        request->commandID = COMMAND_STATUS;
      }
      else if (url.find("commands/list") != string::npos){
        request->commandID = COMMAND_LIST;
      }
      else if (url.find("commands/cancel") != string::npos){
        request->commandID = COMMAND_CANCEL;
      }else{
        request->status = UNKNOWN;
        return -1;
      }
      size_t boddy_pos = msg.find("Content-Length");
      boddy_pos = msg.find("\r\n\r\n", boddy_pos);
      request->commandJson = msg.substr(boddy_pos);
      request->status = COMMANDS;
      return 0;
    }
  }
  request->status = UNKNOWN;
  return -1;
}

bool linux_wifi::sendJsonToClient(std::string json){
  client.print(HTTP_HEADER);
  client.print(json.c_str());
  client.stop();
  return true;
}

bool linux_wifi::sendErrorToClient(){
  client.print(HTTP_HEADER_ERROR);
  client.stop();
  return true;
}
