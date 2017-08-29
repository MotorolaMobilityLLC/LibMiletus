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
#include "libMiletus.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

MiletusProvider *MiletusDevice::provider = new MiletusProvider();

// MILETUSDEVICE CLASS
MiletusDevice::MiletusDevice(const char *_name) : name(_name) {
// TODO: remove this futurely
#ifdef ARDUINO
  Serial.begin(115200);
#endif

  traitsJsonBuffer = new DynamicJsonBuffer;
  infoJsonBuffer = new DynamicJsonBuffer;
}

bool MiletusDevice::addCommInterface(MiletusDeviceCommIf *comm_if, bool def) {
  if (!comm_if->initialized) {
    return false;
  }
  if (comm_if->deviceTransportClass == TransportClass::CLOUD) {
    _buffer = new char[jsonInfo->measureLength() + 1];
    jsonInfo->printTo(_buffer, jsonInfo->measureLength() + 1);
    ((MiletusPubSubIf *)comm_if)->publish("out/info", _buffer, true);
    free(_buffer);
    _buffer = new char[(*jsonTraits).measureLength() + 1];
    (*jsonTraits).printTo(_buffer, (*jsonTraits).measureLength() + 1);
    ((MiletusPubSubIf *)comm_if)->publish("out/traits", _buffer, true);
    free(_buffer);
    ((MiletusPubSubIf *)comm_if)
        ->publish("out/components", this->getComponentsJson().c_str(), true);
  }
  if (def)
    this->comm_ifs.push_front(comm_if);
  else
    this->comm_ifs.push_back(comm_if);
  // default node is always the first in the list
  if (def) {
    // Reset the id of other interfaces
    this->comm_ifs.push_front(comm_if);
  } else {
    this->comm_ifs.push_back(comm_if);
  }
  return true;
}

void MiletusDevice::initJson() {
  traitsJsonBuffer = new DynamicJsonBuffer;
  jsonTraits = &(traitsJsonBuffer->createObject());
  (*jsonTraits)["fingerprint"] = (unsigned int)getFingerprint();
  jsonNestedTraits = &(jsonTraits->createNestedObject("traits"));
  jsonInit = true;
}

void MiletusDevice::merge(JsonObject *dest, JsonObject &src) {
  for (auto kvp : src) {
    (*dest)[kvp.key] = kvp.value;
  }
}

void MiletusDevice::dumpTraits() {
// TODO: Remove the use of Serial
#ifdef ARDUINO
  MiletusDevice::provider->printdbg("JsonTraits:");
  jsonTraits->prettyPrintTo(Serial);
  MiletusDevice::provider->printdbg("JsonNestedTraits:");
  jsonNestedTraits->prettyPrintTo(Serial);
#endif
}

void MiletusDevice::loadJsonTraits(const char *json) {
  if (!jsonInit)
    initJson();
  JsonObject &jsonObjectTrait = traitsJsonBuffer->parseObject(json);
  if (!jsonObjectTrait.success()) {
    MiletusDevice::provider->printdbg(
        "[MiletusDevice::loadJsonTraits] Error parsing JSON trait.");
    return;
  }
  merge(jsonNestedTraits, jsonObjectTrait);
}

bool MiletusDevice::addComponent(Component *comp, list<const char *> entries) {
  int n = entries.size();
  for (int i = 0; i < n; i++) {
    comp->addTrait(entries.back(), this->jsonTraits);
    entries.pop_back();
  }
  components.push_back(comp);
  return true;
}

list<Component *> MiletusDevice::getComponentList() { return this->components; }

