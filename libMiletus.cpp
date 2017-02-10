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

MiletusProvider nullProvider;

MiletusProvider* MiletusDevice::provider = &nullProvider;


MiletusDevice::MiletusDevice(string name){
  // remove this futurely
#ifdef ARDUINO
  Serial.begin(115200);
#endif
  this->name = name;
}

bool MiletusDevice::addCommInterface(MiletusDeviceCommIf* comm_if, bool def){
  // default node is always the first in the list
  if(def) this->comm_ifs.push_front(comm_if);
  else this->comm_ifs.push_back(comm_if);

  return true;
}

void MiletusDevice::initJson(){
  jsonTraits = &(jsonBuffer.createObject());
  (*jsonTraits)["fingerprint"] = FINGERPRINT;
  jsonNestedTraits = &(jsonTraits->createNestedObject("traits"));
  jsonInit = true;
}

void MiletusDevice::merge(JsonObject* dest, JsonObject& src){
  for (auto kvp : src) {
    (*dest)[kvp.key] = kvp.value;
  }
}

void MiletusDevice::dumpTraits(){
  //TODO: Remove the use of Serial
#ifdef ARDUINO
  provider->printdbg("JsonTraits:");
  jsonTraits->prettyPrintTo(Serial);
  provider->printdbg("JsonNestedTraits:");
  jsonNestedTraits->prettyPrintTo(Serial);
#endif
}

void MiletusDevice::loadJsonTraits(const char* json){
  if(!jsonInit) initJson();
  JsonObject& jsonObjectTrait = jsonBuffer.parseObject(json);
  if (!jsonObjectTrait.success()){
    provider->printdbg(
      "Error parsing JSON trait in MiletusDevice::loadJsonTraits");
    return;
  }
  merge(jsonNestedTraits, jsonObjectTrait);
}

bool MiletusDevice::addComponent(Component* comp, list<const char *> entries){
  int n = entries.size();
  for(int i = 0; i < n; i++){
    comp->addTrait(entries.back(), jsonTraits);
    entries.pop_back();
  }
  components.push_back(comp);
  return true;
}

list<Component*> MiletusDevice::getComponentList(){
  return this->components;
}

void MiletusDevice::handleEvents(){
  list<MiletusDeviceCommIf*>::iterator i = comm_ifs.begin();
  list<MiletusDeviceCommIf*>::iterator ie = comm_ifs.end();

  while(i != ie){
    MiletusDeviceCommIf *comm = *i;
    if(comm->initialized){
      RequestT request;
      request.media = *i;
      comm->handleEvent(&request);
      if(request.status >= INFO){
        processRequest(&request);
      }
    }
    i++;
  }
}

string MiletusDevice::getComponentsJson(){
  string cjson = "{";
  cjson +=  "\"fingerprint\":";
  cjson +=  FINGERPRINT;
  cjson +=  ",";
  std::list<Component*>::iterator i, ie;
  cjson +=  "\"components\":{\n";
  for(i = components.begin(), ie = components.end(); i != ie; i++){
    if (i != components.begin()){
      Serial.print(",");
      cjson += ",";
    }
    cjson += (*i)->getJson();
  }
  cjson +=  "\n}\n}";
  return cjson;
}

