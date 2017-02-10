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

#include "linux_wrapper.h"

int main(){
  LinuxServer s(8000);
  s.begin();
  while(true){
    fprintf(stderr, ".");
    LinuxClient c = s.available();
    if(c.available()){
      std::string msg = c.extractHTTPCmd(c.readString());
      fprintf(stderr, "MSG: %s", msg.c_str());
      if(msg.compare("traits")==0){
        c.print("uhuuuuuull\n");
      } else {
        c.print("command not found dude!\n");
        fprintf(stderr, "we got a weird command: %s", msg.c_str());
      }
      c.stop();
    }
    sleep(1);
  }
}
