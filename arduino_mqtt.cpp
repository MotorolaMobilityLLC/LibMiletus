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
#include <arduino_mqtt.h>

// #define DEBUG

#define ERR_MSG(...)                                                           \
  do {                                                                         \
    Serial.print("ERROR: ");                                                   \
    Serial.println(__VA_ARGS__);                                               \
  } while (0)

#ifdef DEBUG
#define DBG_MSG(...)                                                           \
  do {                                                                         \
    Serial.print(__VA_ARGS__);                                                 \
  } while (0)
#define DBG_LNMSG(...)                                                         \
  do {                                                                         \
    Serial.println(__VA_ARGS__);                                               \
  } while (0)
#else
#define DBG_MSG(...)
#define DBG_LNMSG(...)
#endif

arduino_mqtt *arduino_mqtt::callback_obj = 0;

void callback1(char *topic, uint8_t *payload, unsigned int length) {
  if (!arduino_mqtt::callback_obj) {
    ERR_MSG(
        "arduino_mqtt:: callback1 function cannot be invoked. callback_obj == "
        "NULL!");
    return;
  }
  arduino_mqtt::callback_obj->pubsub_callback(topic, payload, length);
}

arduino_mqtt::arduino_mqtt(Client &_network_client, const char *_mqtt_server,
                           int _mqtt_port, const char *_device_topic_name,
                           const char *_mqtt_username)
    : pubsub_client(_network_client), device_topic_name(_device_topic_name),
      mqtt_username(_mqtt_username), mqtt_server(_mqtt_server),
      mqtt_port(_mqtt_port) {
  // initialized = true;
  deviceTransportClass = TransportClass::CLOUD;
  pubsub_client.setServer(_mqtt_server, _mqtt_port);
  pubsub_client.setCallback(callback1);
  arduino_mqtt::callback_obj = this;
  initialized = this->connect();
  if (initialized) {
    DBG_LNMSG("MQTT initialized!");
  } else {
    ERR_MSG("MQTT initialization fail!");
  }
}

void arduino_mqtt::pubsub_callback(char *topic, uint8_t *payload,
                                   unsigned int length) {
  // ONLY FOR COMMAND REQUESTS
  current_request->status = UNKNOWN;
  if (strstr(topic, "/commands") != NULL) {
    /* TODO: Find a better way to do this. */
    String req = (const char *)payload;
    DBG_MSG("Topic: ");
    DBG_LNMSG(topic);
    DBG_LNMSG("Request: ");
    DBG_LNMSG(req);
    char *packageBody = new char[req.length() + 1];
    req.toCharArray(packageBody, req.length() + 1);
    current_request->commandJson = packageBody;
    delete[] packageBody;
    current_request->status = COMMANDS;
    if (strstr(topic, "/commands/execute") != NULL) {
      current_request->commandID = COMMAND_EXECUTE;
      DBG_LNMSG("arduino_mqtt::pubsub_callback Execute Command");
    } else if (strstr(topic, "/commands/status") != NULL) {
      current_request->commandID = COMMAND_STATUS;
      DBG_LNMSG("arduino_mqtt::pubsub_callback Command Status");
    } else if (strstr(topic, "/commands/list") != NULL) {
      current_request->commandID = COMMAND_LIST;
      DBG_LNMSG("arduino_mqtt::pubsub_callback Command List");
    } else if (strstr(topic, "/commands/cancel") != NULL) {
      current_request->commandID = COMMAND_CANCEL;
      DBG_LNMSG("arduino_mqtt::pubsub_callback Command Cancel");
    } else {
      DBG_LNMSG("Unkonwn commandID");
      current_request->status = UNKNOWN;
    }
  }
}

bool arduino_mqtt::connect() {
  // Loop until we're reconnected
  for (int i = 0; !pubsub_client.connected() && i < MAX_RECONNECTION_ATTEMPTS;
       i++) {
    // Attempt to connect
    if (!pubsub_client.connect(mqtt_username)) {
      DBG_LNMSG("arduino_mqtt::connect(mqtt_username) failed with code = ");
      DBG_LNMSG(pubsub_client.state());
      /* Wait for a while. */
      delay(100);
      continue;
    }
  }
  /* Connected OK. Try to subscribe in commands topic. */
  if (!pubsub_client.subscribe(
          ((String)(String(device_topic_name) + "/in/commands/#")).c_str())) {
    ERR_MSG("arduino_mqtt::reconnecte() Managed to reconnect but could not "
            "subscribe to topic.");
    return false;
  }
  return true;
}

bool arduino_mqtt::reconnect() {
  // Loop until we're reconnected\
    // Attempt to connect
  if (!pubsub_client.connect(mqtt_username)) {
    DBG_LNMSG("arduino_mqtt::connect(mqtt_username) failed with code = ");
    DBG_LNMSG(pubsub_client.state());
    /* Wait for a while. */
  }
  /* Connected OK. Try to subscribe in commands topic. */
  if (!pubsub_client.subscribe(
          ((String)(String(device_topic_name) + "/in/commands/#")).c_str())) {
    ERR_MSG("arduino_mqtt::reconnecte() Managed to reconnect but could not "
            "subscribe to topic.");
    return false;
  }
  return true;
}

int arduino_mqtt::handleEvent(RequestT *request) {
  current_request = request;
  request->status = NOCLIENT;
  if (!pubsub_client.connected()) {
    reconnect();
  }

  pubsub_client.loop();
  return request->status;
}

bool arduino_mqtt::sendJsonToClient(std::string json) {
  return this->publish("out/commands/execute", json.c_str(), true);
}

bool arduino_mqtt::publish(const char *subTopic, const char *json,
                           bool retained) {
  DBG_LNMSG("Publish message: ");
  DBG_LNMSG(json);
  String topic = device_topic_name;
  topic += "/";
  topic += subTopic;
  DBG_MSG("**topic: ");
  DBG_LNMSG(topic);
  if (!pubsub_client.connected()) {
    if (!reconnect()) {
      return false;
    }
  }
  pubsub_client.loop();
  pubsub_client.publish(topic.c_str(), (const uint8_t *)json, strlen(json),
                        retained);
  return true;
}

bool arduino_mqtt::sendErrorToClient()
// DEPRECATED
{
  pubsub_client.publish(device_topic_name, "ERROR");
  DBG_LNMSG("arduino_mqtt::sendErroToClient()");
}