void MiletusDevice::processRequest(RequestT *request){
  string response = "";
  switch(request->status){
    case INFO:
      provider->printdbg("INFO");
      _buffer = new char [jsonInfo->measureLength()+1];
      jsonInfo->printTo(_buffer, jsonInfo->measureLength()+1);
      request->media->sendJsonToClient(_buffer);
      free(_buffer);
      break;
    case TRAITS:
      provider->printdbg("TRAITS");
      _buffer = new char [(*jsonTraits).measureLength()+1];
      (*jsonTraits).printTo(_buffer, (*jsonTraits).measureLength()+1);
      request->media->sendJsonToClient(_buffer);
      free(_buffer);
      break;
    case COMPONENTS:
      provider->printdbg("COMPONENTS");
      response = getComponentsJson();
      request->media->sendJsonToClient(response);
      break;
    case COMMANDS:
      provider->printdbg("COMMANDS");
      switch(request->commandID){
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
      provider->printdbg("Error, status unknown - processRequest");
  }
}

string MiletusDevice::executeCommand(string jsonCommand){
  // Find the right command, parse the received JSON and them run.
  // find the right component in the list components
  // break the name in trait.command
  DynamicJsonBuffer jsonBuffer;
  JsonObject& jsonObjectCommand = jsonBuffer.parseObject(jsonCommand.c_str());
  if (!jsonObjectCommand.success()){
    return Command::createJsonError("Error parsing JSON command");
  }
  if (!jsonObjectCommand.containsKey("component")){
    return
      Command::createJsonError("JSON received does not have a component field");
  }
  if (!jsonObjectCommand.containsKey("name")){
    return Command::createJsonError("JSON received does not have a name field");
  }
  string name = (const char *) jsonObjectCommand["name"];
  const char* c;
  unsigned int str_aux;

  // below is a hack so we can avoid the memchr bug
  for(c = name.c_str(), str_aux = 0; str_aux < name.length(); str_aux++){
    if(*(c+str_aux) == '.') break;
  }
  string trait = name.substr(0,str_aux);
  string command = name.substr(str_aux+1);
  //maybe change components to a std:map
  auto i = components.begin();
  for(; i != components.end(); i++){
    if (strcmp((*i)->name.c_str(),
      (const char *) jsonObjectCommand["component"]) == 0){
      break;
    }
  }
  if(i == components.end()){
    return Command::createJsonError("Requested component not found");
  }
  //maybe change commands to a std:map
  auto j = (*i)->traits[trait].commands.begin();
  for(; j != (*i)->traits[trait].commands.end(); j++){
    if (strcmp((*j).name.c_str(), command.c_str()) == 0){
      break;
    }
  }
  if(j == (*i)->traits[trait].commands.end()){
    return Command::createJsonError("Requested command not found");
  }
  // TODO: check if all params are required
  if(jsonObjectCommand.containsKey("parameters")){
    JsonObject& thisCommandParameter = jsonObjectCommand["parameters"];
    for(JsonObject::iterator itParam = thisCommandParameter.begin();
        itParam != thisCommandParameter.end(); ++itParam){
      if (!(*j).parameters.count(itParam->key)){
        string error = "Parameter not contains key ";
        error += itParam->key;
        return Command::createJsonError(error);
      }
      (*j).parameters[itParam->key].currentValue = itParam->value;
    }
  }
  
  if ((*j).run()){
    return (*j).createJsonSuccess();
  }
  return (*j).getResponse();
}


bool Command::appendResult(string key, JsonVariant value){
  if (!responseJsonInitialized){
    createJsonResponse();
    resultsJson = &responseJson->createNestedObject("results");
  }
  // TODO: Keep this value in memmory till this command die;
  (*resultsJson)[key.c_str()] = value;
  return true;
}

void Command::createJsonResponse(){
  responseJson = &(jsonBuffer.createObject());
  (*responseJson)["state"] = "inProgress";
  responseJsonInitialized = true;
}

string Command::createJsonSuccess(){
  if (!responseJsonInitialized){
    createJsonResponse();
  }
  (*responseJson)["state"] = "done";
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

void MiletusDevice::createInfo(string deviceName){
  std::string tmp = R"({
 		"AccountId": "",
 		"Id": "",
 		"Name": "",
 		"Description": "",
 		"ModelManifestId": "LIBMI",
 		"UiDeviceKind": "vendor",
 		"Location": "",
 		"LocalId": "",
 		"DiscoveryTransport": {
 			"hasCloud": false,
 			"hasWifi": false,
 			"hasLan": true,
 			"hasBle": false,
 			"LanTransport": {
 				"ConnectionStatus": "online",
 				"HttpAddress": "192.168.0.1",
 				"HttpInfoPort": 8080,
 				"HttpPort": 8081,
 				"HttpUpdatesPort": 8082,
 				"HttpsPort": 8083,
 				"HttpsUpdatesPort": 8084
 			},
			"CloudTransport": {
 				"ConnectionStatus": "",
 				"Id": ""
 			},
			"WifiTransport": {
 				"ConnectionStatus": "",
 				"Ssid": ""
 			},
 			"BleTransport": {
 				"ConnectionStatus": "",
 				"Address": ""
 			}
 		}
 	})";
  jsonInfoDefs = tmp.c_str();
  jsonInfo = &(infoJsonBuffer.parseObject(jsonInfoDefs));
  if (!jsonInfo->success()){
    provider->printdbg("parseObject() in MiletusDevice failed");
    return;
  }
  (*jsonInfo)["Name"] = deviceName.c_str();
  // The divice Id is set before a provider is defined
  // TODO: Get the ID from a persistent memory
  // The divice Id is set before a provider is defined
  char buffer [10];
  sprintf(buffer, "%04x", provider->getRand());
  (*jsonInfo)["Id"] = buffer;
}

