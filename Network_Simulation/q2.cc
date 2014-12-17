#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/nstime.h"
#include "ns3/onoff-application.h"
#include "ns3/on-off-helper.h"
#include "ns3/global-route-manager.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/ipv4-flow-probe.h"
#include "ns3/flow-monitor-module.h"

#include <stdio.h>
#include <stdlib.h>

using namespace ns3;

FILE *output = fopen("scratch/q2_plot.txt", "w");

NS_LOG_COMPONENT_DEFINE ("q2");
/*This function outputs the no of packets lost till the time at which it is called*/
void get_lost_packets(Ptr<FlowMonitor> monitor, FlowMonitorHelper flowmon)
{
	//check for lost packets since the monitor is never stopped
	monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	//get stats
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	//get current time
	Time t = Simulator::Now();
		
	fprintf(output, "%f\t", t.GetSeconds());
	std::cout << t.GetSeconds() << "\n";
		
	//print the stats of no of lost packets
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{
		double percent_loss = (i->second.lostPackets*100)/(i->second.txBytes/128);  
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\t";   
		std::cout << "  Percent Lost Packets: " << percent_loss << "\n";
		fprintf(output, "%f\t", percent_loss);
	}
	std::cout << "\n";
	fprintf(output, "\n");
	//schedule this function to be called again after 0.1 second 
	if(Simulator::Now().GetSeconds()<4.0)
	{
		Simulator::Schedule(Seconds(0.1), &get_lost_packets, monitor, flowmon);
	}
}

int main (int argc, char *argv[])
{
//  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  //create nodes
  NodeContainer nodes;
  nodes.Create (5);
  //create point to point links and set their attribtues and install them on nodes to get devices
  PointToPointHelper ptp;
  NetDeviceContainer d01, d12, d03, d34, d42, devices;
  //n0-n1 ptp link
  ptp.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  ptp.SetChannelAttribute ("Delay", StringValue("2ms"));
  d01 = ptp.Install (nodes.Get(0), nodes.Get(1));
  //n1-n2 ptp link
  ptp.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  ptp.SetChannelAttribute ("Delay", StringValue("2ms"));
  d12 = ptp.Install (nodes.Get(1), nodes.Get(2));
  //n0-n3 ptp link
  ptp.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  ptp.SetChannelAttribute ("Delay", StringValue("2ms"));
  d03 = ptp.Install (nodes.Get(0), nodes.Get(3));
  //n3-n4 ptp link
  ptp.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  ptp.SetChannelAttribute ("Delay", StringValue("2ms"));
  d34 = ptp.Install (nodes.Get(3), nodes.Get(4));
  //n4-n2 ptp link
  ptp.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  ptp.SetChannelAttribute ("Delay", StringValue("2ms"));
  d42 = ptp.Install (nodes.Get(4), nodes.Get(2));
  //install internet stack  
  InternetStackHelper internet;
  internet.Install (nodes);
  Ipv4AddressHelper address;
  //assign address -- one subnet per ptp link for clarity
  
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = address.Assign (d01);
  
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = address.Assign (d12);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i3 = address.Assign (d03);
  
  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i4 = address.Assign (d34);
  
  address.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i4i2 = address.Assign (d42);
  //compute routing tables for the topology
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  //create a CBR traffic source towards node 2 and install it on node 0
  uint32_t port = 9;
  OnOffHelper cbr02 = OnOffHelper("ns3::UdpSocketFactory", InetSocketAddress(i1i2.GetAddress(1), port));
  cbr02.SetConstantRate(DataRate("900kbps"), 128);
  ApplicationContainer app02 = cbr02.Install(nodes.Get(0));
  app02.Start(Seconds(1.0));
  app02.Stop(Seconds(3.5));
  //create a CBR traffic source towards node 1 and install it on node 0  
  OnOffHelper cbr01 = OnOffHelper("ns3::UdpSocketFactory", InetSocketAddress(i0i1.GetAddress(1), port));
  cbr01.SetConstantRate(DataRate("300kbps"), 128);
  ApplicationContainer app01 = cbr01.Install(nodes.Get(0));
  app01.Start(Seconds(1.5));
  app01.Stop(Seconds(3.0));
  
  //get interface of node 0 on the n0-n1 ptp link to set it down
  Ptr<Node> n1 = nodes.Get (0);
  Ptr<Ipv4> ipv41 = n1->GetObject<Ipv4> ();
  // The first ifIndex is 0 for loopback, then the first p2p is numbered 1,
  uint32_t ipv4ifIndex1 = 1;	//interface index corrosponding to p2p link of node 0 and node 1
  
  //schedule set down of interface
  Simulator::Schedule (Seconds (2.0),&Ipv4::SetDown,ipv41, ipv4ifIndex1);
  //schedule recompute routing tables
  Simulator::Schedule(Seconds(2.1), &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
  //schedule set up of link
  Simulator::Schedule (Seconds (2.7),&Ipv4::SetUp,ipv41, ipv4ifIndex1);
  //schedule recompute routing tables
  Simulator::Schedule(Seconds(2.8), &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
  //create a flow monitor and install it on nodes
  FlowMonitorHelper flowmon = FlowMonitorHelper();
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  //schedule the function to monitor for the first time -- after that it schedules itself for fixed interval of times
  Simulator::Schedule(Seconds(0.0), &get_lost_packets, monitor, flowmon);
  Simulator::Stop(Seconds(4));  
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
