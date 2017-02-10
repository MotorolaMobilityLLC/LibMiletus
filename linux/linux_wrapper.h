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

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <strings.h>

class LinuxClient;
class LinuxServer;

class LinuxServer{

private:
	uint16_t port;
	struct sockaddr_in addr;
	int socket_fd;
        int current_status = 0;

public:
	LinuxServer(uint16_t port){
		this->port = port;
	};

	LinuxClient available();
	bool begin();
	//size_t write(uint8_t);
	//size_t write(const uint8_t *buf, size_t size);
	uint8_t status();
	void stop();
};

class LinuxClient{

private:
	uint16_t rport;
	uint16_t lport;
	int client_fd;

public:
	LinuxClient(){};
	LinuxClient(int client_fd){
		this->client_fd = client_fd;
	};

	uint8_t status();
	//size_t write(uint8_t);
	//size_t write(const uint8_t *buf, size_t size);
        void print(std::string msg);
	int available();
        std::string readString();
        std::string extractHTTPCmd(std::string msg);
	//int readStr(uint8_t *buf, size_t size);
	//std::string readStringUntil(char c);
	void flush();
	void stop();
	uint8_t connected();
};