void MiletusDevice::handleEvents() {
  list<MiletusDeviceCommIf *>::iterator i = comm_ifs.begin();
  list<MiletusDeviceCommIf *>::iterator ie = comm_ifs.end();
  while (i != ie) {
    MiletusDeviceCommIf *comm = *i;
    if (comm->initialized) {
      RequestT request;
      request.media = *i;
      comm->handleEvent(&request);
      if (request.status >= INFO) {
        processRequest(&request);
      }
    }
    i++;
  }
  if (hasUpdateToBePublished) {
    for (auto j = comm_ifs.begin(); j != comm_ifs.end(); ++j) {
      if (!(*j)->initialized) {
        continue;
      }
      if ((*j)->deviceTransportClass == TransportClass::CLOUD) {
        ((MiletusPubSubIf *)*j)
            ->publish("out/components", this->getComponentsJson().c_str(),
                      true);
      }
    }
    hasUpdateToBePublished = false;
  }
}

// TODO: A new fingerprint shuld be generated for each request
unsigned int MiletusDevice::getFingerprint() { return fingerprint++; }

string MiletusDevice::getComponentsJson() {
  string cjson = "{";
  cjson += "\"fingerprint\":";
  char buffer[8];
  sprintf(buffer, "%d", getFingerprint());
  cjson += buffer;
  cjson += ",";
  std::list<Component *>::iterator i, ie;
  cjson += "\"components\":{";
  for (i = components.begin(), ie = components.end(); i != ie; i++) {
    if (i != components.begin()) {
      cjson += ",";
    }
    cjson += (*i)->getJson();
  }
  cjson += "}}";
  return cjson;
}

void MiletusDevice::processRequest(RequestT *request) {
  string response = "";
  switch (request->status) {
  case INFO:
    MiletusDevice::provider->printdbg("INFO");
    _buffer = new char[jsonInfo->measureLength() + 1];
    jsonInfo->printTo(_buffer, jsonInfo->measureLength() + 1);
    request->media->sendJsonToClient(_buffer);
    free(_buffer);
    break;
  case TRAITS:
    MiletusDevice::provider->printdbg("TRAITS");
    _buffer = new char[(*jsonTraits).measureLength() + 1];
    (*jsonTraits).printTo(_buffer, (*jsonTraits).measureLength() + 1);
    request->media->sendJsonToClient(_buffer);
    free(_buffer);
    break;
  case COMPONENTS:
    MiletusDevice::provider->printdbg("COMPONENTS");
    response = getComponentsJson();
    request->media->sendJsonToClient(response);
    break;
  case COMMANDS:
    MiletusDevice::provider->printdbg("COMMANDS");
    switch (request->commandID) {
    case COMMAND_EXECUTE:
      response = executeCommand(request->commandJson);
      request->media->sendJsonToClient(response);
    case COMMAND_STATUS:
    case COMMAND_LIST:
    case COMMAND_CANCEL:
      break;
    default:
      request->media->sendErrorToClient();
      break;
    }
    break;
  default:
    MiletusDevice::provider->printdbg(
        "[MiletusDevice::processRequest] Error: status unknown.");
  }
}

