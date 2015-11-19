/* Network topology
 * a ring network consisting num_switch switch nodes (S)
 * every S node connected to a different ring subnet containing num_edge_switch switch nodes (ES)
 * every ES node connects with num_host hosts
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

NS_LOG_COMPONENT_DEFINE ("PADS-multiple-controller-v1");

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
  uint32_t num_switch = 4; 		// number of switch nodes in the core ring network
  uint32_t num_edge_switch = 10; // number of edge switch nodes in one subnet
  uint32_t num_host = 20; 	// number of hosts connected with one edge switch

  uint32_t controller = POX;
  uint32_t min_start_time = 50; // minimal starting time for on-off application
  uint32_t start_time_interval = 30; // the interval between minimal starting time and maximal starting time

  double prob_remote_app = 0.5; // probability of the event that the destination of an on-off app is remote
  bool verbose =  false;  // log information level indication in ryu application

  std::ostringstream oss;

  CommandLine cmd;
  cmd.AddValue ("maxBytes",
			  "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("num_switch", "Number of switches in the core ring network", num_switch);
  cmd.AddValue ("num_edge_switch", "Number of edge switches in a subnet", num_edge_switch);
  cmd.AddValue ("num_host", "number of hosts connected with one edge switch ", num_host);
  cmd.Parse (argc,argv);
  uint32_t num_controller = num_switch + 1; // number of controllers
  // enable log component

  if (verbose)
  {
	  LogComponentEnable ("PADS-multiple-controller-v1", LOG_LEVEL_INFO);
	  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
	  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
	  LogComponentEnable ("PointToPointChannel", LOG_LEVEL_INFO);
  }


  TIMER_TYPE t0, t1, t2;
  TIMER_NOW (t0);
  // ---------------------define nodes-----------------------
  NodeContainer switch_nodes, controller_nodes;
  vector_of_NodeContainer edge_switch_nodes(num_switch);
  vector_of_vector_of_NodeContainer host_nodes(num_switch,vector_of_NodeContainer(num_edge_switch));

  //----------------------define net device------------------
  vector_of_NetDeviceContainer switch_2_switch_net_device(num_switch);
  vector_of_NetDeviceContainer edgeSwitch_2_switch_net_device(num_switch);
  vector_of_vector_of_NetDeviceContainer edgeSwitch_2_edgeSwitch_net_device(num_switch,vector_of_NetDeviceContainer(num_edge_switch));
  vector_of_vector_of_vector_of_NetDeviceContainer host_2_edgeSwitch_net_device(num_switch,
		  vector_of_vector_of_NetDeviceContainer(num_edge_switch,vector_of_NetDeviceContainer(num_host)));
  vector_of_NetDeviceContainer switch_2_controller_net_device(num_switch);
  vector_of_vector_of_NetDeviceContainer edgeSwitch_2_controller_net_device(num_switch, vector_of_NetDeviceContainer(num_edge_switch));

  // ---------------------define ipv4 interface
  vector_of_vector_of_vector_of_Ipv4InterfaceContainer host_ipv4_interfaces(num_switch,vector_of_vector_of_Ipv4InterfaceContainer(num_edge_switch,vector_of_Ipv4InterfaceContainer(num_host)));

  // ---------------------define links-----------------------
  Layer2P2PHelper link_edgeSwitch_2_edgeSwitch, link_host_2_edgeSwitch, link_switch_2_switch, link_edgeSwitch_2_switch;

  link_switch_2_switch.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_switch_2_switch.SetChannelAttribute ("Delay", StringValue ("5ms"));

  link_edgeSwitch_2_switch.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_edgeSwitch_2_switch.SetChannelAttribute ("Delay", StringValue ("5ms"));

  link_edgeSwitch_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_edgeSwitch_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("5ms"));

  link_host_2_edgeSwitch.SetDeviceAttribute ("DataRate", StringValue ("200Mbps"));
  link_host_2_edgeSwitch.SetChannelAttribute ("Delay", StringValue ("2ms"));

  PointToPointHelper link_controller_2_switch;
  link_controller_2_switch.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  link_controller_2_switch.SetChannelAttribute ("Delay", StringValue ("1ms"));

  // ---------------------create nodes--------------------------
  NS_LOG_INFO("create controller nodes");
  controller_nodes.Create(num_controller);

  NS_LOG_INFO("create switch node in the core network");
  switch_nodes.Create(num_switch);

  NS_LOG_INFO("create edge switch nodes in subnets");
  for (int i = 0; i < num_switch; i++)
  {
    edge_switch_nodes[i].Create(num_edge_switch);
  }

  NS_LOG_INFO("create host nodes");
  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  host_nodes[i][j].Create(num_host);
	  }
  }

  // ----------------------------------------------connect nodes----------------------------------------------
  //---connect switch nodes in core ring network------
  NS_LOG_INFO("connect switch nodes");
  for (int i = 0; i < num_switch; i++)
  {
	  if (i == 0)
	  {
		  switch_2_switch_net_device[i] = link_switch_2_switch.Install
				  (switch_nodes.Get(num_switch-1), switch_nodes.Get(0));
	  }
	  else
	  {
		  switch_2_switch_net_device[i] = link_switch_2_switch.Install
		  				  (switch_nodes.Get(i-1), switch_nodes.Get(i));
	  }
  }
  //----connect edge switch nodes in subnet-------
  NS_LOG_INFO("connect edge switch nodes");
  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  if (j == 0)
		  {
			  edgeSwitch_2_edgeSwitch_net_device[i][j] = link_edgeSwitch_2_edgeSwitch.Install
			  				  (edge_switch_nodes[i].Get(num_edge_switch-1), edge_switch_nodes[i].Get(0));
		  }
		  else
		  {
			  edgeSwitch_2_edgeSwitch_net_device[i][j] = link_edgeSwitch_2_edgeSwitch.Install
			 			  				  (edge_switch_nodes[i].Get(j-1), edge_switch_nodes[i].Get(j));
		  }
	  }
  }
  //------connect edge switch nodes to the switch nodes-------
  NS_LOG_INFO("connect edge switch nodes to switch nodes");
  for (int i = 0; i < num_switch; i++)
  {
	  edgeSwitch_2_switch_net_device[i] = link_edgeSwitch_2_switch.Install
	  			  				  (edge_switch_nodes[i].Get(0), switch_nodes.Get(i));
  }
  //------connect host nodes to edge switch nodes-----------
  NS_LOG_INFO("connect host nodes to edge switch nodes");
  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  for (int z = 0; z < num_host; z++)
		  {
			  host_2_edgeSwitch_net_device[i][j][z] = link_host_2_edgeSwitch.Install
			  	  			  				  (host_nodes[i][j].Get(z), edge_switch_nodes[i].Get(j));
		  }
	  }
  }
  //-----connect the switch nodes to controller------
  NS_LOG_INFO("connect the switch nodes to controller");
  for (int i = 0; i < num_switch; i++)
  {
	  switch_2_controller_net_device[i] = link_controller_2_switch.Install
	  			  	  			  			  (switch_nodes.Get(i), controller_nodes.Get(0));
  }
  //------connect the edge switch nodes to controller------
  NS_LOG_INFO("connect the edge switch nodes to controller");
  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  edgeSwitch_2_controller_net_device[i][j] = link_controller_2_switch.Install
		  	  			  	  			  			  (edge_switch_nodes[i].Get(j), controller_nodes.Get(i+1));
	  }
  }

  // ------------------------------Install the stack in all nodes and dce manager in controller nodes------------------------
  DceManagerHelper dceManager;
  dceManager.SetVirtualPath ("/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/ryu");
  InternetStackHelper stack;
  // install dce manager in controller nodes
  NS_LOG_INFO("install dce manager in the controller nodes");
  dceManager.Install(controller_nodes);
  //install stack in the controller nodes
  NS_LOG_INFO("install stack in the controller nodes");
  stack.Install(controller_nodes);
  // install stack in the switch nodes
  NS_LOG_INFO("install stack in the switch nodes");
  stack.Install(switch_nodes);
  // install stack in the edge switch nodes
  NS_LOG_INFO("install stack in the edge switch nodes");
  for (int i = 0; i < num_switch; i++)
  {
	  stack.Install(edge_switch_nodes[i]);
  }
  // install stack in the host nodes
  NS_LOG_INFO("install stack in the host nodes");
  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  stack.Install(host_nodes[i][j]);
	  }
  }

  //--------------------------------assign the ip address---------------------------------
  NS_LOG_INFO("Assign IP Address");
  Ipv4AddressHelper ipv4;
  Ipv4InterfaceContainer ipv4_address_container;
  oss.str ("");
  oss << "10.0.0.0";
  ipv4.SetBase (oss.str ().c_str (), "255.0.0.0");
  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  for (int z = 0; z < num_host; z++)
		  {
			  host_ipv4_interfaces[i][j][z] = ipv4.Assign(host_2_edgeSwitch_net_device[i][j][z]);
			  NS_LOG_INFO("Host (" << i << "," << j << "," << z << ") ip address for host--edgeSwitch link: " << host_ipv4_interfaces[i][j][z].GetAddress(0));
		  }
	  }
  }

  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  ipv4_address_container = ipv4.Assign(edgeSwitch_2_edgeSwitch_net_device[i][j]);
		  NS_LOG_INFO("Edge Switch (" << i << "," << j << ") ip address for edgeSwitch--edgeSwitch link: " << ipv4_address_container.GetAddress(0));
	  }
  }

  for (int i = 0; i < num_switch; i++)
  {
	  ipv4_address_container = ipv4.Assign(edgeSwitch_2_switch_net_device[i]);
	  NS_LOG_INFO("Edge Switch (" << i << ") ip address for edgeSwitch--switch link: " << ipv4_address_container.GetAddress(0));
  }

  for (int i = 0; i < num_switch; i++)
  {
	  ipv4_address_container = ipv4.Assign(switch_2_switch_net_device[i]);
	  NS_LOG_INFO("Switch (" << i << ") ip address for switch--switch link: " << ipv4_address_container.GetAddress(0));
  }

  for (int i = 0; i < num_switch; i++)
  {
	  oss.str ("");
	  oss << "192.168." << i + 1 << ".0";
	  ipv4.SetBase(oss.str().c_str(),"255.255.255.0");
	  ipv4_address_container = ipv4.Assign(switch_2_controller_net_device[i]);
	  NS_LOG_INFO("Switch (" << i << ") ip address for switch--controller link: " << ipv4_address_container.GetAddress(0));
  }

  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  oss.str ("");
		  oss << "193." << i + 1 << "." << j + 1 << ".0";
		  ipv4.SetBase(oss.str().c_str(),"255.255.255.0");
		  ipv4_address_container = ipv4.Assign(edgeSwitch_2_controller_net_device[i][j]);
		  NS_LOG_INFO("Edge Switch (" << i << "," << j << ") ip address for edgeSwitch--controller link: " << ipv4_address_container.GetAddress(0));
	  }
  }

  //--------------------------------------install application -------------------------------------------
  /* install ON-OFF application and packet sink application in every hosts
	* every host will be installed with an ON-OFF application and packet sink application
	* the destination of the ON-OFF application is local with probability of 0.5 and is remote with 0.5
	*/
  Ptr<UniformRandomVariable> random_num_generator = CreateObject<UniformRandomVariable>();
  //random_num_generator->SetAttribute ("Min", DoubleValue(30));
  //random_num_generator->SetAttribute ("Max", DoubleValue(60));
  ApplicationContainer sink_apps;
  for (int i = 0; i < num_switch; i++)
  {
	  for (int j = 0; j < num_edge_switch; j++)
	  {
		  for (int z = 0; z < num_host; z++)
		  {
			  //NS_LOG_INFO("Install sink application on every host");
			  PacketSinkHelper sink_helper("ns3::TCPSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),9999));
			  sink_apps.Add(sink_helper.Install(host_nodes[i][j].Get(z)));

			 // NS_LOG_INFO("Install the on-off application on every host");
			  if (random_num_generator->GetValue() < prob_remote_app)
				{
				  //the destination of the on-off app is remote
				  int i_remote, j_remote, z_remote;
				  //choose the subnet of destination randomly
				  while (true)
				  {
					i_remote = (int) num_switch * random_num_generator->GetValue();
					if (i_remote != i)
					  break;
				  }
				  //choose the edge switch of destination randomly
				  j_remote = (int) num_edge_switch * random_num_generator->GetValue();
				  //choose the host (i.e., destination) randomly
				  z_remote = (int) num_host * random_num_generator->GetValue();
				  OnOffHelper on_off_helper("ns3::TCPSocketFactory", Address());
				  AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[i_remote][j_remote][z_remote].GetAddress(0),9999));

				  uint8_t buffer[20];
				  uint32_t k_temp;
				  k_temp = remote_address.Get().CopyTo(buffer);
				  std::cout<< "(" << i << "," << j << "," << z <<") -- " << "(" << i_remote << "," << j_remote << "," << z_remote << "): " << \
							  host_ipv4_interfaces[i][j][z].GetAddress(0) << " -- " << \
							  unsigned(buffer[0]) << "." << unsigned(buffer[1]) << "." << unsigned(buffer[2]) << "." << unsigned(buffer[3]) <<std::endl;


				  on_off_helper.SetAttribute("Remote", remote_address);
				  on_off_helper.SetAttribute("DataRate",StringValue("1Kbps"));
				  on_off_helper.SetAttribute("MaxBytes",UintegerValue(maxBytes));
				  ApplicationContainer on_off_app;
				  on_off_app = on_off_helper.Install(host_nodes[i][j].Get(z));
				  on_off_app.Start(Seconds(start_time_interval * random_num_generator->GetValue() + min_start_time));
				}
				else
				{
				  //the destination of the on-off app is local
				  int j_local, z_local;
				  if (num_host == 1)
				  {
					  while (true)
					  {
						  j_local = (int) num_edge_switch * random_num_generator->GetValue();
						  if (j_local != j)
							  break;
					  }
				  }
				  else
				  {
					  j_local = (int) num_edge_switch * random_num_generator->GetValue();
				  }

				  //choose the host (i.e., destination) randomly
				  if (j_local == j)
				  {
					while (true)
					{
					  z_local = (int) num_host * random_num_generator->GetValue();
					  if (z_local != z)
					   break;
					}
				  }
				  else
				  {
					  z_local = (int) num_host * random_num_generator->GetValue();
				  }
				  OnOffHelper on_off_helper("ns3::UdpSocketFactory", Address());
				  AddressValue remote_address(InetSocketAddress(host_ipv4_interfaces[i][j_local][z_local].GetAddress(0),9999));

				  uint8_t buffer[20];
				  uint32_t k_temp;
				  k_temp = remote_address.Get().CopyTo(buffer);
				  std::cout<< "(" << i << "," << j << "," << z <<") -- " << "(" << i << "," << j_local << "," << z_local << "): " << \
						      host_ipv4_interfaces[i][j][z].GetAddress(0) << " -- " << \
							  unsigned(buffer[0]) << "." << unsigned(buffer[1]) << "." << unsigned(buffer[2]) << "." << unsigned(buffer[3]) << std::endl;

				  on_off_helper.SetAttribute("Remote", remote_address);
				  on_off_helper.SetAttribute("DataRate",StringValue("1Kbps"));
				  on_off_helper.SetAttribute("MaxBytes",UintegerValue(maxBytes));
				  ApplicationContainer on_off_app;
				  on_off_app = on_off_helper.Install(host_nodes[i][j].Get(z));
				  on_off_app.Start(Seconds(start_time_interval * random_num_generator->GetValue() + min_start_time));
				}
		  }
	  }
  }
  sink_apps.Start(Seconds(0.0));
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
	// -----------------------------------install the sdn switch application in all edge switch nodes-------------------------
	NS_LOG_INFO("install the sdn switch application on switches");
	for (uint32_t i = 0; i < num_switch; i ++ )
	{
		Ptr<SdnSwitch> sdnS = CreateObject<SdnSwitch> ();
		sdnS->SetStartTime (Seconds (i + 1.0 + 0.1*(double)(i)));
		switch_nodes.Get (i)->AddApplication (sdnS);
	}

	for (int i = 0; i < num_switch; i++)
	{
		for (int j = 0; j < num_edge_switch; j++)
		{
			Ptr<SdnSwitch> sdnS = CreateObject<SdnSwitch> ();
			sdnS->SetStartTime (Seconds (i + (j + 1) * num_switch + 1.0 + 0.1*(double)(i)));
			edge_switch_nodes[i].Get (j)->AddApplication (sdnS);
		}
	}
	std::cout << "Running simulator ... " << std::endl;
	//link_host_2_edgeSwitch.EnablePcapAll("layer2P2P");
	//link_controller_2_switch.EnablePcapAll("controller2switch");
	// Calculate routing tables
	//  std::cout << "Populating Routing tables..." << std::endl;
	// Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

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
	std::cout << "total sended bytes = " << maxBytes * num_switch * num_edge_switch * num_host << std::endl;
	std::cout << "total simulation time = " << simulate_time << std::endl;
	Simulator::Destroy();

	std::cout << "Simulation is finished" << std::endl;
	return 0;
 }

