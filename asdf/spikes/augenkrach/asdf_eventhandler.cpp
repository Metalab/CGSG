
#include "asdf_eventhandler.h"
#include <iostream>

using namespace std;

//konschtruktor
asdfEventHandler::asdfEventHandler() {
  
}


int asdfEventHandler::registerEvent(string pattern, void *func) {
  ASDFevent ev;
  ev.addresspattern = pattern;
  ev.funcp = (void(*)(osc::ReceivedMessage&))func;
  ev.memberfuncp = NULL;    //this NULL pointer is used to check which type of call to make (memberfunction or not)
  registeredEvents.push_back(ev);
}

int asdfEventHandler::registerEvent_memberfunc(string pattern, void(asdfEventHandler::*memberfunc)(osc::ReceivedMessage&)) {
  ASDFevent ev;
  ev.addresspattern = pattern;
  ev.funcp = NULL;    //this NULL pointer is used to check which type of call to make (memberfunction or not)
  ev.memberfuncp = (void(asdfEventHandler::*)(osc::ReceivedMessage&))memberfunc;
  registeredEvents.push_back(ev);
}

int asdfEventHandler::execute(osc::ReceivedMessage& oscmsg) {
  //lookup registered event
  cout << "registered patterns: " << registeredEvents.size() << endl;
  vector<ASDFevent>::iterator revs_I = registeredEvents.begin();
  cout << "msgPattern: " << oscmsg.AddressPattern() << endl;
  while(revs_I != registeredEvents.end()) {
    cout << "registered pattern: " << revs_I->addresspattern << endl;
    if(revs_I->addresspattern.compare(oscmsg.AddressPattern()) == 0) {
      cout << "FOUND pattern" << endl;
      //check pointer type:
      if(revs_I->funcp == NULL) {
        //call to member function
        void(asdfEventHandler::*mfptr)(osc::ReceivedMessage&) = revs_I->memberfuncp;
        (this->*mfptr)(oscmsg);
      } else if (revs_I->memberfuncp == NULL) {
        //call nonmember/static member function:
        revs_I->funcp(oscmsg);
      }
      break;
    }
    ++revs_I;
  }
}




