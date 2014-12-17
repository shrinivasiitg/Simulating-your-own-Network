#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"

#include <stdio.h>
#include <stdlib.h>

FILE *output = fopen("scratch/q4_plot.txt", "w");

using namespace ns3;

/*Function to compute the throughput at the servet taken as argument*/
void get_throughput(Ptr<PacketSink> sink_a, Ptr<PacketSink> sink_b)
{
	Time t = Simulator::Now();								//current time of simulator
	int bytesTotal = sink_a->GetTotalRx();					//get no of packets received
    float mbs = (bytesTotal*8.0)/1000/t.GetSeconds();		//throughput = bytes*8/time/1000 Kbps

	std::cout << "Throughput(Sink 1): " << mbs << " Kbps" << std::endl; 
	fprintf(output, "%f\t", mbs); 
    
    bytesTotal = sink_b->GetTotalRx();					//get no of packets received
    t = Simulator::Now();								//current time of simulator
    mbs = (bytesTotal*8.0)/1000/t.GetSeconds();		//throughput = bytes*8/time/1000 Kbps
    
	std::cout << "Throughput(Sink 1): " << mbs << " Kbps" << std::endl; 
   	fprintf(output, "%f\n", mbs);

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
int main(int argc, char *argv[])
{
//	LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
//	LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
	
	//set maximum allowed queue size to be 10
	Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("10"));
	
	int i;
	for(i=0;i<2500;i+=100)
	{
		//create 4 nodes 
		NodeContainer nodes;
		nodes.Create (4);
		//create point to point links and devices
		PointToPointHelper ptp;
		NetDeviceContainer d02, d03, d01;
		//get the value of delay as a char to be passed on as string value		
		char * ptr = char_cast_of_int(i);
		std::string char_delay = ptr + std::string("ms");
		free(ptr);
		
		std::cout << "Delay: " << char_delay << std::endl;
		fprintf(output, "%s\t", char_delay.c_str());
			
		//set attributes of p2p links and install them on nodes to get devices
		ptp.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
	  	ptp.SetChannelAttribute ("Delay", StringValue("10ms"));
	  	d01 = ptp.Install (nodes.Get(0), nodes.Get(1));
	  	
	  	ptp.SetDeviceAttribute ("DataRate", StringValue ("1.5Mbps"));
	  	ptp.SetChannelAttribute ("Delay", StringValue("10ms"));
	  	d02 = ptp.Install (nodes.Get(0), nodes.Get(2));
	  	
	  	ptp.SetDeviceAttribute ("DataRate", StringValue ("1.5Mbps"));
	  	ptp.SetChannelAttribute ("Delay", StringValue(char_delay));
	  	d03 = ptp.Install (nodes.Get(0), nodes.Get(3));
	  	//install internet stack on the nodes
	  	InternetStackHelper internet;
	  	internet.Install (nodes);
	  	//assign addresses to get interfaces
	  	Ipv4AddressHelper address;

		address.SetBase ("10.1.1.0", "255.255.255.0");
		Ipv4InterfaceContainer i0i1 = address.Assign (d01);
	
		address.SetBase ("10.1.2.0", "255.255.255.0");
		Ipv4InterfaceContainer i0i2 = address.Assign (d02);

		address.SetBase ("10.1.3.0", "255.255.255.0");
		Ipv4InterfaceContainer i0i3 = address.Assign (d03);
		//create a drop tail queue	
		DropTailQueue dtq = DropTailQueue();
		//set the queue on each of the nodes
		d01.Get(0)->GetObject<PointToPointNetDevice>()->SetQueue(dtq.GetObject<Queue>());
		d01.Get(1)->GetObject<PointToPointNetDevice>()->SetQueue(dtq.GetObject<Queue>());
		d02.Get(1)->GetObject<PointToPointNetDevice>()->SetQueue(dtq.GetObject<Queue>());
		d03.Get(1)->GetObject<PointToPointNetDevice>()->SetQueue(dtq.GetObject<Queue>());
		//compute global routing tables
		Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
		//create bulk sender application -- set their attributes -- install them on nodes to get application container -- set start and stop times
		uint16_t port21 = 10;
		BulkSendHelper tcp21 ("ns3::TcpSocketFactory", InetSocketAddress (i0i1.GetAddress (1), port21));
		ApplicationContainer app21 = tcp21.Install(nodes.Get(2));
		app21.Start(Seconds(0.0));
		app21.Stop(Seconds(5.0));
	
		uint16_t port31 = 15;
		BulkSendHelper tcp31 ("ns3::TcpSocketFactory", InetSocketAddress (i0i1.GetAddress (1), port31));
		ApplicationContainer app31 = tcp31.Install(nodes.Get(3));
		app31.Start(Seconds(0.0));
		app31.Stop(Seconds(5.0));
		//create packet sinks and install them on nodes -- set start and stop times
		PacketSinkHelper sink21 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port21));
		ApplicationContainer sinkApps21 = sink21.Install (nodes.Get (1));
		sinkApps21.Start (Seconds (0.0));
		sinkApps21.Stop (Seconds (5.0));
	
		PacketSinkHelper sink31 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port31));
		ApplicationContainer sinkApps31 = sink31.Install (nodes.Get (1));
		sinkApps31.Start (Seconds (0.0));
		sinkApps31.Stop (Seconds (5.0));
		//get packet sinks from packet sink apps to get data received to calculate throughput
		Ptr<PacketSink> sink_a = DynamicCast<PacketSink> (sinkApps21.Get (0));
		Ptr<PacketSink> sink_b = DynamicCast<PacketSink> (sinkApps31.Get (0));
		//scchedule the function at the end of the simulation to get throughput averaged over the time of connection
		Simulator::Schedule(Seconds(5.0), &get_throughput, sink_a, sink_b);		
		Simulator::Stop(Seconds(5.0));
		Simulator::Run();
		Simulator::Destroy();
	}
	return 0;
}
