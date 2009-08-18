
#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "ip/UdpSocket.h"
#include <string>

using namespace std;

int listen_for_osc_packets(int port);
int osc_thread_main(void* tport);

class ExamplePacketListener : public osc::OscPacketListener {
    
  public:
    string address_pattern2match;
    int run_osc_test(int port);
  protected:
    virtual void ProcessMessage( const osc::ReceivedMessage& m, 
      const IpEndpointName& remoteEndpoint );
};

