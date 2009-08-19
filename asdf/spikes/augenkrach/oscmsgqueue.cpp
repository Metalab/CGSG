
#include "oscmsgqueue.h"


using namespace std;

OSCMsgQueue::OSCMsgQueue() {}

int OSCMsgQueue::size() {
  return msgqueue.size();
}

int OSCMsgQueue::storeMsg(const osc::ReceivedMessage& m) {
  //cout << "Storing msg with AddressPattern: " << m.AddressPattern() << endl;
  osc::ReceivedMessage *tmpMSG = new osc::ReceivedMessage(m, 23);
  msgqueue.push_back(tmpMSG);
}