#!/bin/bash
OPKG_CMD=$(which opkg)

mkdir third-party
cd third-party
git clone https://github.com/bblanchon/ArduinoJson.git
cd ArduinoJson
git checkout v5.8.0
cmake .
make
cd ../../
mkdir bin
cd bin
g++ -c -g -Wall -Werror -fpic ../../libMiletus.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic ../linux_wifi.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic ../linux_wrapper.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic ../linux_provider.cpp -std=c++11
g++ -g ../example_libMiletusLinux_wifi.cpp libMiletus.o linux_wifi.o linux_wrapper.o linux_provider.o -o linux_example_wifi -std=c++11
if [[ ! -z $OPKG_CMD ]]; then
	g++ -g ../example_libMiletus_edison_wifi.cpp libMiletus.o linux_wifi.o linux_wrapper.o linux_provider.o -o edison_example_wifi -std=c++11 -lmraa
fi