string MiletusDevice::executeCommand(string jsonCommand) {
  // Find the right command, parse the received JSON and them run.
  // find the right component in the list components
  // break the name in trait.command
  DynamicJsonBuffer jsonBuffer;
  JsonObject &jsonObjectCommand = jsonBuffer.parseObject(jsonCommand.c_str());
  if (!jsonObjectCommand.success()) {
    return Command::displayJsonError(
        "[MiletusDevice::executeCommand] Error parsing JSON command.");
  }
  if (!jsonObjectCommand.containsKey("component")) {
    return Command::displayJsonError(
        "[MiletusDevice::executeCommand] JSON received does not have a "
        "component field.");
  }
  if (!jsonObjectCommand.containsKey("name")) {
    return Command::displayJsonError(
        "[MiletusDevice::executeCommand] JSON received does not have a name "
        "field.");
  }
  string name = (const char *)jsonObjectCommand["name"];
  const char *c;
  unsigned int str_aux;

  // below is a hack so we can avoid the memchr bug
  for (c = name.c_str(), str_aux = 0; str_aux < name.length(); str_aux++) {
    if (*(c + str_aux) == '.')
      break;
  }
  string trait = name.substr(0, str_aux);
  string command = name.substr(str_aux + 1);
  // maybe change components to a std:map
  auto i = components.begin();
  for (; i != components.end(); i++) {
    if (strcmp((*i)->name.c_str(),
               (const char *)jsonObjectCommand["component"]) == 0) {
      break;
    }
  }
  if (i == components.end()) {
    return Command::displayJsonError(
        "[MiletusDevice::executeCommand] Requested component not found.");
  }
  // maybe change commands to a std:map
  auto j = (*i)->traits[trait].commands.begin();
  for (; j != (*i)->traits[trait].commands.end(); j++) {
    if (strcmp((*j).name.c_str(), command.c_str()) == 0) {
      break;
    }
  }
  if (j == (*i)->traits[trait].commands.end()) {
    return Command::displayJsonError(
        "[MiletusDevice::executeCommand] Requested command not found.");
  }
  // TODO: check if all params are required
  if (jsonObjectCommand.containsKey("parameters")) {
    JsonObject &thisCommandParameter = jsonObjectCommand["parameters"];
    for (JsonObject::iterator itParam = thisCommandParameter.begin();
         itParam != thisCommandParameter.end(); ++itParam) {
      if (!(*j).parameters.count(itParam->key)) {
        string error =
            "[MiletusDevice::executeCommand] Parameter does not contain key " +
            std::string(itParam->key) + ".";
        return Command::displayJsonError(error);
      }
      (*j).parameters[itParam->key].currentValue = itParam->value;
    }
  }
  return (*j).run();
}

bool MiletusDevice::updateTransportInfo(char *transport, char *key,
                                        char *value) {
  // TODO: call ant test it!
  if (!(*jsonInfo).containsKey(transport)) {
    // we should rise an error here
    MiletusDevice::provider->printdbg(
        std::string("[MiletusDevice::updateTransportInfo] Wrong transport: " +
                    std::string(transport))
            .c_str());
    return false;
  }
  JsonObject &discoveryTransport = (*jsonInfo)[transport];
  if (!discoveryTransport.containsKey(key)) {
    // we should rise an error here
    MiletusDevice::provider->printdbg(
        std::string(
            "[MiletusDevice::updateTransportInfo] Wrong transport key: " +
            std::string(key))
            .c_str());
    return false;
  }
  discoveryTransport[key] = value;
  return true;
}
void MiletusDevice::createInfo() {
  // DynamicJsonBuffer infoJsonBuffer;
  infoJsonBuffer = new DynamicJsonBuffer;
  jsonInfo = &(infoJsonBuffer->createObject());
  char buffer[8];
  sprintf(buffer, "%08X", MiletusDevice::provider->getRand());
  (*jsonInfo)["Id"] = buffer;
  (*jsonInfo)["Name"] = this->name;
  (*jsonInfo)["Description"] = "";
  (*jsonInfo)["ModelManifestId"] = "LIBMI";
}

void MiletusDevice::setProvider(MiletusProvider *p) {
  delete MiletusDevice::provider;
  MiletusDevice::provider = p;
  this->createInfo();
}

bool MiletusDevice::setState(const char *component, const char *trait,
                             const char *state, JsonVariant value) {
  std::list<Component *>::iterator i, ie;
  for (i = components.begin(), ie = components.end(); i != ie; i++) {
    if (strcmp((*i)->name.c_str(), component) == 0)
      hasUpdateToBePublished = true;
    return (*i)->setState(trait, state, value);
  }
  return false;
}

bool MiletusDevice::setStateCallback(const char *component, const char *trait,
                                     const char *state,
                                     bool (*f)(const char *)) {
  std::list<Component *>::iterator i, ie;
  for (i = components.begin(), ie = components.end(); i != ie; i++) {
    if (strcmp((*i)->name.c_str(), component) == 0)
      return (*i)->setStateCallback(trait, state, f);
  }
  return false;
}

