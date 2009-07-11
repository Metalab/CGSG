#include <string>
#include <deque>
#include <vector>
#include "osc/OscReceivedElements.h"
#include <sstream>
#include <iostream>

using namespace std;

class OSCMsgQueue {
  public:
    deque<osc::ReceivedMessage*> msgqueue;
    
    OSCMsgQueue();
    int storeMsg(const osc::ReceivedMessage& m);
    int size();
  private:
    
};