void MiletusDevice::setProvider(MiletusProvider* p){
  this->provider = p;
  createInfo(this->name);
}

bool MiletusDevice::setState(const char* component, const char* trait,
                             const char* states, JsonVariant value){
  std::list<Component*>::iterator i, ie;
  for(i = components.begin(), ie = components.end(); i != ie; i++){
    if(strcmp((*i)->name.c_str(), component)==0)
      return (*i)->setState(trait, states, value);
  }
  return false;
}

// COMPONENT CLASS
void Component::addTrait(const char* trait, JsonObject* jsonTraits){
  JsonObject& traitObj = (*jsonTraits)["traits"];
  if (!traitObj.containsKey(trait)){
    // we should rise an error here
    MiletusDevice::provider->printdbg("Trait does not exist: ");
    MiletusDevice::provider->printdbg(trait);
    return;
  }
  // TODO: remove new
  Trait* myTrait = new Trait();
  JsonObject& selectedTrait = traitObj[trait];
  if (selectedTrait.containsKey("commands")){
    // There is a command in this trait
    JsonObject& traitCommands = selectedTrait["commands"];
    // traitCommands.prettyPrintTo(Serial);
    for(JsonObject::iterator itCommand = traitCommands.begin();
        itCommand != traitCommands.end(); ++itCommand){
      Command* myCommand = new Command(itCommand->key);

      JsonObject& selectedCommand = traitCommands[itCommand->key];
      if(selectedCommand.containsKey("parameters")){
        JsonObject& thisCommandParameter = selectedCommand["parameters"];
        for(JsonObject::iterator itParam = thisCommandParameter.begin();
            itParam != thisCommandParameter.end(); ++itParam){
          Parameter* thisParam = new Parameter();
          JsonObject& selectedCommandParameter =
            thisCommandParameter[itParam->key];
          if(!selectedCommandParameter.containsKey("type")){
            // rise an error
          }
          const char * parameterType = selectedCommandParameter["type"];
          thisParam->type = Type::getTypeFromChar(parameterType);
          // Serial.print("Type: ");
          // Serial.println(thisParam->type);
          if(selectedCommandParameter.containsKey("enum")){
            JsonObject& thisParamEnum = selectedCommandParameter["enum"];
            for(JsonObject::iterator itParamEnum = thisParamEnum.begin();
                itParamEnum != thisParamEnum.end(); ++itParamEnum){
              thisParam->values.push_back(itParamEnum->value.as<char*>());
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
  if (selectedTrait.containsKey("state")){
    JsonObject& selectedTraitState = selectedTrait["state"];
    // selectedTraitState.prettyPrintTo(Serial);
    for(auto itState = selectedTraitState.begin();
        itState != selectedTraitState.end(); ++itState){
      State * myState = new State();
      myState->name = (const char * ) itState->key;
      JsonObject& selectedStateParameter = selectedTraitState[itState->key];
      if(!selectedStateParameter.containsKey("type")){
        // TODO: rise an error
        MiletusDevice::provider->printdbg(itState->key);
        MiletusDevice::provider->printdbg("state does not contains type field");
      }
      const char * stateType = selectedStateParameter["type"];
      myState->type = Type::getTypeFromChar(stateType);
      // Serial.print("Type: ");
      // Serial.println(myState->type);
      if(selectedStateParameter.containsKey("enum")){
        JsonObject& thisStateEnum = selectedStateParameter["enum"];
        for(auto itParamEnum = thisStateEnum.begin();
            itParamEnum != thisStateEnum.end(); ++itParamEnum){
          myState->values.push_back(itParamEnum->value.as<char*>());
        }
      }
      if(selectedStateParameter.containsKey("isRequired")){
        myState->isRequired = selectedStateParameter["isRequired"];
      }else{
        myState->isRequired = false;
      }
      if(selectedStateParameter.containsKey("minimum")){
        myState->minimum = selectedStateParameter["minimum"];
      }
      if(selectedStateParameter.containsKey("maximum")){
        myState->maximum = selectedStateParameter["maximum"];
      }
      myState->setParentComponent(this);
      myTrait->states[itState->key] = *myState;
      delete myState;
    }
  }
  traits[trait] = *myTrait;
  delete myTrait;
}

bool Component::setCommand(const char* trait, const char* cmd,
                           bool (*f)(Command&)){
  std::map<string, Trait>::iterator i = this->traits.find(trait);
  if(i == traits.end()){
    MiletusDevice::provider->printdbg("Error finding trait");
    MiletusDevice::provider->printdbg(trait);
    return false;
  }
  std::list<Command> * cmds = &this->traits[trait].commands;
  std::list<Command>::iterator ic, ice;
  for(ic = cmds->begin(), ice = cmds->end(); ic != ice; ic++){
    if(ic->name.compare(cmd)==0){
      ic->setUserFunction(f);
      return true;
    }
  }
  MiletusDevice::provider->printdbg("Error finding command");
  MiletusDevice::provider->printdbg(cmd);
  return false;
}

void Command::setParentComponent(Component * component){
  this->parentComponent = component;
}

void State::setParentComponent(Component * component){
  this->parentComponent = component;
}

void Command::setUserFunction(bool (*f)(Command&)){
  this->userFunction = f;
  this->initialized = true;
}

bool Command::run(){
  if(!initialized){
    this->createJsonError("Attempt to run a non-initialized command");
    MiletusDevice::provider->printdbg(this->name);
    return false;
  }
  return this->userFunction(*this);
}

void Command::abort(string errorMessage){
  this->response = this->createJsonError(errorMessage);
}

miletusType Type::getTypeFromChar(const char * type){
/*  Serial.println("Comparing types: ");
  Serial.println(type);
  Serial.println(strcmp(type, "boolean"));
  Serial.println(strcmp(type, "number"));
  Serial.println(strcmp(type, "string"));
  Serial.println(strcmp(type, "integer"));
  Serial.println(strcmp(type, "object"));*/

  if(strcmp(type, "boolean") == 0){
    return miBoolean;
  }
  if(strcmp(type, "number") == 0){
    return miNumber;
  }
  if(strcmp(type, "string") == 0){
    return miString;
  }
  if(strcmp(type, "integer") == 0){
    return miInteger;
  }
  if(strcmp(type, "object") == 0){
    return miObject;
  }
  return miUnknown;
}

string Command::getJson(){
  string json = "{\n\"";
  json += name;
  json += "\": {\n\"minimalRole\": \"";
  switch(role){
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
  json += "\n\"initialized\":";
  json += initialized? "true,": "false,";
  if(parameters.size() > 0){
    json += "\n\"parameters\": {";
    for(auto i = parameters.begin(); i != parameters.end(); i++){
      json += "\n\"";
      json += i->first;
      json += "\": {\n\"type\": \"";
      switch(i->second.type){
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
      if(i->second.values.size() > 0){
        json += ",\n\"values\": ";
        auto j = i-> second.values.begin();
        for(; j != i->second.values.end(); j++){
          if(j != i->second.values.begin()) json += " ,";
          json += "\"";
          json += *j;
          json += "\"";
        }
      }
    }
    json += "\n}\n";
  }
  json += "}\n";
  return json;
}

string State::getJson(){
  string json = "{";
  json += "\n\"value\": ";
  if(value == 0) {
    json += "\"null\"";
  } else{
#ifdef ARDUINO
    json += value.as<String>().c_str();
#else
    json += value.as<string>();
#endif
  }
  json += ",";
  if(values.size() > 0){
    json += "\n\"values\": ";
    std::list<string>::iterator i = values.begin();
    for(; i != values.end(); i++){
      if(i != values.begin()) json += " ,";
      json += "\"";
      json += *i;
      json += "\"";
    }
  }
  if(isRequired){
    json += "\n\"isRequired\": true,";
  } else {
    json += "\n\"isRequired\": false,";
  }
  switch(type){
    case miBoolean:
      json += "\n\"type\": \"boolean\",";
      break;
    case miNumber:
      json += "\n\"type\": \"number\",";
      break;
    case miString:
      json += "\n\"type\": \"string\",";
      break;
    default:
      json += "\n\"type\": \"unknown\",";
      break;
  }
  json += "\n\"maximum\": ";
  char buffer [50];
  int n = sprintf (buffer, "%d", maximum);
  // string m = MiletusDevice::provider->itos(maximum);
  if(n == 0) json+= "\"\",";
  else json += buffer;
  json += "\n\"minimum\": ";
  n = sprintf (buffer, "%d", minimum);
  // m = MiletusDevice::provider->itos(minimum);
  if(n == 0) json+= "\"\"";
  else json += buffer;
  json += "\n}";
  return json;
}

string Trait::getJson(){
  string json = "";
  std::map<string, State>::iterator j = states.begin();
  while(j != states.end()){
    json += "\"";
    json += j->first;
    json += "\": \n";
    if(j->second.value == 0) {
      json += "\"null\"";
    } else{
#ifdef ARDUINO
      json += j->second.value.as<String>().c_str();
#else
      json += j->second.value.as<string>();
#endif
    }
    j++;
    if(j != states.end()) json += ",\n";
  }
  json += "\n}";
  return json;
}

State* Component::getState(const char* trait, const char* state){
  std::map<string, Trait>::iterator i = this->traits.find(trait);
  if(i == traits.end()){
    MiletusDevice::provider->printdbg("Error finding trait on getState");
    MiletusDevice::provider->printdbg(trait);
    return NULL;
  }
  std::map<string, State>::iterator ii = traits[trait].states.find(state);
  if(ii == traits[trait].states.end()){
    MiletusDevice::provider->printdbg("Error finding state on getState");
    MiletusDevice::provider->printdbg(state);
    return NULL;
  }
  return &traits[trait].states[state];
}

bool Component::setStateCallback(const char* trait, const char* state,
                                 bool (*f)(void)){
  std::map<string, Trait>::iterator i = this->traits.find(trait);
  if(i == traits.end()){
    MiletusDevice::provider->printdbg("Error finding trait on getState");
    MiletusDevice::provider->printdbg(trait);
    return false;
  }
  std::map<string, State>::iterator ii = traits[trait].states.find(state);
  if(ii == traits[trait].states.end()){
    MiletusDevice::provider->printdbg("Error finding state on getState");
    MiletusDevice::provider->printdbg(state);
    return false;
  }
  traits[trait].states[state].callback = f;
  return true;
}

bool Component::setState(const char* trait, const char* state,
                         JsonVariant value){
  std::map<string, Trait>::iterator i = this->traits.find(trait);
  if(i == traits.end()){
    MiletusDevice::provider->printdbg("Error finding trait on getState");
    MiletusDevice::provider->printdbg(trait);
    return false;
  }
  std::map<string, State>::iterator ii = traits[trait].states.find(state);
  if(ii == traits[trait].states.end()){
    MiletusDevice::provider->printdbg("Error finding state on getState");
    MiletusDevice::provider->printdbg(state);
    return false;
  }
  traits[trait].states[state].value = value;
  if(traits[trait].states[state].callback != NULL){
    if(! traits[trait].states[state].callback()) return false;
  }
  return true;
}

string Component::getJson(){    
  string json = "";

  // json += "{\n\"";
  json += "\"";
  json += this->name;
  json += "\":\n{";
  json += "\"traits\":\n[";
  std::map<string, Trait>::iterator i = traits.begin();
  for(; i != traits.end(); i++){
    if (i != traits.begin()){
      json += ",";
    }
    json += "\"";
    json += i->first;
    json += "\"";
  }
  json += "\n],\n";
  json += "\"state\": {";
  i = traits.begin();
  for(; i != traits.end(); i++){
    if (i != traits.begin()){
      json += ",";
    }
    json += "\n\"";
    json += i->first;
    json += "\":\n{";
    json += i->second.getJson();
  }
  json += "\n}\n";
  json += "}\n";
  return json;
}

string Command::getResponse(){
  return this->response;
}

string Command::createJsonError(string error){
  MiletusDevice::provider->printdbg("Request error: ");
  MiletusDevice::provider->printdbg(error);
  char * jsonResponse;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& jsonObjectError = jsonBuffer.createObject();
  jsonObjectError["state"] = "error";
  ((JsonObject& )jsonObjectError
   .createNestedObject("error"))["code"] = error.c_str();
  jsonResponse = new char [jsonObjectError.measureLength()+1];
  jsonObjectError.printTo(jsonResponse, jsonObjectError.measureLength()+1);
  string jerror(jsonResponse);
  free(jsonResponse);
  return jerror;
}

JsonVariant Command::getParameter(string paramName){
  return parameters[paramName].currentValue;
}
