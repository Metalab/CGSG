/* 
    Example of two different ways to process received OSC messages using oscpack.
    Receives the messages from the SimpleSend.cpp example.
*/

#include <iostream>

#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "ip/UdpSocket.h"

#include "oscReceive.h"

#define PORT 7000
#define OSC_ADDR_PATTERN "/asdf"

#include <SDL_thread.h>
#include <deque>
#include <string>
#include <sstream>
#include "oscmsgqueue.h"

using namespace std;

//global osc msg double ended queue "deck"
extern deque<string> oscQ;
extern SDL_mutex *oscMsgQmutex;
extern OSCMsgQueue oscMsgQ;


ExamplePacketListener listener;

int listen_for_osc_packets(int port) {
  oscMsgQmutex = SDL_CreateMutex();
  SDL_CreateThread((int (*)(void *))osc_thread_main, (void*)port);
}
  
int osc_thread_main(void* tport) {
  int port = (int)tport;
  listener.address_pattern2match = OSC_ADDR_PATTERN;
  listener.run_osc_test(port);
}



int ExamplePacketListener::run_osc_test(int port)
{
    cout << "Listening for osc messages on port" << port << endl;
    UdpListeningReceiveSocket s(
            IpEndpointName( IpEndpointName::ANY_ADDRESS, port ),
            this );
                  
    //s.RunUntilSigInt();
    s.Run();
    cout << endl;
    cout << "LISTENER is stopping" << endl;
    cout << endl;
    return 0;
}


void ExamplePacketListener::ProcessMessage( const osc::ReceivedMessage& m, 
		const IpEndpointName& remoteEndpoint )
{
    try{
        // example of parsing single messages. osc::OsckPacketListener
        // handles the bundle traversal.
        //cout << "address pattern: " << m.AddressPattern() << endl;
        string addresspattern = m.AddressPattern();
        
        if(addresspattern.length() > address_pattern2match.length()-1) {
          string asdfpattern = addresspattern.substr(0,5);
          if(asdfpattern.compare(address_pattern2match) == 0) {
            //cout << "ASDF addresspattern: " << m.AddressPattern() << endl;
            SDL_mutexP(oscMsgQmutex); //lock
            oscMsgQ.storeMsg(m);
            SDL_mutexV(oscMsgQmutex); //unlock
          }
        }else if( strcmp( m.AddressPattern(), "/test1" ) == 0 ){
            // example #1 -- argument stream interface
            osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
            bool a1;
            osc::int32 a2;
            float a3;
            const char *a4;
            args >> a1 >> a2 >> a3 >> a4 >> osc::EndMessage;
            
            std::cout << "received '/test1' message with arguments: "
                << a1 << " " << a2 << " " << a3 << " " << a4 << "\n";
            
        }else if( strcmp( m.AddressPattern(), "/test2" ) == 0 ){
            // example #2 -- argument iterator interface, supports
            // reflection for overloaded messages (eg you can call 
            // (*arg)->IsBool() to check if a bool was passed etc).
            osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
            bool a1 = (arg++)->AsBool();
            int a2 = (arg++)->AsInt32();
            float a3 = (arg++)->AsFloat();
            const char *a4 = (arg++)->AsString();
            if( arg != m.ArgumentsEnd() )
                throw osc::ExcessArgumentException();
            
            std::cout << "received '/test2' message with arguments: "
                << a1 << " " << a2 << " " << a3 << " " << a4 << "\n";
        }else if( strcmp( m.AddressPattern(), "/test3" ) == 0 ) {
          
          osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
          
          if( arg->IsBool() ){
              bool a = (arg++)->AsBoolUnchecked();
              std::cout << "received '/test3' message with bool argument: "
                  << a << "\n";
          }else if( arg->IsInt32() ){
              int a = (arg++)->AsInt32Unchecked();
              std::cout << "received '/test3' message with int32 argument: "
                  << a << "\n";
          }else if( arg->IsFloat() ){
              float a = (arg++)->AsFloatUnchecked();
              std::cout << "received '/test3' message with float argument: "
                  << a << "\n";
          }else if( arg->IsString() ){
              const char *a = (arg++)->AsStringUnchecked();
              std::cout << "received '/test3' message with string argument: '"
                  << a << "'\n";
          }else{
              std::cout << "received '/test3' message with unexpected argument type\n";
          }
          
          //if( arg != m.ArgumentsEnd() )
              //throw ExcessArgumentException();
        } 
    }catch( osc::Exception& e ){
        // any parsing errors such as unexpected argument types, or 
        // missing arguments get thrown as exceptions.
        std::cout << "error while parsing message: "
            << m.AddressPattern() << ": " << e.what() << "\n";
    }
}



