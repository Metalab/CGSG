/*
  class that provides list of registered events & and functionality to execute them according to arguments&types
    ^^ well _should_ be able to :)
*/
#include <string>
#include <vector>
#include "osc/OscReceivedElements.h"

using namespace std;
class asdfEventHandler;

typedef struct {
  string addresspattern;
  std::vector<string> argtypes;
  //void (*funcp)(...);
  void(*funcp)(osc::ReceivedMessage&);
  void(asdfEventHandler::*memberfuncp)(osc::ReceivedMessage&);
} ASDFevent;

class asdfEventHandler {
  public:
    std::vector<ASDFevent> registeredEvents;
    
    asdfEventHandler();
    
    int execute(osc::ReceivedMessage *oscmsg);
    int registerEvent(string pattern, void* func);
    int registerEvent_memberfunc(string pattern, void(asdfEventHandler::*memberfunc)(osc::ReceivedMessage&));
  private:
    
    
};