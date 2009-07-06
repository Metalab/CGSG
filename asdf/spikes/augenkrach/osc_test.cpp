/*
 osc test
*/
#include <sdlvu.h> // will also include GL headers
#include "map_augenkrach.h"

#include <vector>
#include <algorithm>


#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include <limits.h>
#include <float.h>


//Open Sound Control osc stuff
#include "oscReceive.h"
#define OSC_PORT 2345
extern ExamplePacketListener listener;
#include <deque>
#include <string>

// our queue for osc messages
#include "oscmsgqueue.h"
OSCMsgQueue oscMsgQ;
SDL_mutex *oscMsgQmutex;

using namespace std;

typedef deque<OSCMsg_t>::const_iterator oscMsgQ_CI;

int main(int argc, char *argv[])
{
  int oscport = OSC_PORT;
	std::cout << "listening for osc input \n";
  listen_for_osc_packets(oscport);
  bool done= false;
  while (!done) {
    //process OSC messages:
    ///*
    SDL_mutexP(oscMsgQmutex);
    if(oscMsgQ.msgqueue.size() > 0) {
      oscMsgQ_CI msgIter = oscMsgQ.msgqueue.begin();
      for(msgIter = oscMsgQ.msgqueue.begin(); msgIter != oscMsgQ.msgqueue.end(); msgIter++) {
        string addresspattern = msgIter->addresspattern;
        cout << "addresspattern: " << addresspattern << endl;
        if(msgIter->arguments.size() > 0) {
          stringvector::const_iterator arg_CI = msgIter->arguments.begin();
          for(arg_CI = msgIter->arguments.begin(); arg_CI < msgIter->arguments.end(); arg_CI++) {
            cout << "arg: " << *arg_CI << endl;
          }
        }
      }
      //clear all msgs
      oscMsgQ.msgqueue.clear();
    }
    SDL_mutexV(oscMsgQmutex);
    //*/ OSC msg processing
  }
  return 0;
}

