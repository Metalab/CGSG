
#include "oscmsgqueue.h"


using namespace std;

OSCMsgQueue::OSCMsgQueue() {}

int OSCMsgQueue::size() {
  return msgqueue.size();
}

int OSCMsgQueue::storeMsg(osc::ReceivedMessage m) {
  msgqueue.push_back(m);
}