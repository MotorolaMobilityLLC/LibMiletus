/*************************************************************************
The MIT License (MIT)
Copyright (c) 2017 Jeferson Rech Brunetta -- ra161253@students.ic.unicamp.br
Copyright (c) 2017 Edson Borin -- edson@ic.unicamp.br

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
#ifndef ARDUINO_MQTT_H
#define ARDUINO_MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <libMiletusPubSubIf.h>

#ifndef MAX_RECONNECTION_ATTEMPTS
#define MAX_RECONNECTION_ATTEMPTS 5
#endif

class arduino_mqtt : public MiletusPubSubIf {
public:
  arduino_mqtt(Client &_network_client, const char *mqtt_server, int mqtt_port,
               const char *device_topic_name, const char *mqtt_username);
  ~arduino_mqtt(){};
  int handleEvent(RequestT *);
  bool sendJsonToClient(std::string json);
  bool sendErrorToClient();
  bool publish(const char *topic, const char *json, bool retained = 0);
  void pubsub_callback(char *topic, uint8_t *payload, unsigned int length);
  static arduino_mqtt *callback_obj;

private:
  bool connect();
  bool reconnect();
  PubSubClient pubsub_client;
  const char *device_topic_name;
  const char *mqtt_username;
  const char *mqtt_server;
  int mqtt_port;
  RequestT *current_request;
};

#endif // ARDUINO_MQTT_H