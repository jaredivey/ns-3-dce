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
 * A ring network consisting numSwitchsPerSubnet Switch(S) nodes,
 * all S nodes are connected to one controller,
 * and each S node connects to numHostsPerSwitch hosts
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

enum CONTROLLER
{
  RYU,
  POX,
} CONTROLLER;

#define REALLY_BIG_TIME 1000000

typedef struct timeval TIMER_TYPE;
#define TIMER_NOW(_t) gettimeofday (&_t,NULL);
#define TIMER_SECONDS(_t) ((double)(_t).tv_sec + (_t).tv_usec * 1e-6)
#define TIMER_DIFF(_t1, _t2) (TIMER_SECONDS (_t1) - TIMER_SECONDS (_t2))

NS_LOG_COMPONENT_DEFINE ("PADS-Controller-simple");

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
  uint32_t num_hosts_per_switch    = 2;
  uint32_t num_controller = 1;
  uint32_t controller = RYU;
  bool verbose = true;

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
  LogComponentEnable ("PADS-Controller-simple", LOG_LEVEL_INFO);
//  LogComponentEnable ("SdnSwitch", LOG_LEVEL_INFO);

  TIMER_TYPE t0, t1, t2;
  TIMER_NOW (t0);

  //define nodes
  NodeContainer edge_switch_nodes;
  NodeContainer controller_nodes;
  vector_of_NodeContainer host_nodes(num_switches_per_subnet);

  //define ipv4 interface
  vector_of_Ipv4InterfaceContainer edge_switch_ipv4_interfaces(num_switches_per_subnet);
  vector_of_Ipv4InterfaceContainer controller_ipv4_interfaces(num_switches_per_subnet);
  vector_of_vector_of_Ipv4InterfaceContainer host_ipv4_interfaces(num_switches_per_subnet,vector_of_Ipv4InterfaceContainer(num_hosts_per_switch));

  //define net device
  vector_of_NetDeviceContainer edge_switch_net_devices(num_switches_per_subnet);
  vector_of_NetDeviceContainer controller_net_devices (num_switches_per_subnet);
  vector_of_vector_of_NetDeviceContainer host_net_devices(num_switches_per_subnet,vector_of_NetDeviceContainer(num_hosts_per_switch));


  //define links
  // NS_LOG_INFO ("Create channels.");
  Layer2P2PHelper link_edgeSwitch_2_edgeSwitch, link_host_2_edgeSwitch;
  link_edgeSwitch_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_edgeSwitch_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("5ms"));
  link_host_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("200Mbps"));
  link_host_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("2ms"));

  PointToPointHelper link_controller_2_switch;
  link_controller_2_switch.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  link_controller_2_switch.SetChannelAttribute ("Delay", StringValue ("1ms"));

  //define the routing protocol, use nix vector routing protocol
  InternetStackHelper stack;

  //define the ipv4 address helper
  Ipv4AddressHelper address;
  std::ostringstream oss;
  controller_nodes.Create(num_controller);

  NS_LOG_INFO("Create the edge switches, and connect these switches by ring");

  //--------------------------------------create the edge switch networks---------------------------------
  // create edge switch nodes
  edge_switch_nodes.Create(num_switches_per_subnet);
  stack.Install(edge_switch_nodes);

  //set the address base
	oss.str("");
	oss << "10.0.0.0";
	address.SetBase(oss.str().c_str(),"255.255.255.0");

  //create the rings connecting the edge switch nodes
  for (uint32_t j = 0; j < num_switches_per_subnet; j++)
  {
	if (j > 0)
	{
		edge_switch_net_devices[j] = link_edgeSwitch_2_edgeSwitch.Install
					(NodeContainer(edge_switch_nodes.Get(j-1), edge_switch_nodes.Get(j)));
	}
	else
	{
		edge_switch_net_devices[j] = link_edgeSwitch_2_edgeSwitch.Install
							(NodeContainer(edge_switch_nodes.Get(num_switches_per_subnet-1), edge_switch_nodes.Get(0)));
	}

    edge_switch_ipv4_interfaces[j] = address.Assign(edge_switch_net_devices[j]);

    //debug
    //std::ostream os;
    std::cout << "edge switch - edge switch: " << j << "-" << j+1 << "  ";
    edge_switch_ipv4_interfaces[j].GetAddress(0,0).Print(std::cout);
    std::cout << "--" ;
    edge_switch_ipv4_interfaces[j].GetAddress(1,0).Print(std::cout);
    std::cout << std::endl;
  }

  //-----------------------------------create the hosts ----------------------------------
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
/*
 		//set the address base for the hosts connecting to the (i,j) th edge switch
 		oss.str("");
 		oss << "143." << z << "." << j << ".0";
 		address.SetBase(oss.str().c_str(),"255.255.255.0");
*/
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

  // ---------------------------------------create the controller-------------------------
  NS_LOG_INFO("Create the controller");
  DceManagerHelper dceManager;
  dceManager.SetVirtualPath ("/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/ryu");

  // create controller nodes
  //controller_nodes.Create(num_controller);
  dceManager.Install (controller_nodes);
  stack.Install(controller_nodes);
  NS_LOG_INFO("connect the controller to the edge switches");
  // connect each edge switch node to the controller
  for (uint32_t i = 0; i < num_switches_per_subnet; i++)
  {
	  controller_net_devices[i] = link_controller_2_switch.Install(edge_switch_nodes.Get(i),controller_nodes.Get(0));
	  //set the address base for the hosts connecting to the (i,j) th edge switch
	  oss.str("");
	  oss << "144.0." << i << ".0";
	  address.SetBase(oss.str().c_str(),"255.255.255.0");
	  controller_ipv4_interfaces[i] = address.Assign(controller_net_devices[i]);
  }


  //---------------------------------install application -------------------------------------
  /* install ON-OFF application and packet sink application in every hosts
   * every node will be installed with an ON-OFF application and packet sink application
   * the destination of the ON-OFF application is local with probability of 0.5 and is remote with 0.5
   */


  NS_LOG_INFO("Install ON-OFF application");
  Ptr<UniformRandomVariable> random_num_generator = CreateObject<UniformRandomVariable>();
  random_num_generator->SetAttribute ("Min", DoubleValue(30));
  random_num_generator->SetAttribute ("Max", DoubleValue(60));

  ApplicationContainer sinkApps, on_off_app_container;

