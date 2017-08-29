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
#ifndef LIB_MILETUS_PROVIDER_H
#define LIB_MILETUS_PROVIDER_H

#include <stdio.h>
#include <string.h> // Needed by ArduinoJson DummyPrint.
#include <string>

#define DEBUG 1

#define STRSIZE 20

using namespace std;

/*
 * This class devides the hardware specific features required to
 * support the libMiletus device operation.
 */
class MiletusProvider {
public:
  MiletusProvider(){};

  virtual ~MiletusProvider(){};

  /*
   * Prints a message for debugging.
   */
  virtual void printdbg(const char *msg) {
    if (DEBUG)
      printf("%s\n", msg);
  }
  /*
   * Prints the amount of free memory on the heap.
   */
  virtual void printFreeHeap() {}
  /*
   * Returns a pseudo random number.
   */
  virtual uint32_t getRand() { return 0; }
};

#endif // LIB_MILETUS_PROVIDER_H