// COMMAND CLASS
bool Command::appendResult(string key, JsonVariant value) {
  if (!responseJsonInitialized) {
    createJsonResponse();
    resultsJson = &responseJson->createNestedObject("results");
  }
  // TODO: Keep this value in memmory till this command die;
  (*resultsJson)[key.c_str()] = value;
  return true;
}

void Command::createJsonResponse() {
  responseJson = &(responseJsonBuffer->createObject());
  (*responseJson)["state"] = "inProgress";
  responseJsonInitialized = true;
}

void Command::createJsonSuccess() {
  if (!responseJsonInitialized) {
    createJsonResponse();
  }
  (*responseJson)["state"] = "done";
}

string Command::getResponse() {
  if (!responseJsonInitialized) {
    createJsonResponse();
  }
  string response;
#ifdef ARDUINO
  String responseA;
  responseJson->printTo(responseA);
  response = responseA.c_str();
#else
  responseJson->printTo(response);
#endif
  return response;
}

string Command::displayJsonError(string error) {
  MiletusDevice::provider->printdbg(
      std::string("[Command::displayJsonError] Request error: " + error + ".")
          .c_str());
  char *jsonResponse;
  DynamicJsonBuffer jsonBuffer;
  JsonObject &jsonObjectError = jsonBuffer.createObject();
  jsonObjectError["state"] = "error";
  ((JsonObject &)jsonObjectError.createNestedObject("error"))["code"] =
      error.c_str();
  jsonResponse = new char[jsonObjectError.measureLength() + 1];
  jsonObjectError.printTo(jsonResponse, jsonObjectError.measureLength() + 1);
  string jerror(jsonResponse);
  free(jsonResponse);
  return jerror;
}

void Command::createJsonError(const char *error) {
  if (!responseJsonInitialized) {
    createJsonResponse();
  }
  MiletusDevice::provider->printdbg(
      "[Command::createJsonError] Request error: ");
  MiletusDevice::provider->printdbg(error);
  (*responseJson)["state"] = "error";
  ((JsonObject &)(*responseJson).createNestedObject("error"))["code"] = error;
  Serial.printf("Error: %s\n", error);
}

void Command::setParentComponent(Component *component) {
  this->parentComponent = component;
}

void Command::setUserFunction(bool (*f)(Command &)) { this->userFunction = f; }

string Command::run() {
  responseJsonBuffer = new DynamicJsonBuffer();
  if (this->userFunction == 0) {
    this->createJsonError("Attempt to run a non-initialized command.");
    MiletusDevice::provider->printdbg(
        "[Command::run] Attempt to run a non-initialized command: ");
    MiletusDevice::provider->printdbg(this->name.c_str());
    return this->getResponse();
  }
  Serial.printf("Going to execute command at addr %x\n", this->userFunction);
  if (this->userFunction(*this)) {
    createJsonSuccess();
  }
  return this->getResponse();
}

void Command::abort(const char *errorMessage) {
  this->createJsonError(errorMessage);
}

string Command::getJson() {
  string json = "{\"";
  json += name;
  json += "\": {\"minimalRole\": \"";
  switch (role) {
  case viewer:
    json += "viewer\",";
    break;
  case user:
    json += "user\",";
    break;
  case manager:
    json += "manager\",";
    break;
  case owner:
    json += "owner\",";
    break;
  case robot:
    json += "robot\",";
    break;
  default:
    json += "unknown\",";
    break;
  }
  json += "\"initialized\":";
  json += this->userFunction == 0 ? "true," : "false,";
  if (parameters.size() > 0) {
    json += "\"parameters\": {";
    for (auto i = parameters.begin(); i != parameters.end(); i++) {
      json += "\"";
      json += i->first;
      json += "\": {\"type\": \"";
      switch (i->second.type) {
      case miBoolean:
        json += "boolean\"";
        break;
      case miNumber:
        json += "number\"";
        break;
      case miString:
        json += "string\"";
        break;
      default:
        json += "unknown\"";
        break;
      }
      if (i->second.possibleValues.size() > 0) {
        json += ",\"values\": ";
        auto j = i->second.possibleValues.begin();
        for (; j != i->second.possibleValues.end(); j++) {
          if (j != i->second.possibleValues.begin())
            json += " ,";
          json += "\"";
          json += *j;
          json += "\"";
        }
      }
    }
    json += "}";
  }
  json += "}";
  return json;
}

