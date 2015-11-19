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
//#include "ns3/dce-module.h"
#include "ns3/SdnSwitch.h"
#include "ns3/log.h"
#include "ns3/ipv4-static-routing-helper.h"

using namespace ns3;

#define REALLY_BIG_TIME 1000000

typedef struct timeval TIMER_TYPE;
#define TIMER_NOW(_t) gettimeofday (&_t,NULL);
#define TIMER_SECONDS(_t) ((double)(_t).tv_sec + (_t).tv_usec * 1e-6)
#define TIMER_DIFF(_t1, _t2) (TIMER_SECONDS (_t1) - TIMER_SECONDS (_t2))

NS_LOG_COMPONENT_DEFINE ("PADS2016-noController");

enum CONTROLLER
{
  RYU,
  POX,
} CONTROLLER;

enum APPCHOICE
{
  BULK_SEND,
  PING,
  ON_OFF,
} APPCHOICE;

int main (int argc, char *argv[])
{
  typedef std::vector<NodeContainer> vector_of_NodeContainer;
  typedef std::vector<vector_of_NodeContainer> vector_of_vector_of_NodeContainer;
  typedef std::vector<vector_of_vector_of_NodeContainer> vector_of_vector_of_vector_of_NodeContainer;

  typedef std::vector<Ipv4InterfaceContainer> vector_of_Ipv4InterfaceContainer;
  typedef std::vector<vector_of_Ipv4InterfaceContainer> vector_of_vector_of_Ipv4InterfaceContainer;
  typedef std::vector<vector_of_vector_of_Ipv4InterfaceContainer> vector_of_vector_of_vector_of_Ipv4InterfaceContainer;

  typedef std::vector<NetDeviceContainer> vector_of_NetDeviceContainer;
  typedef std::vector<vector_of_NetDeviceContainer> vector_of_vector_of_NetDeviceContainer;
  typedef std::vector<vector_of_vector_of_NetDeviceContainer> vector_of_vector_of_vector_of_NetDeviceContainer;

  bool verbose = true;//why need this?
  uint32_t maxBytes = 5000;
 // uint32_t controller = RYU;
  uint32_t appChoice = ON_OFF;

  uint32_t num_gateway_switches    = 3;
  uint32_t num_switches_per_subnet = 3;
  uint32_t num_hosts_per_switch    = 2;
  double   prob_remote_app         = 0.5;


  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
 // cmd.AddValue ("controller",
  //              "Controller to use: (0) Ryu; (1) POX", controller);
  cmd.AddValue ("appChoice",
                "Application to use: (0) Bulk Send; (1) Ping; (2) On Off", appChoice);
  cmd.AddValue ("num_switches_per_subnet", "Number of switches per subnet", num_switches_per_subnet);
  cmd.AddValue ("num_hosts_per_switch", "Number of hosts per end switch", num_hosts_per_switch);
  cmd.AddValue ("num_gateway_switches", "Number of gateway switches/Number of subnets", num_gateway_switches);
  cmd.AddValue ("prob_remote_app", "Probability of the event that the destination of an app is remote", prob_remote_app);
 // cmd.AddValue ("numControllers", "Number of controllers; switches will be assigned equally across controllers", numControllers);

  cmd.Parse (argc,argv);
  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  LogComponentEnable ("Ipv4StaticRouting", LOG_LEVEL_INFO);
  LogComponentEnable ("PointToPointChannel", LOG_LEVEL_INFO);
  if (verbose)
    {
      LogComponentEnable ("PADS2016-noController", LOG_LEVEL_INFO);
      LogComponentEnable ("SdnSwitch", LOG_LEVEL_INFO);
    }

 // NS_ASSERT_MSG (numSwitches % numControllers == 0, "Number of switches must be a multiple of number of controllers.");
 // NS_ASSERT_MSG (numSwitches % 2 == 0, "Number of switches must be even.");

  TIMER_TYPE t0, t1, t2;
  TIMER_NOW (t0);

  //define nodes
  NodeContainer gateway_switch_nodes;
  vector_of_NodeContainer edge_switch_nodes(num_gateway_switches);
  vector_of_vector_of_NodeContainer host_nodes(num_gateway_switches,vector_of_NodeContainer(num_switches_per_subnet));

  //define ipv4 interface
  vector_of_Ipv4InterfaceContainer gateway_switch_ipv4_interfaces(num_gateway_switches);
  vector_of_Ipv4InterfaceContainer gateway_edge_ipv4_interfaces(num_gateway_switches);
  vector_of_vector_of_Ipv4InterfaceContainer edge_switch_ipv4_interfaces(num_gateway_switches,vector_of_Ipv4InterfaceContainer(num_switches_per_subnet));
  vector_of_vector_of_vector_of_Ipv4InterfaceContainer host_ipv4_interfaces(num_gateway_switches,vector_of_vector_of_Ipv4InterfaceContainer(num_switches_per_subnet,vector_of_Ipv4InterfaceContainer(num_hosts_per_switch)));

  //define net device
  vector_of_NetDeviceContainer gateway_switch_net_devices(num_gateway_switches);
  vector_of_NetDeviceContainer gateway_edge_net_devices(num_gateway_switches);
  vector_of_vector_of_NetDeviceContainer edge_switch_net_devices(num_gateway_switches,vector_of_NetDeviceContainer(num_switches_per_subnet));
  vector_of_vector_of_vector_of_NetDeviceContainer host_net_devices(num_gateway_switches,vector_of_vector_of_NetDeviceContainer(num_switches_per_subnet,vector_of_NetDeviceContainer(num_hosts_per_switch)));


  //define links
  // NS_LOG_INFO ("Create channels.");
  PointToPointHelper link_gateway_2_gateway, link_edgeSwitch_2_edgeSwitch, link_edgeSwitch_2_gateway, link_host_2_edgeSwitch;
  link_gateway_2_gateway.SetDeviceAttribute ("DataRate", StringValue ("2Gbps"));
  link_gateway_2_gateway.SetChannelAttribute ("Delay", StringValue ("200ms"));
  link_edgeSwitch_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_edgeSwitch_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("5ms"));
  link_edgeSwitch_2_gateway.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_edgeSwitch_2_gateway.SetChannelAttribute ("Delay", StringValue ("5ms"));
  link_host_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  link_host_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("1ms"));

  //enable the pcap for links
  AsciiTraceHelper ascii;
  //define the routing protocol, use nix vector routing protocol
  InternetStackHelper stack;
 // Ipv4StaticRoutingHelper staticRouting;
 // stack.SetRoutingHelper(staticRouting);

  //define the ipv4 address helper
  Ipv4AddressHelper address;
  std::ostringstream oss;

  //create the networks
  NS_LOG_INFO("Create the network");
  NS_LOG_INFO("Create the core network and assign ip");

  // ----------------------------create the core network--------------------------
  // create the gateway nodes
  gateway_switch_nodes.Create(num_gateway_switches);
  stack.Install(gateway_switch_nodes);

  // create the ring connecting the gateway switches
  for (uint32_t i = 0; i < num_gateway_switches; i++)
  {
	//set the address base for the core network
	oss.str("");
	oss << "251.168." << i << ".0";
	address.SetBase (oss.str().c_str(), "255.255.255.0");
	if (i < num_gateway_switches - 1)
		gateway_switch_net_devices[i] = link_gateway_2_gateway.Install(gateway_switch_nodes.Get(i),gateway_switch_nodes.Get(i+1));
	else
		gateway_switch_net_devices[i] = link_gateway_2_gateway.Install(gateway_switch_nodes.Get(i),gateway_switch_nodes.Get(0));
	// assign the address
	gateway_switch_ipv4_interfaces[i] = address.Assign(gateway_switch_net_devices[i]);

	//debug
	std::cout << "gateway switch - gateway switch: " << "(" << i <<  ") ";
	gateway_switch_ipv4_interfaces[i].GetAddress(0,0).Print(std::cout);
	std::cout << "--" ;
	gateway_switch_ipv4_interfaces[i].GetAddress(1,0).Print(std::cout);
	std::cout << std::endl;
  }

  // ----------------------------create the edge subnetwork ---------------------------------
  for (uint32_t i = 0; i < num_gateway_switches; i++)
    {
      NS_LOG_INFO("Create the " << i <<"th subnetwork, and assign ip");
      //create the nodes in the ith subnet
      edge_switch_nodes[i].Create(num_switches_per_subnet);
      stack.Install(edge_switch_nodes[i]);

      //create the ring connecting the edge nodes in the ith subnet
      for (uint32_t j = 0; j < num_switches_per_subnet; j++)
      {
    	  //set the address base for the ith subnet
    	  oss.str("");
    	  oss << "1." << i << "." << j << ".0";
    	  address.SetBase(oss.str().c_str(),"255.255.255.0");
    	  if (j < num_switches_per_subnet - 1)
    		  edge_switch_net_devices[i][j] = link_edgeSwitch_2_edgeSwitch.Install(edge_switch_nodes[i].Get(j),edge_switch_nodes[i].Get(j+1));
    	  else
    		  edge_switch_net_devices[i][j] = link_edgeSwitch_2_edgeSwitch.Install(edge_switch_nodes[i].Get(j),edge_switch_nodes[i].Get(0));
    	  // assign the address
    	  edge_switch_ipv4_interfaces[i][j] = address.Assign(edge_switch_net_devices[i][j]);

    	  //debug
		  std::cout << "edge switch - edge switch: " << "(" << i << ", " << j << ") ";
		  edge_switch_ipv4_interfaces[i][j].GetAddress(0,0).Print(std::cout);
		  std::cout << "--" ;
		  edge_switch_ipv4_interfaces[i][j].GetAddress(1,0).Print(std::cout);
		  std::cout << std::endl;

      }
      //connect the subnet to the gateway switch
      NS_LOG_INFO("Connect the " << i << " th  subnetwork to the" << i << "th gateway switch, and assign ip");
      gateway_edge_net_devices[i] = link_edgeSwitch_2_gateway.Install(gateway_switch_nodes.Get(i),edge_switch_nodes[i].Get(0));
      oss.str("");
      oss << "1." << num_gateway_switches + i << "." << num_switches_per_subnet + 2 << ".0";
      address.SetBase(oss.str().c_str(),"255.255.255.0");
      gateway_edge_ipv4_interfaces[i] = address.Assign(gateway_edge_net_devices[i]);

      //debug
	  std::cout << "edge switch - gateway switch: " << "(" << i << ", 0 -- " << i << ") ";
      gateway_edge_ipv4_interfaces[i].GetAddress(0,0).Print(std::cout);
	  std::cout << "--" ;
	  gateway_edge_ipv4_interfaces[i].GetAddress(1,0).Print(std::cout);
	  std::cout << std::endl;
    }

  // -------------------------------create the host nodes-------------------------
  NS_LOG_INFO("Create the hosts, connect the hosts to edge switches, and assign ip");
  for (uint32_t i = 0; i < num_gateway_switches; i++)
  {
    for (uint32_t j = 0; j < num_switches_per_subnet; j++)
    {
      // create the host nodes
      host_nodes[i][j].Create(num_hosts_per_switch);
      stack.Install(host_nodes[i][j]);

      // enable ascii tracing
      oss.str("");
      oss << "PADS-noController-v3-host-" << i << "-" << j << ".tr";
      link_gateway_2_gateway.EnableAscii(ascii.CreateFileStream (oss.str()),host_nodes[i][j]);
      // connect the host to the edge switch
      for (uint32_t z = 0; z < num_hosts_per_switch; z++)
      {
    	//set the address base for the hosts connecting to the (i,j) th edge switch
		oss.str("");
		oss << z+2 << "." << i << "." << j << ".0";
		address.SetBase(oss.str().c_str(),"255.255.255.0");
		//connect the host to the responding edge switch
		host_net_devices[i][j][z] = link_host_2_edgeSwitch.Install(host_nodes[i][j].Get(z),edge_switch_nodes[i].Get(j));
		//assign ip
		host_ipv4_interfaces[i][j][z] = address.Assign(host_net_devices[i][j][z]);

		//debug
		std::cout << "host - edge switch: " << "(" << i << "," << j << " -- " << z << ") ";
		host_ipv4_interfaces[i][j][z].GetAddress(0,0).Print(std::cout);
		std::cout << "--" ;
		host_ipv4_interfaces[i][j][z].GetAddress(1,0).Print(std::cout);
		std::cout << std::endl;
      }
    }
  }

  // ----------------------------install on-off application-----------------------------------
  /* install ON-OFF application and packet sink application in every hosts
   * every node will be installed with an ON-OFF application and packet sink application
   * the destination of the ON-OFF application is local with probability of 0.5 and is remote with 0.5
   */
  NS_LOG_INFO("Install ON-OFF application");
  Ptr<UniformRandomVariable> random_num_generator = CreateObject<UniformRandomVariable>();