//  // -----------------debug start-----------------------
//  PacketSinkHelper sink_helper("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));
//  ApplicationContainer sink_app;
//  sinkApps.Add(sink_helper.Install(host_nodes[0].Get(0)));
//  //sink_app.Start(Seconds(0.0));
//  OnOffHelper on_off_helper("ns3::UdpSocketFactory", Address());
//  AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[0][0].GetAddress(0,0),9999));
//  on_off_helper.SetAttribute("Remote", remote_address);
//  on_off_helper.SetAttribute("MaxBytes",UintegerValue(512));
//  ApplicationContainer on_off_app;
//  //on_off_app = on_off_helper.Install(host_nodes[0].Get(1));
//  on_off_app = on_off_helper.Install(host_nodes[1].Get(0));
//  on_off_app.Start(Seconds(random_num_generator->GetValue()));
//  //------------------debug end----------------------

  for (uint32_t j = 0; j < num_switches_per_subnet; j++)
  {
	  for (uint32_t z = 0; z < num_hosts_per_switch; z++)
	  {
		PacketSinkHelper sink_helper("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));
		//PacketSinkHelper sink_helper("ns3::TcpSocketFactory",InetSocketAddress(host_ipv4_interfaces[i][j][z].GetAddress(0),9999));

		sinkApps.Add(sink_helper.Install(host_nodes[j].Get(z)));

		//install the on-off application in the host
		int j_local, z_local;

		/*
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
		}*/


		if (j == 0)
			j_local = num_switches_per_subnet - 1;
		else
			j_local = j -1;
		if (z == 0)
			z_local = num_hosts_per_switch - 1;
		else
			z_local = z -1;


		OnOffHelper on_off_helper("ns3::TcpSocketFactory", Address());
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
	//	on_off_app_container.Add(on_off_app);
		on_off_app.Start(Seconds(random_num_generator->GetValue()));
	  }
  }
  sinkApps.Start(Seconds(0.0));

  // -----------------------------installer the controller application------------------------------------
  NS_LOG_INFO("install the controller application");
  DceApplicationHelper dce;
  ApplicationContainer apps;

   for (uint32_t i = 0; i < controller_nodes.GetN (); ++i)
     {
       dce.SetStackSize (1<<20);

 //    Python controllers
       dce.SetBinary ("python2-dce");
       dce.ResetArguments ();
       dce.ResetEnvironment ();
       dce.AddEnvironment ("PATH", "/:/python2.7:/pox:/ryu");
       dce.AddEnvironment ("PYTHONHOME", "/:/python2.7:/pox:/ryu");
       dce.AddEnvironment ("PYTHONPATH", "/:/python2.7:/pox:/ryu");
 //      dce.AddArgument ("-v");
       dce.AddArgument ("-S");

       if (controller==RYU)
         {
 //        Ryu arguments
           dce.AddArgument ("ryu-manager");
           if (verbose)
             {
               dce.AddArgument ("--verbose");
             }
           dce.AddArgument ("ryu/app/simple_switch_stp.py");
         }
       else if (controller==POX)
         {
 //        POX arguments
           dce.AddArgument ("pox/pox.py");
           dce.AddArgument ("--unthreaded-sh");
           dce.AddArgument ("log.level");
           if (verbose)
             {
               dce.AddArgument ("--DEBUG");
             }
           else
             {
               dce.AddArgument ("--WARNING");
             }
           dce.AddArgument ("forwarding.l2_learning");
         }
       else
         {
           NS_LOG_ERROR ("Unsupported controller choice");
           return 0;
         }

       apps = dce.Install (controller_nodes.Get (i));
       apps.Start (Seconds (0.0));
     }
  // -----------------------------------install the sdn switch application in all edge switch nodes-------------------------
   NS_LOG_INFO("install the sdn switch application");
   for (uint32_t i = 0; i < num_switches_per_subnet; i ++ )
  {
	Ptr<SdnSwitch> sdnS = CreateObject<SdnSwitch> ();
	sdnS->SetStartTime (Seconds (i + 1.0 + 0.1*(double)(i)));
	edge_switch_nodes.Get (i)->AddApplication (sdnS);
  }

  std::cout << "Running simulator ... " << std::endl;
  link_host_2_edgeSwitch.EnablePcapAll("layer2P2P");
  link_controller_2_switch.EnablePcapAll("controller2switch");
  // Calculate routing tables
//  std::cout << "Populating Routing tables..." << std::endl;
 // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

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