JsonVariant Command::getParameter(string paramName) {
  return this->parameters[paramName].currentValue;
}

// COMPONENT CLASS
void Component::addTrait(const char *trait, JsonObject *jsonTraits) {
  JsonObject &traitObj = (*jsonTraits)["traits"];
  if (!traitObj.containsKey(trait)) {
    // we should rise an error here
    MiletusDevice::provider->printdbg(
        std::string("[Component::addTrait] Trait does not exist: " +
                    std::string(trait) + ".")
            .c_str());
    return;
  }
  // TODO: remove new
  Trait *myTrait = new Trait();
  JsonObject &selectedTrait = traitObj[trait];
  if (selectedTrait.containsKey("commands")) {
    // There is a command in this trait
    JsonObject &traitCommands = selectedTrait["commands"];
    // traitCommands.prettyPrintTo(Serial);
    for (JsonObject::iterator itCommand = traitCommands.begin();
         itCommand != traitCommands.end(); ++itCommand) {
      Command *myCommand = new Command(itCommand->key);

      JsonObject &selectedCommand = traitCommands[itCommand->key];
      if (selectedCommand.containsKey("parameters")) {
        JsonObject &thisCommandParameter = selectedCommand["parameters"];
        for (JsonObject::iterator itParam = thisCommandParameter.begin();
             itParam != thisCommandParameter.end(); ++itParam) {
          Parameter *thisParam = new Parameter();
          JsonObject &selectedCommandParameter =
              thisCommandParameter[itParam->key];
          if (!selectedCommandParameter.containsKey("type")) {
            // rise an error
          }
          const char *parameterType = selectedCommandParameter["type"];
          thisParam->type = Type::getTypeFromChar(parameterType);
          // Serial.print("Type: ");
          // Serial.println(thisParam->type);
          if (selectedCommandParameter.containsKey("enum")) {
            JsonObject &thisParamEnum = selectedCommandParameter["enum"];
            for (JsonObject::iterator itParamEnum = thisParamEnum.begin();
                 itParamEnum != thisParamEnum.end(); ++itParamEnum) {
              thisParam->possibleValues.push_back(
                  itParamEnum->value.as<char *>());
            }
          }
          myCommand->parameters[itParam->key] = *thisParam;
          delete thisParam;
        }
      }
      myCommand->setParentComponent(this);
      myTrait->commands.push_back(*myCommand);
      delete myCommand;
    }
  }
  if (selectedTrait.containsKey("state")) {
    JsonObject &selectedTraitState = selectedTrait["state"];

    // selectedTraitState.prettyPrintTo(Serial);
    for (auto itState = selectedTraitState.begin();
         itState != selectedTraitState.end(); ++itState) {
      State *myState = new State();
      myState->name = (const char *)itState->key;
      JsonObject &selectedStateParameter = selectedTraitState[itState->key];

      if (!selectedStateParameter.containsKey("type")) {
        // TODO: rise an error
        MiletusDevice::provider->printdbg(
            std::string("[Component::addTrait] " + std::string(itState->key) +
                        " state does not contain type field.")
                .c_str());
      }
      const char *stateType = selectedStateParameter["type"];
      myState->type = Type::getTypeFromChar(stateType);
      // Serial.print("Type: ");
      // Serial.println(myState->type);
      if (selectedStateParameter.containsKey("enum")) {
        JsonObject &thisStateEnum = selectedStateParameter["enum"];
        for (auto itParamEnum = thisStateEnum.begin();
             itParamEnum != thisStateEnum.end(); ++itParamEnum) {
          myState->possibleValues.push_back(itParamEnum->value.as<char *>());
        }
      }
      if (selectedStateParameter.containsKey("isRequired")) {
        myState->isRequired = selectedStateParameter["isRequired"];
      } else {
        myState->isRequired = false;
      }
      if (selectedStateParameter.containsKey("minimum")) {
        myState->minimum = selectedStateParameter["minimum"];
      }
      if (selectedStateParameter.containsKey("maximum")) {
        myState->maximum = selectedStateParameter["maximum"];
      }
      if (selectedStateParameter.containsKey("unity")) {
        myState->unity = (const char *)selectedStateParameter["unity"];
      }
      myState->setParentComponent(this);
      myTrait->states[itState->key] = *myState;
      delete myState;
    }
  }
  traits[trait] = *myTrait;
  delete myTrait;
}

