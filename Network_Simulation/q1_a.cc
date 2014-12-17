#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/nstime.h"

#include <stdio.h>
#include <stdlib.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("q1_a");
/*variable to store the total no of bytes received at the server, kept as global variable so as to avoid passing arguments*/
unsigned int bytesTotal;
/*Function to compute the throughput at the servet taken as argument*/
void get_throughput(Ptr<UdpServer> server)
{
	int packets = server->GetReceived();					//get no of packets received
    bytesTotal = packets*1024; 								//no of packets into packet size = bytes
    Time t = Simulator::Now();								//current time of simulator
    float mbs = (bytesTotal*8.0)/1000/t.GetSeconds();		//throughput = bytes*8/time/1000 Kbps
    std::cout << "Throughput: " << mbs << " Kbps" << std::endl << std::endl; 
}
/* This function takes an integer value and returns a char * which is that integer value as a string */
char *char_cast_of_int(int delay)
{
    int i, nchar;
    int temp = delay;
    i = nchar = 0;
    while(temp!=0)
    {
        temp = temp/10;
        nchar++;
    }
    char *c_delay = (char *)malloc((nchar+1)*sizeof(char));
    for(i=nchar-1;i>=0;i--)
    {
        c_delay[i] = delay%10 + 48;
        delay = delay/10;
    }
    c_delay[nchar] = '\0';
    return c_delay;
}
int main (int argc, char *argv[])
{
  //loop for delay
  int i;
  for(i=0;i<3000;i+=100)
  {
  	  //creating nodes
      NodeContainer nodes;
      nodes.Create (2);
      
      int delay = i;
      //get char delay to pass as string value
      char * ptr = char_cast_of_int(delay);
      std::string char_delay = ptr + std::string("ms");
      free(ptr);
      std::cout << "Delay: " << char_delay << std::endl;
      //point to point link helper 
      PointToPointHelper pointToPoint;
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("8Kbps"));
      pointToPoint.SetChannelAttribute ("Delay", StringValue(char_delay));
	  //install point to point in nodes
      NetDeviceContainer devices;
      devices = pointToPoint.Install (nodes);
	  //install internet stack
      InternetStackHelper stack;
      stack.Install (nodes);
	  //assign addresses
      Ipv4AddressHelper address;
      address.SetBase ("10.1.1.0", "255.255.255.0");
	  Ipv4InterfaceContainer interfaces = address.Assign (devices);
	  //create server at port 10
      UdpServerHelper Server (10);
	  //install server and set start and stop times
      ApplicationContainer serverApps = Server.Install (nodes.Get (1));
      serverApps.Start (Seconds (0.0));
      serverApps.Stop (Seconds (20.0));
	  //create clienrt and set attributes
      UdpClientHelper Client (interfaces.GetAddress (1), 10);
      Client.SetAttribute ("MaxPackets", UintegerValue (100));
      Client.SetAttribute ("Interval", TimeValue (Seconds (0.0)));
      Client.SetAttribute ("PacketSize", UintegerValue (1024));
	  //install client on node
      ApplicationContainer clientApps = Client.Install (nodes.Get (0));
      clientApps.Start (Seconds (0.0));
      clientApps.Stop (Seconds (20.0));

	  //schedule the get_throughput function to calculate throughput at the end of the simulation
      Simulator::Schedule(Seconds(20), &get_throughput, Server.GetServer());
      //run simulation
      Simulator::Run ();
      Simulator::Destroy ();
      //set bytesTotal = 0 for new simulation
      bytesTotal = 0;
  }    
  return 0;
}
