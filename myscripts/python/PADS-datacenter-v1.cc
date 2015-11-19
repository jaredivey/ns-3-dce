/* Network topology
 * there are num_core_switch core switches, num_edge_switch edge switches
 * each edge switch connects to every core switch, and is connected to num_host hosts
 * all edge switches and core switches are connected to a controller
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

typedef struct timeval TIMER_TYPE;
#define TIMER_NOW(_t) gettimeofday (&_t,NULL);
#define TIMER_SECONDS(_t) ((double)(_t).tv_sec + (_t).tv_usec * 1e-6)
#define TIMER_DIFF(_t1, _t2) (TIMER_SECONDS (_t1) - TIMER_SECONDS (_t2))

enum CONTROLLER
{
  RYU,
  POX,
} CONTROLLER;

NS_LOG_COMPONENT_DEFINE ("PADS-datacenter-v1");

int main (int argc, char *argv[])
{
  typedef std::vector<NodeContainer> vector_of_NodeContainer;
  typedef std::vector<vector_of_NodeContainer> vector_of_vector_of_NodeContainer;

  typedef std::vector<Ipv4InterfaceContainer> vector_of_Ipv4InterfaceContainer;
  typedef std::vector<vector_of_Ipv4InterfaceContainer> vector_of_vector_of_Ipv4InterfaceContainer;
  typedef std::vector<vector_of_vector_of_Ipv4InterfaceContainer> vector_of_vector_of_vector_of_Ipv4InterfaceContainer;

  typedef std::vector<NetDeviceContainer> vector_of_NetDeviceContainer;
  typedef std::vector<vector_of_NetDeviceContainer> vector_of_vector_of_NetDeviceContainer;
  typedef std::vector<vector_of_vector_of_NetDeviceContainer> vector_of_vector_of_vector_of_NetDeviceContainer;

  uint32_t maxBytes = 5120;
  uint32_t num_core_switch = 5; 		// number of switch nodes in the core ring network
  uint32_t num_edge_switch = 10; // number of edge switch nodes in one subnet
  uint32_t num_host    = 100; 	// number of hosts connected with one edge switch
  uint32_t num_controller = 1; // number of controllers
  uint32_t controller = POX;
  uint32_t min_start_time = 50; // minimal starting time for on-off application
  uint32_t start_time_interval = 30 ; // the interval between minimal starting time and maximal starting time

  bool verbose =  false;  // log information level indication in ryu application

  std::ostringstream oss;

  CommandLine cmd;
  cmd.AddValue ("maxBytes",
			  "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("num_core_switch", "Number of core switches", num_core_switch);
  cmd.AddValue ("num_edge_switch", "Number of edge switches", num_edge_switch);
  cmd.AddValue ("num_host", "number of hosts connected with one edge switch ", num_host);
  cmd.Parse (argc,argv);

  // enable log component
  if (verbose)
  {
	  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
	  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
	  LogComponentEnable ("PointToPointChannel", LOG_LEVEL_INFO);
	  LogComponentEnable ("PADS-datacenter-v1", LOG_LEVEL_INFO);
  }


  TIMER_TYPE t0, t1, t2;
  TIMER_NOW (t0);

  // ---------------------------define nodes-------------------------------------
  NodeContainer core_switch_nodes, edge_switch_nodes, controller_nodes;
  vector_of_NodeContainer host_nodes(num_edge_switch);

  // ---------------------------define net device--------------------------------
  vector_of_NetDeviceContainer coreSwitch_2_controller_net_device(num_core_switch);
  vector_of_NetDeviceContainer edgeSwitch_2_controller_net_device(num_edge_switch);
  vector_of_vector_of_NetDeviceContainer edgeSwitch_2_coreSwitch_net_device(num_core_switch,vector_of_NetDeviceContainer(num_edge_switch));
  vector_of_vector_of_NetDeviceContainer host_2_edgeSwitch_net_device(num_edge_switch,vector_of_NetDeviceContainer(num_host));

  // ---------------------------define ipv4 interface----------------------------
  vector_of_Ipv4InterfaceContainer coreSwitch_2_controller_ipv4_interface(num_core_switch);
  vector_of_Ipv4InterfaceContainer edgeSwitch_2_controller_ipv4_interface(num_edge_switch);
  vector_of_vector_of_Ipv4InterfaceContainer edgeSwitch_2_coreSwitch_ipv4_interface(num_core_switch,vector_of_Ipv4InterfaceContainer(num_edge_switch));
  vector_of_vector_of_Ipv4InterfaceContainer host_2_edgeSwitch_ipv4_interface(num_edge_switch,vector_of_Ipv4InterfaceContainer(num_host));



  // ---------------------------define links------------------------------------
  Layer2P2PHelper link_host_2_edgeSwitch, link_edgeSwitch_2_coreSwitch;
  PointToPointHelper link_switch_2_controller;
  link_host_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("200Mbps"));
  link_host_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("5ms"));
  link_edgeSwitch_2_coreSwitch.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_edgeSwitch_2_coreSwitch.SetChannelAttribute ("Delay", StringValue ("5ms"));
  link_switch_2_controller.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_switch_2_controller.SetChannelAttribute ("Delay", StringValue ("5ms"));

  // --------------------------create nodes--------------------------------------
  NS_LOG_INFO("create controller nodes");
  controller_nodes.Create(num_controller);

  NS_LOG_INFO("create core switch nodes");
  core_switch_nodes.Create(num_core_switch);

  NS_LOG_INFO("create edge switch nodes");
  edge_switch_nodes.Create(num_edge_switch);

  NS_LOG_INFO("create host nodes");
  for (int i = 0; i < num_edge_switch; i++)
  {
	host_nodes[i].Create(num_host);
  }

  //-------------------------connect nodes---------------------------------------
  //-------connect hosts to edge switch----------
  for (int i = 0; i < num_edge_switch; i++)
  {
	  for (int j = 0; j < num_host; j++)
	  {
		  host_2_edgeSwitch_net_device[i][j] = link_host_2_edgeSwitch.Install(host_nodes[i].Get(j),edge_switch_nodes.Get(i));
	  }
  }
  //-------connect edge switch to core switch-----
  for (int i = 0; i < num_core_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  edgeSwitch_2_coreSwitch_net_device[i][j] = link_edgeSwitch_2_coreSwitch.Install(edge_switch_nodes.Get(j),core_switch_nodes.Get(i));
	  }
  }
  //------connect core switch to controller--------
  for (int i = 0; i < num_core_switch; i++)
  {
	  coreSwitch_2_controller_net_device[i] = link_switch_2_controller.Install(core_switch_nodes.Get(i),controller_nodes.Get(0));
  }
  //------connect edge switch to controller--------
  for (int i = 0; i < num_edge_switch; i++)
  {
	  edgeSwitch_2_controller_net_device[i] = link_switch_2_controller.Install(edge_switch_nodes.Get(i),controller_nodes.Get(0));
  }


  //-------------------------install stack in all nodes and dce manager in controller nodes---------
  DceManagerHelper dceManager;
  dceManager.SetVirtualPath ("/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/ryu");
  InternetStackHelper stack;
  // install dce manager in controller nodes
  NS_LOG_INFO("install dce manager in the controller nodes");
  dceManager.Install(controller_nodes);
  //install stack in the controller nodes
  NS_LOG_INFO("install stack in the controller nodes");
  stack.Install(controller_nodes);
  // install stack in the core switch nodes
  NS_LOG_INFO("install stack in the core switch nodes");
  stack.Install(core_switch_nodes);
  // install stack in the edge switch nodes
  NS_LOG_INFO("install stack in the edge switch nodes");
  stack.Install(edge_switch_nodes);
  // install stack in the host nodes
  NS_LOG_INFO("install stack in the host nodes");
  for (int i = 0; i < num_edge_switch; i++)
  {
	  stack.Install(host_nodes[i]);
  }


  //-------------------------assign the ip address-------------------------
  NS_LOG_INFO("Assign non controller link IP Address");
  Ipv4AddressHelper ipv4;
  Ipv4InterfaceContainer ipv4_address_container;

  oss.str ("");
  oss << "10.0.0.0";
  ipv4.SetBase (oss.str ().c_str (), "255.0.0.0");

  for (int i = 0; i < num_edge_switch; i++)
  {
	  for (int j = 0; j < num_host; j++)
	  {
		  host_2_edgeSwitch_ipv4_interface[i][j] = ipv4.Assign(host_2_edgeSwitch_net_device[i][j]);
		  NS_LOG_INFO("Host (" << i << "," << j << ") ip address for host--edgeSwitch link: " << host_2_edgeSwitch_ipv4_interface[i][j].GetAddress(0));
	  }
  }

  for (int i = 0; i < num_core_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  edgeSwitch_2_coreSwitch_ipv4_interface[i][j] = ipv4.Assign(edgeSwitch_2_coreSwitch_net_device[i][j]);
		  NS_LOG_INFO("Edge Switch (" << i << "," << j << ") ip address for edgeSwitch--coreSwitch link: " << edgeSwitch_2_coreSwitch_ipv4_interface[i][j].GetAddress(0));
	  }
  }

  NS_LOG_INFO("Assign controller link IP Address");
  for (int i = 0 ; i < num_core_switch; i++)
  {
	  oss.str ("");
	  oss << "193.0" << i + 1 << ".0";
	  ipv4.SetBase(oss.str().c_str(),"255.255.255.0");
	  coreSwitch_2_controller_ipv4_interface[i] = ipv4.Assign(coreSwitch_2_controller_net_device[i]);
  }

  for (int i = 0; i < num_edge_switch; i++)
  {
	  oss.str ("");
	  oss << "194.0" << i + 1 << ".0";
	  ipv4.SetBase(oss.str().c_str(),"255.255.255.0");
	  edgeSwitch_2_controller_ipv4_interface[i] = ipv4.Assign(edgeSwitch_2_controller_net_device[i]);
  }

  // -----------------------------install application-------------------------
  Ptr<UniformRandomVariable> random_num_generator = CreateObject<UniformRandomVariable>();
  ApplicationContainer sink_apps;
  for (int i = 0; i < num_edge_switch; i++)
  {
	  for (int j = 0; j < num_host; j++)
	  {
		  // install the sink in the host
		  PacketSinkHelper sink_helper("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));
		  sink_apps.Add(sink_helper.Install(host_nodes[i].Get(j)));

		  // install on-off application on the node (i_remote, j_remote) whose destination host is (i,j)
		  int i_remote, j_remote;
		  i_remote = (int) num_edge_switch * random_num_generator->GetValue();
		  if (i_remote == i)
		  {
			  while (true)
			  {
				  j_remote = (int) num_host * random_num_generator->GetValue();
				  if (j_remote != j)
					  break;
			  }
		  }
		  else
		  {
			  j_remote = (int) num_host * random_num_generator->GetValue();
		  }

		  OnOffHelper on_off_helper("ns3::UdpSocketFactory", Address());
		  AddressValue remote_address(InetSocketAddress(host_2_edgeSwitch_ipv4_interface[i_remote][j_remote].GetAddress(0),9999));

		  uint8_t buffer[20];
		  uint32_t k_temp;
		  k_temp = remote_address.Get().CopyTo(buffer);
		  std::cout<< "(" << i << "," << j << ") -- " << "(" << i_remote << "," << j_remote << "): " << \
					  host_2_edgeSwitch_ipv4_interface[i][j].GetAddress(0) << " -- " << \
					  unsigned(buffer[0]) << "." << unsigned(buffer[1]) << "." << unsigned(buffer[2]) << "." << unsigned(buffer[3]) <<std::endl;

		  on_off_helper.SetAttribute("Remote", remote_address);
		  on_off_helper.SetAttribute("DataRate",StringValue("1Kbps"));
		  on_off_helper.SetAttribute("MaxBytes",UintegerValue(maxBytes));
		  ApplicationContainer on_off_app;
		  on_off_app = on_off_helper.Install(host_nodes[i].Get(j));
		  on_off_app.Start(Seconds(start_time_interval * random_num_generator->GetValue() + min_start_time));

	  }
  }
  sink_apps.Start(Seconds(0.0));

  // ---------------------------------install the controller application--------------------------
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
		 dce.AddArgument ("openflow.discovery");
		 dce.AddArgument ("openflow.spanning_tree");
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

  // -------------------------------------install the sdn switch application in all switch nodes--------------------
  NS_LOG_INFO("install the sdn switch application on core switches");
  for (uint32_t i = 0; i < num_core_switch; i ++ )
  {
	Ptr<SdnSwitch> sdnS = CreateObject<SdnSwitch> ();
	sdnS->SetStartTime (Seconds (i + 1.0 + 0.1*(double)(i)));
	core_switch_nodes.Get (i)->AddApplication (sdnS);
  }
  NS_LOG_INFO("install the sdn switch application on edge switches");
  for (uint32_t i = 0; i < num_edge_switch; i ++ )
  {
	Ptr<SdnSwitch> sdnS = CreateObject<SdnSwitch> ();
	sdnS->SetStartTime (Seconds (i + 1.0 + 0.1*(double)(i)));
	edge_switch_nodes.Get (i)->AddApplication (sdnS);
  }

  std::cout << "Running simulator ... " << std::endl;
  //Simulator::Stop(Seconds(10.0));
	TIMER_NOW (t1);
	Simulator::Run();
	TIMER_NOW (t2);

	uint32_t totalRx = 0;
	for (uint32_t i = 0; i < sink_apps.GetN(); ++i)
	{
	 Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sink_apps.Get (i));
	 if (sink1)
	   {
		 totalRx += sink1->GetTotalRx();
	   }
	}
	double simulate_time = TIMER_DIFF (t1, t0) + TIMER_DIFF (t2, t1);

	std::cout << "total received bytes = " << totalRx << std::endl;
	std::cout << "total sended bytes = " << maxBytes * num_edge_switch * num_host << std::endl;
	std::cout << "total simulation time = " << simulate_time << std::endl;
	Simulator::Destroy();

	std::cout << "Simulation is finished" << std::endl;
	return 0;


}
