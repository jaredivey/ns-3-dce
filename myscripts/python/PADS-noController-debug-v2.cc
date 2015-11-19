/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Network topology
 * A ring core network consisting numGatewaySwitchs GatewaySwitch(GS) nodes, each GS node is connected
 * to a ring subnetwork consisting numSwitchsPerSubNet Switch (S) nodes, and each S node connects to numHostsPerSwitch hosts
 */
#include <string>
#include <fstream>
#include <vector>
#include <sys/time.h>

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/layer2-p2p-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/SdnSwitch.h"
#include "ns3/log.h"
#include "ns3/ipv4-static-routing-helper.h"

using namespace ns3;

#define REALLY_BIG_TIME 1000000

typedef struct timeval TIMER_TYPE;
#define TIMER_NOW(_t) gettimeofday (&_t,NULL);
#define TIMER_SECONDS(_t) ((double)(_t).tv_sec + (_t).tv_usec * 1e-6)
#define TIMER_DIFF(_t1, _t2) (TIMER_SECONDS (_t1) - TIMER_SECONDS (_t2))

NS_LOG_COMPONENT_DEFINE ("PADS-noController-v2");

int main (int argc, char *argv[])
{
  typedef std::vector<NodeContainer> vector_of_NodeContainer;
  typedef std::vector<vector_of_NodeContainer> vector_of_vector_of_NodeContainer;

  typedef std::vector<Ipv4InterfaceContainer> vector_of_Ipv4InterfaceContainer;
  typedef std::vector<vector_of_Ipv4InterfaceContainer> vector_of_vector_of_Ipv4InterfaceContainer;

  typedef std::vector<NetDeviceContainer> vector_of_NetDeviceContainer;
  typedef std::vector<vector_of_NetDeviceContainer> vector_of_vector_of_NetDeviceContainer;

  uint32_t maxBytes = 5000;
  uint32_t num_switches_per_subnet = 3;
  uint32_t num_hosts_per_switch    = 3;


  CommandLine cmd;
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("num_switches_per_subnet", "Number of switches per subnet", num_switches_per_subnet);
  cmd.AddValue ("num_hosts_per_switch", "Number of hosts per end switch", num_hosts_per_switch);
 // cmd.AddValue ("num_gateway_switches", "Number of gateway switches/Number of subnets", num_gateway_switches);
  cmd.Parse (argc,argv);

  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  LogComponentEnable ("Ipv4StaticRouting", LOG_LEVEL_INFO);
  LogComponentEnable ("PointToPointChannel", LOG_LEVEL_INFO);
  LogComponentEnable ("PADS-noController-v2", LOG_LEVEL_INFO);

  TIMER_TYPE t0, t1, t2;
  TIMER_NOW (t0);

  //define nodes
  NodeContainer edge_switch_nodes;
  vector_of_NodeContainer host_nodes(num_switches_per_subnet);

  //define ipv4 interface
  vector_of_Ipv4InterfaceContainer edge_switch_ipv4_interfaces(num_switches_per_subnet);
  vector_of_vector_of_Ipv4InterfaceContainer host_ipv4_interfaces(num_switches_per_subnet,vector_of_Ipv4InterfaceContainer(num_hosts_per_switch));

  //define net device
  vector_of_NetDeviceContainer edge_switch_net_devices(num_switches_per_subnet);
  vector_of_vector_of_NetDeviceContainer host_net_devices(num_switches_per_subnet,vector_of_NetDeviceContainer(num_hosts_per_switch));


  //define links
  // NS_LOG_INFO ("Create channels.");
  PointToPointHelper link_edgeSwitch_2_edgeSwitch, link_host_2_edgeSwitch;
  link_edgeSwitch_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_edgeSwitch_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("5ms"));
  link_host_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  link_host_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("2ms"));

  //define the routing protocol, use nix vector routing protocol
  InternetStackHelper stack;
  //Ipv4StaticRoutingHelper staticRouting;
 // stack.SetRoutingHelper(staticRouting);

  //define the ipv4 address helper
  Ipv4AddressHelper address;
  std::ostringstream oss;

  NS_LOG_INFO("Create the edge switches, and connect these switches by ring");
  //create the edge switch nodes
  edge_switch_nodes.Create(num_switches_per_subnet);
  stack.Install(edge_switch_nodes);

  //create the rings connecting the edge switch nodes
  for (uint32_t j = 0; j < num_switches_per_subnet; j++)
  {
	int next;
	if (j == num_switches_per_subnet-1)
	{
		next = 0;
	}
	else
	{
		next = j + 1;
	}

	//set the address base
	oss.str("");
	oss << "10.0." << j << ".0";
	address.SetBase(oss.str().c_str(),"255.255.255.0");
	edge_switch_net_devices[j] = link_edgeSwitch_2_edgeSwitch.Install
			(NodeContainer(edge_switch_nodes.Get(j), edge_switch_nodes.Get(next)));
    edge_switch_ipv4_interfaces[j] = address.Assign(edge_switch_net_devices[j]);

    //debug
    //std::ostream os;
    std::cout << "edge switch - edge switch: " << j << "-" << j+1 << "  ";
    edge_switch_ipv4_interfaces[j].GetAddress(0,0).Print(std::cout);
    std::cout << "--" ;
    edge_switch_ipv4_interfaces[j].GetAddress(1,0).Print(std::cout);
    std::cout << std::endl;
  }

  //create the host nodes
  for (uint32_t i = 0; i < num_switches_per_subnet; i++)
  {
	  host_nodes[i].Create(num_hosts_per_switch);
	  stack.Install(host_nodes[i]);
  }
  NS_LOG_INFO("Create the hosts, connect the hosts to edge switches, and assign ip");

  for (uint32_t j = 0; j < num_switches_per_subnet; j++)
  {

	for (uint32_t z = 0; z < num_hosts_per_switch; z++)
	{
		//connect the host to the responding edge switch
		host_net_devices[j][z] = link_host_2_edgeSwitch.Install
				(NodeContainer(host_nodes[j].Get(z),edge_switch_nodes.Get(j)));

		//set the address base for the hosts connecting to the (i,j) th edge switch
		oss.str("");
		oss << "143." << z << "." << j << ".0";
		address.SetBase(oss.str().c_str(),"255.255.255.0");
		//assign ip
		host_ipv4_interfaces[j][z] = address.Assign(host_net_devices[j][z]);

		//debug
		//std::ostream os;
		std::cout << "host - edge switch: " << z << "-" << j << "  ";
		host_ipv4_interfaces[j][z].GetAddress(0,0).Print(std::cout);
		std::cout << "--" ;
		host_ipv4_interfaces[j][z].GetAddress(1,0).Print(std::cout);
		std::cout << std::endl;
	}
  }
  /* install ON-OFF application and packet sink application in every hosts
   * every node will be installed with an ON-OFF application and packet sink application
   * the destination of the ON-OFF application is local with probability of 0.5 and is remote with 0.5
   */
  ApplicationContainer sinkApps;
  NS_LOG_INFO("Install ON-OFF application");
  Ptr<UniformRandomVariable> random_num_generator = CreateObject<UniformRandomVariable>();
  for (uint32_t j = 0; j < num_switches_per_subnet; j++)
  {
	  for (uint32_t z = 0; z < num_hosts_per_switch; z++)
	  {
		PacketSinkHelper sink_helper("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));
		//PacketSinkHelper sink_helper("ns3::TcpSocketFactory",InetSocketAddress(host_ipv4_interfaces[i][j][z].GetAddress(0),9999));

		//ApplicationContainer sink_app;
		sinkApps.Add(sink_helper.Install(host_nodes[j].Get(z)));
		//sink_app.Start(Seconds(0.0));

		//install the on-off application in the host
		int j_local, z_local;
		j_local = (int) num_switches_per_subnet * random_num_generator->GetValue();
		  //choose the host (i.e., destination) randomly
		if (j_local == j)
		{
			while (true)
			{
			  z_local = (int) num_hosts_per_switch * random_num_generator->GetValue();
			  if (z_local != z)
			break;
			}
		}
		else
		{
			z_local = (int) num_hosts_per_switch * random_num_generator->GetValue();
		}
		OnOffHelper on_off_helper("ns3::UdpSocketFactory", Address());
		AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[j_local][z_local].GetAddress(0,0),9999));


		//debug
		uint8_t buffer[20];
		uint32_t k_temp;
		k_temp = remote_address.Get().CopyTo(buffer);
		std::cout<< "(" << j << ", " << z << ") -- " << "(" << j_local << ", " << z_local << "): " << \
				unsigned(buffer[0]) << "." << unsigned(buffer[1]) << "." << unsigned(buffer[2]) << "." << unsigned(buffer[3]) <<std::endl;

		on_off_helper.SetAttribute("Remote", remote_address);
		on_off_helper.SetAttribute("DataRate",StringValue("1Kbps"));
		on_off_helper.SetAttribute("MaxBytes",UintegerValue(512));
		ApplicationContainer on_off_app;
		on_off_app = on_off_helper.Install(host_nodes[j].Get(z));
		on_off_app.Start(Seconds(random_num_generator->GetValue()));
	  }
  }
  sinkApps.Start(Seconds(0.0));

  std::cout << "Running simulator ... " << std::endl;
  link_host_2_edgeSwitch.EnablePcapAll("point_point");
  // Calculate routing tables
  std::cout << "Populating Routing tables..." << std::endl;
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Simulator::Stop(Seconds(10.0));
  Simulator::Run();
  uint32_t totalRx = 0;
 	for (uint32_t i = 0; i < sinkApps.GetN(); ++i)
 	{
 	 Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (i));
 	 if (sink1)
 	   {
 		 totalRx += sink1->GetTotalRx();
 	   }
 	}
   std::cout << "total received bytes = " << totalRx << std::endl;

  Simulator::Destroy();
  std::cout << "Simulation is finished" << std::endl;
  return 0;
}







