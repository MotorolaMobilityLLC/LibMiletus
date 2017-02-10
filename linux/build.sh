mkdir ./lib
cd lib
git clone https://github.com/bblanchon/ArduinoJson.git
cd ArduinoJson
cmake .
make
cd ../../
g++ -c -g -Wall -Werror -fpic ../libMiletus.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic linux_wifi.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic linux_wrapper.cpp -std=c++11
g++ -c -g -Wall -Werror -fpic linux_provider.cpp -std=c++11
g++ -g example_libMiletusLinus.cpp libMiletus.o linux_wifi.o linux_wrapper.o linux_provider.o -o linux_example -std=c++11
