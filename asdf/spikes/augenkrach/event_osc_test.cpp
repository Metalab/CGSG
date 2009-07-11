/*
 test for handling osc messages/events and call functions if msg matching certain pattern arrives
 
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

#include "asdf_eventhandler.h"


using namespace std;
using namespace osc;


int lolfunction(osc::ReceivedMessage& oscmsg) {
  cout << "lolfunction called with msgAddressPattern: " << oscmsg.AddressPattern() << endl;
}

float blafunction(osc::ReceivedMessage& oscmsg) {
  cout << "blafunction called with msgAddressPattern: " << oscmsg.AddressPattern() << endl;
  cout << "args: ";
  try {
  osc::ReceivedMessage::const_iterator arg = oscmsg.ArgumentsBegin();
  osc::ReceivedMessage::const_iterator argend = oscmsg.ArgumentsEnd();
  for(;arg != argend; arg++) {
    cout << arg->AsString() << " " << endl;
  }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
}
class foobar {
  public:
    int trallafitti();
};

int  foobar::trallafitti() {
  cout << "trallafitti" << endl;
}
//test class for asdf to pass pointer 2 member functions to the event handler
//black magic here:
// extends class asdfEventHandler. so we can pass function/member functions of this class
// to  registerEvent() & registerEvent_memberfunc(),
// which will get called if an osc msg arrives matching the registered address pattern
class asdf : public asdfEventHandler, public foobar {
  public:
    asdf();
    
    void selectcamera(int camno);
    void osc_selectcamera(osc::ReceivedMessage& oscmsg);
    
  private:
  };

asdf::asdf() {
  
  //register a non-member or function of anyclass or a static function of a class:
  registerEvent("/asdf/lolfunction", (void *)lolfunction);
  registerEvent("/asdf/blafunction", (void *)blafunction);
  
  //register a memberfunction of class asdf:
  registerEvent_memberfunc("/asdf/camera/select", (void(asdfEventHandler::*)(osc::ReceivedMessage&))&asdf::osc_selectcamera);
}


void asdf::selectcamera(int camno) {
  cout << "selectcamera called with arg: " << camno << endl;
}

void asdf::osc_selectcamera(osc::ReceivedMessage& oscmsg) {
  cout << "osc_selectcamera called with msgAddressPattern: " << oscmsg.AddressPattern() << endl;
  cout << "args: ";
  try {
    osc::ReceivedMessage::const_iterator arg = oscmsg.ArgumentsBegin();
    osc::ReceivedMessage::const_iterator argend = oscmsg.ArgumentsEnd();
    if(arg->IsInt32()) {
      selectcamera(arg->AsInt32());
    }
    for(;arg != argend; arg++) {
      cout << arg->AsString() << " " << endl;
    }
    
    
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
}

typedef deque<osc::ReceivedMessage*>::const_iterator oscMsgQ_CI;

int main(int argc, char *argv[])
{
  //starts osc msg listener in its own thread(maybe shaky yet)
  listen_for_osc_packets(OSC_PORT);
  
  //create object which inherits the superduper hakish eventhandler for oscmsg's
  //god(john carmack) knows what happens if one creates 2 of those
  asdf myASDF;
  
  std::cout << "listening for osc input on port " << OSC_PORT << endl;
  bool done=false;
  while (!done) {
    //process OSC messages:
    SDL_mutexP(oscMsgQmutex);
    if(oscMsgQ.msgqueue.size() > 0) {
      oscMsgQ_CI msgIter = oscMsgQ.msgqueue.begin();
      for(msgIter = oscMsgQ.msgqueue.begin(); msgIter != oscMsgQ.msgqueue.end(); msgIter++) {
        //try execute a registered func for msg
        myASDF.execute(*msgIter);
        (*msgIter)->freeCopiedMessageBuffer();
        delete *msgIter;
      }
      //clear all msgs
      oscMsgQ.msgqueue.clear();
    }
    SDL_mutexV(oscMsgQmutex);
    // OSC msg processing
  }
  return 0;
}