bool Component::setCommand(const char *trait, const char *cmd,
                           bool (*f)(Command &)) {
  std::map<string, Trait>::iterator i = this->traits.find(trait);
  if (i == traits.end()) {
    MiletusDevice::provider->printdbg(
        std::string("[Component::setCommand] Error finding trait: " +
                    std::string(trait) + ".")
            .c_str());
    return false;
  }
  std::list<Command> *cmds = &this->traits[trait].commands;
  std::list<Command>::iterator ic, ice;
  for (ic = cmds->begin(), ice = cmds->end(); ic != ice; ic++) {
    if (ic->name.compare(cmd) == 0) {
      ic->setUserFunction(f);
      return true;
    }
  }
  MiletusDevice::provider->printdbg(
      std::string("[Component::setCommand] Error finding command: " +
                  std::string(cmd) + ".")
          .c_str());
  return false;
}

State *Component::getState(const char *trait, const char *state) {
  std::map<string, Trait>::iterator i = this->traits.find(trait);
  if (i == traits.end()) {
    MiletusDevice::provider->printdbg(
        std::string("[Component::getState] Error finding trait: " +
                    std::string(trait) + ".")
            .c_str());
    return NULL;
  }
  std::map<string, State>::iterator ii = traits[trait].states.find(state);
  if (ii == traits[trait].states.end()) {
    MiletusDevice::provider->printdbg(
        std::string("[Component::getState] Error finding state: " +
                    std::string(state) + ".")
            .c_str());
    return NULL;
  }
  return &traits[trait].states[state];
}

bool Component::setStateCallback(const char *trait, const char *state,
                                 bool (*f)(const char *)) {
  std::map<string, Trait>::iterator i = this->traits.find(trait);
  if (i == traits.end()) {
    MiletusDevice::provider->printdbg(
        std::string("[Component::setStateCallback] Error finding trait: " +
                    std::string(trait) + ".")
            .c_str());
    return false;
  }
  std::map<string, State>::iterator ii = traits[trait].states.find(state);
  if (ii == traits[trait].states.end()) {
    MiletusDevice::provider->printdbg(
        std::string("[Component::setStateCallback] Error finding state: " +
                    std::string(state) + ".")
            .c_str());
    return false;
  }
  traits[trait].states[state].callback = f;
  return true;
}

bool Component::setState(const char *trait, const char *state,
                         JsonVariant value) {
  std::map<string, Trait>::iterator i = this->traits.find(trait);
  if (i == traits.end()) {
    MiletusDevice::provider->printdbg(
        std::string("[Component::setState] Error finding trait: " +
                    std::string(trait) + ".")
            .c_str());
    return false;
  }
  std::map<string, State>::iterator ii = traits[trait].states.find(state);
  if (ii == traits[trait].states.end()) {
    MiletusDevice::provider->printdbg(
        std::string("[Component::setState] Error finding state: " +
                    std::string(state) + ".")
            .c_str());
    return false;
  }
  traits[trait].states[state].value = value;
  if (traits[trait].states[state].callback != 0) {
#ifdef ARDUINO
    String component_trait_state_value;
#else
    string component_trait_state_value;
#endif
    component_trait_state_value = this->name.c_str();
    component_trait_state_value += ".";
    component_trait_state_value += trait;
    component_trait_state_value += ".";
    component_trait_state_value += state;
    component_trait_state_value += "=";
#ifdef ARDUINO
    component_trait_state_value += value.as<String>().c_str();
#else
    component_trait_state_value += value.as<string>();
#endif
    traits[trait].states[state].callback(component_trait_state_value.c_str());
  }
  return true;
}

