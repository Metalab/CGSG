#include <string>
#include <deque>
#include <vector>
#include "osc/OscReceivedElements.h"
#include <sstream>
#include <iostream>

using namespace std;

typedef std::vector<string> stringvector;

typedef struct {
  string addresspattern;
  stringvector arguments;
} OSCMsg_t;

class OSCMsgQueue {
  public:
    deque<OSCMsg_t> msgqueue;
    
    OSCMsgQueue();
    int storeMsg(osc::ReceivedMessage m);
    int size();
  private:
    
};