/*  // debug
  PacketSinkHelper sink_helper("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));
  ApplicationContainer sink_app;
  sink_app = sink_helper.Install(host_nodes[0][0].Get(0));
  sink_app.Start(Seconds(0.0));
  OnOffHelper on_off_helper("ns3::UdpSocketFactory", Address());
  AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[0][0][0].GetAddress(0,0),9999));
  on_off_helper.SetAttribute("Remote", remote_address);
  on_off_helper.SetAttribute("MaxBytes",UintegerValue(512));
  ApplicationContainer on_off_app;
  on_off_app = on_off_helper.Install(host_nodes[0][1].Get(1));
 // on_off_app = on_off_helper.Install(edge_switch_nodes[0].Get(0));
  on_off_app.Start(Seconds(random_num_generator->GetValue()));*/


  for (uint32_t i = 0; i < num_gateway_switches; i++)
  {
    for (uint32_t j = 0; j < num_switches_per_subnet; j++)
    {
      for (uint32_t z = 0; z < num_hosts_per_switch; z++)
      {
		//PacketSinkHelper sink_helper("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));
		PacketSinkHelper sink_helper("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));

		ApplicationContainer sink_app;
		sink_app = sink_helper.Install(host_nodes[i][j].Get(z));
		sink_app.Start(Seconds(0.0));

		//install the on-off application in the host
		if (random_num_generator->GetValue() < prob_remote_app)
		{
		  //the destination of the on-off app is remote
		  int i_remote, j_remote, z_remote;
		  //choose the subnet of destination randomly
		  while (true)
		  {
			i_remote = (int) num_gateway_switches * random_num_generator->GetValue();
			if (i_remote != i)
			  break;
		  }
		  //choose the edge switch of destination randomly
		  j_remote = (int) num_switches_per_subnet * random_num_generator->GetValue();
		  //choose the host (i.e., destination) randomly
		  z_remote = (int) num_hosts_per_switch * random_num_generator->GetValue();
		  OnOffHelper on_off_helper("ns3::UdpSocketFactory", Address());
		  AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[i_remote][j_remote][z_remote].GetAddress(0,0),9999));
		  on_off_helper.SetAttribute("Remote", remote_address);
		  on_off_helper.SetAttribute("MaxBytes",UintegerValue(512));

		  //debug
		  uint8_t buffer[20];
		  uint32_t k_temp;
		  k_temp = remote_address.Get().CopyTo(buffer);
		  std::cout<< "(" << i << "," << j << ", " << z << ") -- " << "(" << i_remote << "," << j_remote << ", " << z_remote << "): " << \
		  unsigned(buffer[0]) << "." << unsigned(buffer[1]) << "." << unsigned(buffer[2]) << "." << unsigned(buffer[3]) <<std::endl;


		  ApplicationContainer on_off_app;
		  on_off_app = on_off_helper.Install(host_nodes[i][j].Get(z));
		  on_off_app.Start(Seconds(random_num_generator->GetValue()));
		 // on_off_app.Stop(Seconds(5.0));
		}
		else
		{

		  //the destination of the on-off app is local
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

			//debug
			std::cout << num_gateway_switches * (num_switches_per_subnet + 1) + \
			  i * num_switches_per_subnet * num_hosts_per_switch + j * num_hosts_per_switch + z << std::endl;
		  }
		  else
		  {
				z_local = (int) num_hosts_per_switch * random_num_generator->GetValue();
		  }

		  //debug
          if (i == 0 && j == 0 && z == 0)
          {
        	  j_local = 0;
        	  z_local = 1;
          }

		  OnOffHelper on_off_helper("ns3::UdpSocketFactory", Address());
		  AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[i][j_local][z_local].GetAddress(0,0),9999));

		  //debug
		  uint8_t buffer[20];
		  uint32_t k_temp;
		  k_temp = remote_address.Get().CopyTo(buffer);
		  std::cout<< "(" << i << "," << j << ", " << z << ") -- " << "(" << i << "," << j_local << ", " << z_local << "): " << \
				unsigned(buffer[0]) << "." << unsigned(buffer[1]) << "." << unsigned(buffer[2]) << "." << unsigned(buffer[3]) <<std::endl;

		  on_off_helper.SetAttribute("Remote", remote_address);
		  on_off_helper.SetAttribute("DataRate",StringValue("1Kbps"));
		  on_off_helper.SetAttribute("MaxBytes",UintegerValue(512));
		  ApplicationContainer on_off_app;
		  on_off_app = on_off_helper.Install(host_nodes[i][j].Get(z));
		  on_off_app.Start(Seconds(random_num_generator->GetValue()));
		 // on_off_app.Stop(Seconds(5.0));
		}
      }
    }
  }

  std::cout << "Running simulator ... " << std::endl;
 // link_host_2_edgeSwitch.EnablePcapAll("gateway_2_gateway");
 // AsciiTraceHelper ascii;
  link_host_2_edgeSwitch.EnableAsciiAll (ascii.CreateFileStream ("PADS-noController-v3.tr"));

  // Calculate routing tables
  std::cout << "Populating Routing tables..." << std::endl;
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//  Simulator::Stop(Seconds(20.0));
  Simulator::Run();
  Simulator::Destroy();
  std::cout << "Simulation is finished" << std::endl;
  return 0;
}











