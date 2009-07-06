
#include "oscmsgqueue.h"


using namespace std;

OSCMsgQueue::OSCMsgQueue() {
  
}

int OSCMsgQueue::size() {
  return msgqueue.size();
}

int OSCMsgQueue::storeMsg(osc::ReceivedMessage m) {
  OSCMsg_t oscmsg;
  oscmsg.addresspattern = m.AddressPattern();
  osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
  osc::ReceivedMessage::const_iterator argend = m.ArgumentsEnd();
  
  for (;arg != argend; arg++) {
    if( arg->IsBool() ) {
        std::stringstream sstr;
        sstr << (arg)->AsBoolUnchecked();
        std::string str1 = sstr.str();
        oscmsg.arguments.push_back(str1);
        //cout << "argB: " << str1 << endl;
    }else if( arg->IsInt32() ) {
        std::stringstream sstr;
        sstr << (arg)->AsInt32Unchecked();
        std::string str1 = sstr.str();
        oscmsg.arguments.push_back(str1);
        //cout << "argI32: " << str1 << endl;
    }else if( arg->IsFloat() ) {
        std::stringstream sstr;
        sstr << (arg)->AsFloatUnchecked();
        std::string str1 = sstr.str();
        oscmsg.arguments.push_back(str1);
        //cout << "argF: " << str1 << endl;
    }else if( arg->IsString() ) {
        const char *aS = (arg)->AsStringUnchecked();
        oscmsg.arguments.push_back(aS);
        //cout << "argS: " << aS << endl;
    }else {
      //const char *aU = "unsupported argument type";
      //oscQ.push_back("ASDF_OSC_FAIL");
      //cout << "arg: " << aU << endl;
    }
  }
  
  msgqueue.push_back(oscmsg);
}