string Component::getJson() {
  string json = "";

  // json += "{\n\"";
  json += "\"";
  json += this->name;
  json += "\":{";
  json += "\"traits\":[";
  std::map<string, Trait>::iterator i = traits.begin();
  for (; i != traits.end(); i++) {
    if (i != traits.begin()) {
      json += ",";
    }
    json += "\"";
    json += i->first;
    json += "\"";
  }
  json += "],";
  json += "\"state\": {";
  i = traits.begin();
  for (; i != traits.end(); i++) {
    if (i != traits.begin()) {
      json += ",";
    }
    json += "\"";
    json += i->first;
    json += "\":{";
    json += i->second.getJson();
  }
  json += "}}";
  return json;
}

// STATE CLASS
void State::setParentComponent(Component *component) {
  this->parentComponent = component;
}

string State::getJson() {
  string json = "{";
  json += "\"value\": ";
  if (type == miBoolean) {
    json += value ? "true" : "false";
  } else {
    json += "\"";
#ifdef ARDUINO
    json += value.as<String>().c_str();
#else
    json += value.as<string>();
#endif
    json += "\"";
  }
  json += ",";
  if (possibleValues.size() > 0) {
    json += "\"values\": ";
    std::list<string>::iterator i = possibleValues.begin();
    for (; i != possibleValues.end(); i++) {
      if (i != possibleValues.begin())
        json += " ,";
      json += "\"";
      json += *i;
      json += "\"";
    }
  }
  if (isRequired) {
    json += "\"isRequired\": true,";
  } else {
    json += "\"isRequired\": false,";
  }
  switch (type) {
  case miBoolean:
    json += "\"type\": \"boolean\",";
    break;
  case miNumber:
    json += "\"type\": \"number\",";
    break;
  case miString:
    json += "\"type\": \"string\",";
    break;
  default:
    json += "\"type\": \"unknown\",";
    break;
  }

  json += "\"maximum\": ";
#ifdef ARDUINO
  String buffer;
#else
  std::string buffer;
#endif

  size_t n;

  n = maximum.printTo(buffer);

  if (n == 0)
    json += "\"\",";
  else
    json += buffer.c_str();

  json += "\"minimum\": ";

  n = minimum.printTo(buffer);

  if (n == 0)
    json += "\"\",";
  else
    json += buffer.c_str();

  json += "\"unity\": ";

  if (unity.empty())
    json += "\"\"";
  else
    json += unity;

  json += "}";

  return json;
}

// TYPE CLASS
miletusType Type::getTypeFromChar(const char *type) {
  if (strcmp(type, "boolean") == 0) {
    return miBoolean;
  }
  if (strcmp(type, "number") == 0) {
    return miNumber;
  }
  if (strcmp(type, "string") == 0) {
    return miString;
  }
  if (strcmp(type, "integer") == 0) {
    return miInteger;
  }
  if (strcmp(type, "object") == 0) {
    return miObject;
  }
  return miUnknown;
}

// TRAIT CLASS
string Trait::getJson() {
  string json = "";
  std::map<string, State>::iterator j = states.begin();
  while (j != states.end()) {
    json += "\"";
    json += j->first;
    json += "\": ";
    if (j->second.type == miBoolean) {
      json += j->second.value ? "true" : "false";
    } else {
      json += "\"";
#ifdef ARDUINO
      json += j->second.value.as<String>().c_str();
#else
      json += j->second.value.as<string>();
#endif
      json += "\"";
    }
    j++;
    if (j != states.end())
      json += ",";
  }
  json += "}";
  return json;
}
