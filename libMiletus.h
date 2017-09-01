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
#ifndef LIB_MILETUS_H
#define LIB_MILETUS_H

#include "libMiletusCommIf.h"
#include "libMiletusProvider.h"
#include "libMiletusPubSubIf.h"

#include <list>
#include <map>
#include <string>

#ifdef ARDUINO
#include <ArduinoJson.h>
#else
#include "linux/third-party/ArduinoJson/ArduinoJson.h"
#endif

using namespace std;

enum miletusType {
  miBoolean,
  miNumber,
  miString,
  miInteger,
  miObject,
  miUnknown
};
enum miletusRole { viewer, user, manager, owner, robot };

class Type;
class Parameter;
class Command;
class State;
class Trait;
class Component;
class MiletusDevice;

class Type {
public:
  static miletusType getTypeFromChar(const char *type);
};

class Parameter {
public:
  Parameter(){};
  string name; // TODO: Remove it
  miletusType type;
  bool isRequired;
  JsonVariant currentValue;
  list<string> possibleValues;
};

class Command {
public:
  std::map<string, Parameter> parameters;
  string name;

  Command(){};
  Command(string name) { this->name = name; };
  string getJson();
  string run(); // Returns true/false; error captured through getJsonResponse();
  // bool parse(string Command);
  void setUserFunction(bool (*f)(Command &command));
  string getResponse();
  static string displayJsonError(string error);
  void createJsonError(const char *error);
  void createJsonSuccess();
  void createJsonResponse();

  string getName();
  void setParentComponent(Component *component);

  list<string> getArgumentList();
  string getArgument(int pos);
  bool appendResult(string key, JsonVariant value);
  void abort(const char *error);

  JsonVariant getParameter(string paramName);

private:
  miletusRole role;
  string targetComponent;
  bool responseJsonInitialized = false;
  Component *parentComponent;
  bool (*userFunction)(Command &command);
  JsonObject *responseJson;
  JsonObject *resultsJson;

  DynamicJsonBuffer *responseJsonBuffer;
};

class State {
public:
  bool isRequired;
  miletusType type;
  JsonVariant minimum;
  JsonVariant maximum;
  string unity;
  list<string> possibleValues;
  JsonVariant value;
  string name;
  bool (*callback)(const char *component_trait_state_value);

  void setParentComponent(Component *component);
  string getJson();
  State() { this->callback = NULL; };

private:
  Component *parentComponent;
};

class Trait {
public:
  // string name; // is it requeired?
  // Trait(string name){this->name = name;};
  Trait(){};
  std::map<string, State> states;
  std::list<Command> commands;
  string getJson();
};

class Component {
public:
  std::map<string, Trait> traits;
  string name;
  Component() { this->name = ""; };
  Component(string name) { this->name = name; }
  State *getState(const char *trait, const char *state);
  bool setState(const char *trait, const char *state, JsonVariant value);
  bool setStateCallback(const char *trait, const char *state,
                        bool (*f)(const char *component_trait_state_value));
  bool setCommand(const char *trait, const char *cmd,
                  bool (*f)(Command &command));
  void addTrait(const char *trait, JsonObject *jsonTraits);
  string getJson();
};

class MiletusDevice {
public:
  // create a function fo find a componet/trate/state by it name
  static MiletusProvider *provider;

  MiletusDevice(const char *_name);

  bool addCommInterface(MiletusDeviceCommIf *comm_if, bool def = true);
  void loadJsonTraits(const char *json);
  bool addComponent(Component *comp, list<const char *> entries);
  list<Component *> getComponentList();
  void createInfo();
  string getComponentsJson();

  string executeCommand(string jsonCommand);
  void handleEvents();
  void processRequest(RequestT *request);
  void dumpTraits();
  bool setState(const char *componentName, const char *traitName,
                const char *stateName, JsonVariant value);
  bool setStateCallback(const char *componentName, const char *traitName,
                        const char *stateName,
                        bool (*f)(const char *traitName));
  bool publishStateUpdate(MiletusDeviceCommIf *commIf);
  /*
   * When this function is called, the device ID is set if it is zero.
   */
  void setProvider(MiletusProvider *p);
  unsigned int getFingerprint();

  DynamicJsonBuffer *getCommandJsonBuffer();

  // bool addProvider(MiletusProvider* p) {provider = p;}
private:
  bool updateTransportInfo(char *transport, char *key, char *value);

  list<Component *> components;
  list<MiletusDeviceCommIf *> comm_ifs;

  std::map<string, string> settings;
  std::map<string, void (*)(void)> setting_cbs;

  bool jsonInit = false;

  void merge(JsonObject *dest, JsonObject &src);
  void initJson();

  char *_buffer = NULL;

  int fingerprint = 0;
  bool hasUpdateToBePublished = false;
  const char *name;
  JsonObject *jsonTraits;
  JsonObject *jsonNestedTraits;
  JsonObject *jsonInfo;
  DynamicJsonBuffer *traitsJsonBuffer;
  DynamicJsonBuffer *infoJsonBuffer;
};

#endif // LIB_MILETUS_H
