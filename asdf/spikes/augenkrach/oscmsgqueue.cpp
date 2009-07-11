
#include "oscmsgqueue.h"


using namespace std;

OSCMsgQueue::OSCMsgQueue() {}

int OSCMsgQueue::size() {
  return msgqueue.size();
}

int OSCMsgQueue::storeMsg(const osc::ReceivedMessage& m) {
  cout << "Storing msg with AddressPattern: " << m.AddressPattern() << endl;
  
  deque<osc::ReceivedMessage*>::iterator msgIter = msgqueue.begin();
  for(; msgIter != msgqueue.end(); msgIter++) {
    //osc::ReceivedMessage tmpMSG = *msgIter;
    //cout << "msgQueue: " << tmpMSG.AddressPattern() << endl;
    cout << "preStore msgQueue: " << (*msgIter)->AddressPattern() << endl;
  }
  
  osc::ReceivedMessage *tmpMSG = new osc::ReceivedMessage(m, 23);
  msgqueue.push_back(tmpMSG);
  
  msgIter = msgqueue.begin();
  for(; msgIter != msgqueue.end(); msgIter++) {
    //osc::ReceivedMessage tmpMSG = *msgIter;
    //cout << "msgQueue: " << tmpMSG.AddressPattern() << endl;
    cout << "msgQueue: " << (*msgIter)->AddressPattern() << endl;
  }
}