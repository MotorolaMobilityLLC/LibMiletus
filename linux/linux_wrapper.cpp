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

#include "linux_wrapper.h"

bool LinuxServer::begin(){
  this->current_status = 0;
  this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(this->socket_fd < 0){
    fprintf(stderr, "Error creating socket on Server\n");
    return false;
  }

  fcntl(this->socket_fd, F_SETFL, O_NONBLOCK);
  bzero((char *) &addr, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(this->port);

  fprintf(stderr, "Listening on port: %d\n", this->port);

  if(bind(this->socket_fd, (struct sockaddr *) &addr, sizeof(addr))<0){
    fprintf(stderr, "Error binding socket on Server\n");
    return false;
  }
  listen(this->socket_fd, 7);
  this->current_status = 1;
  return true;
}

LinuxClient LinuxServer::available(){
  int receiver;
  struct sockaddr_in client;

  socklen_t client_length = sizeof(client);
  receiver = accept(socket_fd, (struct sockaddr *) &client, &client_length);
    if(receiver > 0){
      return LinuxClient(receiver);
    }
  return LinuxClient(0);
}

void LinuxServer::stop(){
  close(this->socket_fd);
  this->current_status = 0;
}

uint8_t LinuxServer::status(){
  return this->current_status;
}

// CLIENT
std::string LinuxClient::readString(){
  int len;
  char buffer[256];
  len = recv(this->client_fd, buffer, (size_t) 255, 0);
  if(len < 0){
    fprintf(stderr, "Error reading from client\n");
    return NULL;
  }
  std::string msg = buffer;
  return msg;
}

std::string LinuxClient::extractHTTPCmd(std::string msg){
  size_t get_pos = msg.find("GET /");
  msg = msg.substr(get_pos+5);
  size_t cmd_pos = msg.find(" ");
  msg = msg.substr(0, cmd_pos);
  return msg;
}

void LinuxClient::print(std::string msg){
  send(this->client_fd, msg.c_str(), (size_t) msg.length(), 0);
}

int LinuxClient::available(){
  return this->client_fd;
}

void LinuxClient::flush(){
  shutdown(client_fd, SHUT_WR);
}

void LinuxClient::stop(){
  close(client_fd);
}
