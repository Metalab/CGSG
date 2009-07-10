/*
 test for creating a seperate sdl thread to listen for incoming osc messages, store them and process them in our main loop
*/
#include <SDL.h>

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

typedef deque<osc::ReceivedMessage>::const_iterator oscMsgQ_CI;

int main(int argc, char *argv[])
{
  int oscport = OSC_PORT;
	std::cout << "listening for osc input (but only for string arguments!!!)\n";
  listen_for_osc_packets(oscport);
  bool done= false;
  while (!done) {
    //process OSC messages:
    ///*
    SDL_mutexP(oscMsgQmutex);
    if(oscMsgQ.msgqueue.size() > 0) {
      oscMsgQ_CI msgIter = oscMsgQ.msgqueue.begin();
      for(msgIter = oscMsgQ.msgqueue.begin(); msgIter != oscMsgQ.msgqueue.end(); msgIter++) {
        string addresspattern = msgIter->AddressPattern();
        cout << "addresspattern: " << addresspattern << endl;
        if(msgIter->ArgumentCount() > 0) {
          osc::ReceivedMessage::const_iterator arg_CI = msgIter->ArgumentsBegin();
          for(; arg_CI != msgIter->ArgumentsEnd(); arg_CI++) {
            
            cout << "string arg: " << arg_CI->AsStringUnchecked() << endl;
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

