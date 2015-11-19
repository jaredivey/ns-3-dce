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

  uint32_t num_gateway_switches    = 4;
  uint32_t num_switches_per_subnet = 4;
  uint32_t num_hosts_per_switch    = 3;
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
  vector_of_NodeContainer gateway_switch_nodes(num_gateway_switches);
  vector_of_NodeContainer gateway_edge_nodes(num_gateway_switches);
  vector_of_vector_of_NodeContainer edge_switch_nodes(num_gateway_switches,vector_of_NodeContainer(num_switches_per_subnet));
  vector_of_vector_of_vector_of_NodeContainer host_nodes(num_gateway_switches,vector_of_vector_of_NodeContainer(num_switches_per_subnet,vector_of_NodeContainer(num_hosts_per_switch)));
  
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
   
  // create the nodes in the core network
  for (uint32_t i = 0; i < num_gateway_switches; i++)
  {
    Ptr<Node> node = CreateObject<Node>();
    gateway_switch_nodes[i].Add (node);
    stack.Install(gateway_switch_nodes[i]);
  }
  for (uint32_t i = 0; i < num_gateway_switches-1; i++)
  {
    gateway_switch_nodes[i].Add(gateway_switch_nodes[i+1].Get(0));
  }
  gateway_switch_nodes[num_gateway_switches-1].Add(gateway_switch_nodes[0].Get(0));

  //create the ring connecting the gateway switch in the core network
  for (uint32_t i = 0; i < num_gateway_switches; i++)
  {
	//set the address base for the core network
    oss.str("");
    oss << "251.168." << i << ".0";
    address.SetBase (oss.str().c_str(), "255.255.255.0");
    gateway_switch_net_devices[i] = link_gateway_2_gateway.Install(gateway_switch_nodes[i]);
    gateway_switch_ipv4_interfaces[i] = address.Assign(gateway_switch_net_devices[i]);
  } 

  for (uint32_t i = 0; i < num_gateway_switches; i++)
    {
      NS_LOG_INFO("Create the " << i <<"th subnetwork, and assign ip");
      //create the nodes in the ith subnet
      for (uint32_t j = 0; j < num_switches_per_subnet; j++)
        {
          Ptr<Node> node = CreateObject<Node>();
          edge_switch_nodes[i][j].Add (node);
          stack.Install(edge_switch_nodes[i][j]);
        }
      for (uint32_t j = 0; j < num_switches_per_subnet-1; j++)
        {
          edge_switch_nodes[i][j].Add (edge_switch_nodes[i][j+1].Get(0));
        }
      edge_switch_nodes[i][num_switches_per_subnet-1].Add (edge_switch_nodes[i][0].Get(0));
      //set the address base for the ith subnet

      //create the rings connecting the nodes in the ith subnet
      for (uint32_t j = 0; j < num_switches_per_subnet; j++)
      {
    	  oss.str("");
    	  oss << "1." << i << "." << j << ".0";
    	  address.SetBase(oss.str().c_str(),"255.255.255.0");
    	  edge_switch_net_devices[i][j] = link_edgeSwitch_2_edgeSwitch.Install (edge_switch_nodes[i][j]);
    	  edge_switch_ipv4_interfaces[i][j] = address.Assign(edge_switch_net_devices[i][j]);
      }
      //connect the subnet to the gateway switch
      NS_LOG_INFO("Connect the " << i << "th  subnetwork to the" << i << "th gateway switch, and assign ip");  
      gateway_edge_nodes[i].Add(gateway_switch_nodes[i].Get(0));
      gateway_edge_nodes[i].Add(edge_switch_nodes[i][0].Get(0));
      gateway_edge_net_devices[i] = link_edgeSwitch_2_gateway.Install(gateway_edge_nodes[i]);

      oss.str("");
      oss << "1." << num_gateway_switches + i << "." << num_switches_per_subnet + 2 << ".0";
      address.SetBase(oss.str().c_str(),"255.255.255.0");
      gateway_edge_ipv4_interfaces[i] = address.Assign(gateway_edge_net_devices[i]);
    }
  //create the host nodes
  NS_LOG_INFO("Create the hosts, connect the hosts to edge switches, and assign ip");
  for (uint32_t i = 0; i < num_gateway_switches; i++)
  {
    for (uint32_t j = 0; j < num_switches_per_subnet; j++)
    {
      for (uint32_t z = 0; z < num_hosts_per_switch; z++)
      {
    	//set the address base for the hosts connecting to the (i,j) th edge switch
		oss.str("");
		oss << z+2 << "." << i << "." << j << ".0";
		address.SetBase(oss.str().c_str(),"255.255.255.0");
		Ptr<Node> node = CreateObject<Node>();
		host_nodes[i][j][z].Add(node);
		stack.Install(host_nodes[i][j][z]);
		//connect the host to the responding edge switch
		host_nodes[i][j][z].Add(edge_switch_nodes[i][j].Get(0));
		host_net_devices[i][j][z] = link_host_2_edgeSwitch.Install(host_nodes[i][j][z]);
		//assign ip
		host_ipv4_interfaces[i][j][z] = address.Assign(host_net_devices[i][j][z]);
      }
    }
  }
  /* install ON-OFF application and packet sink application in every hosts
   * every node will be installed with an ON-OFF application and packet sink application
   * the destination of the ON-OFF application is local with probability of 0.5 and is remote with 0.5
   */
  NS_LOG_INFO("Install ON-OFF application");
  Ptr<UniformRandomVariable> random_num_generator = CreateObject<UniformRandomVariable>();
  for (uint32_t i = 0; i < num_gateway_switches; i++)
  {
    for (uint32_t j = 0; j < num_switches_per_subnet; j++)
    {
      for (uint32_t z = 0; z < num_hosts_per_switch; z++)
      {
  	//PacketSinkHelper sink_helper("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));
	PacketSinkHelper sink_helper("ns3::TcpSocketFactory",InetSocketAddress(host_ipv4_interfaces[i][j][z].GetAddress(0),9999));
        
	ApplicationContainer sink_app;
	sink_app = sink_helper.Install(host_nodes[i][j][z].Get(0));
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
	  OnOffHelper on_off_helper("ns3::TcpSocketFactory", Address());
	  AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[i_remote][j_remote][z_remote].GetAddress(0.0),9999));
	  on_off_helper.SetAttribute("Remote", remote_address);
	  on_off_helper.SetAttribute("MaxBytes",UintegerValue(512));
	  //debug
	  on_off_helper.SetAttribute("DataRate",StringValue("1Kbps"));
	  ApplicationContainer on_off_app;
	  on_off_app = on_off_helper.Install(host_nodes[i][j][z].Get(0));
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
	 
	  OnOffHelper on_off_helper("ns3::TcpSocketFactory", Address());
	  AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[i][j_local][z_local].GetAddress(0),9999));
	  on_off_helper.SetAttribute("Remote", remote_address);
	  on_off_helper.SetAttribute("DataRate",StringValue("1Kbps"));
	  on_off_helper.SetAttribute("MaxBytes",UintegerValue(512));
          ApplicationContainer on_off_app;
	  on_off_app = on_off_helper.Install(host_nodes[i][j][z].Get(0));
	  on_off_app.Start(Seconds(random_num_generator->GetValue()));
	 // on_off_app.Stop(Seconds(5.0));
	}
      }
    }
  }
  std::cout << "Running simulator ... " << std::endl;
  link_gateway_2_gateway.EnablePcapAll("gateway_2_gateway");
  // Calculate routing tables
  std::cout << "Populating Routing tables..." << std::endl;
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//  Simulator::Stop(Seconds(20.0));
  Simulator::Run();
  Simulator::Destroy();
  std::cout << "Simulation is finished" << std::endl;
  return 0;
}







