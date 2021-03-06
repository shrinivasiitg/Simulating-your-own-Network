#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include <stdio.h>
#include <cstring>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");

FILE *fp_1,*fp_2,*fp_3,*fp_4,*fp_5;

/*Function to get throughput*/
void get_throughput(Ptr<PacketSink> sink_a, Ptr<PacketSink> sink_b,Ptr<PacketSink> sink_c,Ptr<PacketSink> sink_d,Ptr<PacketSink> sink_e)
{
	
	// Throughput calculation.
	// Transfer Size = sink->GetTotalRx()
	// Throughput = Transfer Size/Time
	// fprintf for writing simultaneuosly into file used for generating graphs
	
	Time t = Simulator::Now();
	printf("Current time:%f seconds.\n",t.GetSeconds());
	
	//connection 1
	int bytesTotal = sink_a->GetTotalRx();
	float throughput = (bytesTotal*8.0)/1000/t.GetSeconds();
	printf("Throughput at sink 1 : %f Kbps.\n",throughput);
	fprintf(fp_1,"%f\t",t.GetSeconds());
	fprintf(fp_1,"%f\n",throughput); 
	
	//connection 2
	bytesTotal = sink_b->GetTotalRx();
	t = Simulator::Now();
	throughput = (bytesTotal*8.0)/1000/t.GetSeconds();
	printf("Throughput at sink 2 : %f Kbps.\n",throughput);
	fprintf(fp_2,"%f\t",t.GetSeconds());
	fprintf(fp_2,"%f\n",throughput);
	
	//connection 3
	bytesTotal = sink_c->GetTotalRx();
	t = Simulator::Now();
	throughput = (bytesTotal*8.0)/1000/t.GetSeconds();
	printf("Throughput at sink 3 : %f Kbps.\n",throughput);
	fprintf(fp_3,"%f\t",t.GetSeconds());
	fprintf(fp_3,"%f\n",throughput);
	
	//connection 4
	bytesTotal = sink_d->GetTotalRx();
	t = Simulator::Now();
	throughput = (bytesTotal*8.0)/1000/t.GetSeconds();
	printf("Throughput at sink 4 : %f Kbps.\n",throughput);
	fprintf(fp_4,"%f\t",t.GetSeconds());
	fprintf(fp_4,"%f\n",throughput);
	
	//connection 5
	bytesTotal = sink_e->GetTotalRx();
	t = Simulator::Now();
	throughput = (bytesTotal*8.0)/1000/t.GetSeconds();
	printf("Throughput at sink 5 : %f Kbps.\n\n",throughput);
	fprintf(fp_5,"%f\t",t.GetSeconds());
	fprintf(fp_5,"%f\n",throughput);
	
	//reschedule
	if(t.GetSeconds()<50.1)
	{
		Simulator::Schedule(Seconds(0.1), &get_throughput, sink_a, sink_b,sink_c,sink_d,sink_e);
	}
}
int main (int argc, char *argv[])
{

  //Open Files so that they can be given directly to gnuplot.
  fp_1=fopen("scratch/q3_con1.txt","w");
  fp_2=fopen("scratch/q3_con2.txt","w");
  fp_3=fopen("scratch/q3_con3.txt","w");
  fp_4=fopen("scratch/q3_con4.txt","w");
  fp_5=fopen("scratch/q3_con5.txt","w");

  //max bytes to transfer -- 0 means infinite
  uint32_t maxBytes = 0;

  //create nodes
  NodeContainer nodes;
  nodes.Create (2);
  
  //create p2p links
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));
  //install link in nodes to get devices
  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);
  //install the internet stack on the nodes
  InternetStackHelper internet;
  internet.Install (nodes);
  //assign addresses
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  //port no's for the five connections
  uint16_t port_1 = 9, port_2 = 10, port_3 = 11, port_4 = 12, port_5 = 13;

  //create applications -- set attribtues -- install into nodes to get apps -- set their start and atop times
  BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (i.GetAddress (1), port_1));
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (50.1));
  
  BulkSendHelper source_1 ("ns3::TcpSocketFactory", InetSocketAddress (i.GetAddress (1), port_2));
  source_1.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps_1 = source_1.Install (nodes.Get (0));
  sourceApps_1.Start (Seconds (5.0));
  sourceApps_1.Stop (Seconds (45.0));
  
  BulkSendHelper source_2 ("ns3::TcpSocketFactory", InetSocketAddress (i.GetAddress (1), port_3));
  source_2.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps_2 = source_2.Install (nodes.Get (0));
  sourceApps_2.Start (Seconds (10.0));
  sourceApps_2.Stop (Seconds (40.0));
  
  BulkSendHelper source_3 ("ns3::TcpSocketFactory", InetSocketAddress (i.GetAddress (1), port_4));
  source_3.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps_3 = source_3.Install (nodes.Get (0));
  sourceApps_3.Start (Seconds (15.0));
  sourceApps_3.Stop (Seconds (35.0));
  
  BulkSendHelper source_4 ("ns3::TcpSocketFactory", InetSocketAddress (i.GetAddress (1), port_5));
  source_4.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps_4 = source_4.Install (nodes.Get (0));
  sourceApps_4.Start (Seconds (20.0));
  sourceApps_4.Stop (Seconds (30.0));

  //create a PacketSinkApplication and install it on nodes

  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port_1));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (50.1));
  
  PacketSinkHelper sink_1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port_2));
  ApplicationContainer sinkApps_1 = sink_1.Install (nodes.Get (1));
  sinkApps_1.Start (Seconds (5.0));
  sinkApps_1.Stop (Seconds (45.0));
  
  PacketSinkHelper sink_2 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port_3));
  ApplicationContainer sinkApps_2 = sink_2.Install (nodes.Get (1));
  sinkApps_2.Start (Seconds (10.0));
  sinkApps_2.Stop (Seconds (40.0));
  
  PacketSinkHelper sink_3 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port_4));
  ApplicationContainer sinkApps_3 = sink_3.Install (nodes.Get (1));
  sinkApps_3.Start (Seconds (15.0));
  sinkApps_3.Stop (Seconds (35.0));
  
  PacketSinkHelper sink_4 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port_5));
  ApplicationContainer sinkApps_4 = sink_4.Install (nodes.Get (1));
  sinkApps_4.Start (Seconds (20.0));
  sinkApps_4.Stop (Seconds (30.0));

  //get sinks for calculation of throughput 
  Ptr<PacketSink> sink_a = DynamicCast<PacketSink> (sinkApps.Get (0));
  Ptr<PacketSink> sink_b = DynamicCast<PacketSink> (sinkApps_1.Get (0));
  Ptr<PacketSink> sink_c = DynamicCast<PacketSink> (sinkApps_2.Get (0));
  Ptr<PacketSink> sink_d = DynamicCast<PacketSink> (sinkApps_3.Get (0));
  Ptr<PacketSink> sink_e = DynamicCast<PacketSink> (sinkApps_4.Get (0));
  //schedule the function once, it will reschedule itself repeatedly itself
  Simulator::Schedule(Seconds(0.1), &get_throughput, sink_a,sink_b,sink_c,sink_d,sink_e);
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
