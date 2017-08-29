#!/bin/bash
OPKG_CMD=$(which opkg)

if [ ! -d third-party ]; then
    THIRD_PARTY_ALREADY_EXISTS=false
    echo ">> Creating 'third-party' directory"
    mkdir third-party
else
    THIRD_PARTY_ALREADY_EXISTS=true
fi

echo ">> Entering 'third-party' directory"
cd third-party

if [ ! -d ArduinoJson ]; then
    git clone https://github.com/bblanchon/ArduinoJson.git

    git checkout v5.8.3

    echo ">> Entering 'ArduinoJson' directory"
    cd ArduinoJson

    echo ">> Running CMake"
    cmake .
    if [ $? -ne 0 ]; then
        echo ">> CMake error. Reversing changes!"
        if $THIRD_PARTY_ALREADY_EXISTS; then
            rm -rf ../../third-party/ArduinoJson
        else
            rm -rf ../../third-party
        fi
        exit 1
    fi
    make -j 4

    echo ">> Leaving 'ArduinoJson' directory"
    cd ../
else
    echo ">> The 'ArduinoJson' directory already exists. Moving on..."
fi

echo ">> Leaving 'third-party' directory"
cd ../

echo ">> Recreating bin directory"
rm -rf bin
mkdir bin
cd bin

echo ">> Compiling"
g++ -c -g -Wall -Werror -fpic ../../libMiletus.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic ../linux_wifi.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic ../linux_wrapper.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic ../linux_provider.cpp -std=c++11

g++ -c -g -Wall -Werror -fpic ../../base64.cpp -std=c++11

g++ -g ../example_libMiletusLinux_wifi.cpp *.o -o linux_example_wifi -std=c++11

if [[ ! -z $OPKG_CMD ]]; then
	g++ -g ../example_libMiletus_edison_wifi.cpp *.o -o edison_example_wifi -std=c++11 -lmraa
fi

echo ">> Done!"
