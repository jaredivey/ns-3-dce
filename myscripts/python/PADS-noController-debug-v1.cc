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
 *
 *  l0---                 ---r0
 *       \               /
 *   .    \             /     .
 *   .     s0---...---sn      .
 *   .    /             \     .
 *       /               \
 *  ln---                 ---rn
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

NS_LOG_COMPONENT_DEFINE ("simple-dumbbell");

enum APPCHOICE
{
  BULK_SEND,
  PING,
  ON_OFF,
} APPCHOICE;

int
main (int argc, char *argv[])
{
  bool tracing = false;
  uint32_t maxBytes = 0;
  bool verbose = true;
  uint32_t appChoice = ON_OFF;

  uint32_t numHosts    = 2;
  uint32_t numSwitches = 3;
  uint32_t num_controller = 1;
  uint32_t controller = RYU;

  std::ostringstream oss;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("appChoice",
                "Application to use: (0) Bulk Send; (1) Ping; (2) On Off", appChoice);
  cmd.AddValue ("numSwitches", "Number of switches", numSwitches);
  cmd.AddValue ("numHosts", "Number of hosts per end switch", numHosts);


  cmd.Parse (argc,argv);


  LogComponentEnable ("simple-dumbbell", LOG_LEVEL_INFO);

  LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("V4Ping", LOG_LEVEL_INFO);
  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  //LogComponentEnable ("SdnSwitch", LOG_LEVEL_INFO);


  TIMER_TYPE t0, t1, t2;
  TIMER_NOW (t0);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer switchNodes, leftNodes, rightNodes;
  // --transform to dce--
    NodeContainer controller_nodes;
    controller_nodes.Create(num_controller);
  switchNodes.Create (numSwitches);
  leftNodes.Create (numHosts);
  rightNodes.Create (numHosts);



  NS_LOG_INFO ("Create channels.");

  Layer2P2PHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("EncapsulationMode",EnumValue(1));
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  std::vector<NetDeviceContainer> leftHostToSwitchContainers;
  std::vector<NetDeviceContainer> rightHostToSwitchContainers;
  std::vector<NetDeviceContainer> switchToSwitchContainers;
  
  // --transform to dce--
  PointToPointHelper link_controller_2_switch;
  link_controller_2_switch.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link_controller_2_switch.SetChannelAttribute ("Delay", StringValue ("1ms"));
  std::vector<NetDeviceContainer> controller_net_devices;


  for (uint32_t j = 0; j < numHosts; ++j)
     {
       leftHostToSwitchContainers.push_back (pointToPoint.Install (
                               NodeContainer(leftNodes.Get (j),
                                             switchNodes.Get (0))));
     }

  for (uint32_t j = 0; j < numHosts; ++j)
     {
       rightHostToSwitchContainers.push_back (pointToPoint.Install (
                                NodeContainer(switchNodes.Get (numSwitches-1),
                                              rightNodes.Get (j))));
     }

  for (uint32_t j = 0; j < numSwitches; ++j)
     {
       if (j > 0)
         {
           switchToSwitchContainers.push_back (pointToPoint.Install (
                                 NodeContainer(switchNodes.Get (j-1),
                                               switchNodes.Get (j))));
         }
       else
       {
    	   switchToSwitchContainers.push_back (pointToPoint.Install (
    	                                    NodeContainer(switchNodes.Get (numSwitches-1),switchNodes.Get (0))));
       }
     }

  for (uint32_t i = 0; i < numSwitches; i++)
       {
    	   controller_net_devices.push_back(link_controller_2_switch.Install(switchNodes.Get(i),controller_nodes.Get(0)));
       }
//
// Install the internet stack on the nodes
//

  //---transform to dce---
  // ---------------------------------------create the controller-------------------------
   NS_LOG_INFO("Create the controller");
   DceManagerHelper dceManager;
   dceManager.SetVirtualPath ("/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/ryu");
   dceManager.Install (controller_nodes);

   InternetStackHelper internet;
	internet.Install (leftNodes);
	internet.Install (switchNodes);
	internet.Install (rightNodes);
   internet.Install(controller_nodes);

   // We've got the "hardware" in place.  Now we need to add IP addresses.
   //
     NS_LOG_INFO ("Assign IP Addresses.");
     Ipv4AddressHelper ipv4;
     std::vector<Ipv4InterfaceContainer> leftHostToSwitchIPContainers;


   oss.str ("");
   oss << "10.0.0.0";
   ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");
  for (uint32_t i=0; i < leftHostToSwitchContainers.size(); ++i)
    {
	/*  oss.str ("");
	  oss << "10.1." << i << ".0";
	  ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");*/
      leftHostToSwitchIPContainers.push_back(ipv4.Assign (leftHostToSwitchContainers[i]));

      NS_LOG_INFO ("Host " << i << " address: " << leftHostToSwitchIPContainers[i].GetAddress(0));
    }

  std::vector<Ipv4InterfaceContainer> switchToSwitchIPContainers;
  for (uint32_t i=0; i < switchToSwitchContainers.size(); ++i)
    {
     /* oss.str ("");
      oss << "10.1." << numHosts + i << ".0";
      ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");*/

      switchToSwitchIPContainers.push_back(ipv4.Assign (switchToSwitchContainers[i]));
    }

  std::vector<Ipv4InterfaceContainer> rightHostToSwitchIPContainers;
  for (uint32_t i=0; i < rightHostToSwitchContainers.size(); ++i)
    {
	 /* oss.str ("");
	  oss << "10.1." << numHosts + i +numSwitches << ".0";
	  ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");*/

      rightHostToSwitchIPContainers.push_back(ipv4.Assign (rightHostToSwitchContainers[i]));

      NS_LOG_INFO ("Host " << i << " address: " << rightHostToSwitchIPContainers[i].GetAddress(1));
    }




  	 NS_LOG_INFO("connect the controller to the edge switches");
  	std::vector<Ipv4InterfaceContainer> switchToControllerIPContainers;
    // connect each edge switch node to the controller
    for (uint32_t i = 0; i < controller_net_devices.size(); i++)
    {
 	  //set the address base for the hosts connecting to the (i,j) th edge switch
  	  oss.str("");
  	  oss << "192.168." << i + 1 << ".0";
  	  ipv4.SetBase(oss.str().c_str(),"255.255.255.0");
  	  switchToControllerIPContainers.push_back(ipv4.Assign (controller_net_devices[i]));
    }
    //-----------------------------------------------------------------------------------------

// Set up default next hops for host nodes.
//
//  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  NS_LOG_INFO ("Create Applications.");
  ApplicationContainer sourceApps;
  uint16_t port = 0;

  Ptr<UniformRandomVariable> randTime = CreateObject<UniformRandomVariable>();
  randTime->SetAttribute ("Min", DoubleValue(30));
  randTime->SetAttribute ("Max", DoubleValue(60));

  for (uint32_t i = 0; i < leftNodes.GetN(); ++i)
    {
      if (appChoice == BULK_SEND)
        {
          ApplicationContainer sourceApp;
          port = 9;  // well-known echo port number

          NS_LOG_INFO ("Bulk Send app to address: " << rightHostToSwitchIPContainers[i].GetAddress(1));
          BulkSendHelper source ("ns3::TcpSocketFactory",
                                 InetSocketAddress (rightHostToSwitchIPContainers[i].GetAddress(1), port));

          // Set the amount of data to send in bytes.  Zero is unlimited.
          source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

          sourceApp = source.Install (leftNodes.Get (i));
          sourceApp.Start (Seconds (randTime->GetValue()));
          sourceApps.Add(sourceApp);
        }
      else if (appChoice == PING)
        {
          for (uint32_t j = 0; j < numHosts; ++j)
            {
              NS_LOG_INFO ("PING app to address: " << rightHostToSwitchIPContainers[j].GetAddress(1));
              V4PingHelper source1 (rightHostToSwitchIPContainers[j].GetAddress(1));
              source1.SetAttribute ("Verbose", BooleanValue (true));
              source1.SetAttribute ("PingAll", BooleanValue (true));
              source1.SetAttribute ("Count", UintegerValue (1));

              NS_LOG_INFO ("PING app to address: " << leftHostToSwitchIPContainers[j].GetAddress(0));
              V4PingHelper source2 (leftHostToSwitchIPContainers[j].GetAddress(0));
              source2.SetAttribute ("Verbose", BooleanValue (true));
              source2.SetAttribute ("PingAll", BooleanValue (true));
              source2.SetAttribute ("Count", UintegerValue (1));

              ApplicationContainer sourceAppL2R;
              sourceAppL2R = source1.Install (leftNodes.Get (i));
              if ((i == 0) && (j == 0))
                {
                  sourceAppL2R.Start (Seconds (numSwitches*2));
                  sourceApps.Add(sourceAppL2R);
                }
              else
                {
                  sourceAppL2R.Start (Seconds (REALLY_BIG_TIME));
                  sourceApps.Add(sourceAppL2R);
                }

              ApplicationContainer sourceAppL2L;
              if (i != j)
                {
                  sourceAppL2L = source2.Install (leftNodes.Get (i));
                  sourceAppL2L.Start (Seconds (REALLY_BIG_TIME));
                  sourceApps.Add(sourceAppL2L);
                }

              ApplicationContainer sourceAppR2L;
              sourceAppR2L = source2.Install (rightNodes.Get (i));
              sourceAppR2L.Start (Seconds (REALLY_BIG_TIME));
              sourceApps.Add(sourceAppR2L);

              ApplicationContainer sourceAppR2R;
              if (i != j)
                {
                  sourceAppR2R = source1.Install (rightNodes.Get (i));
                  sourceAppR2R.Start (Seconds (REALLY_BIG_TIME));
                  sourceApps.Add(sourceAppR2R);
                }
            }
        }
      else if (appChoice == ON_OFF)
        {
          ApplicationContainer sourceApp;
          port = 50000;

          OnOffHelper source ("ns3::UdpSocketFactory",
                              InetSocketAddress (rightHostToSwitchIPContainers[i].GetAddress(1), port));
          source.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
          source.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          source.SetAttribute ("MaxBytes", UintegerValue (512));

          sourceApp = source.Install (leftNodes.Get (i));
          sourceApp.Start (Seconds (randTime->GetValue()));
          sourceApps.Add(sourceApp);
        }
    }
//
// Create a PacketSinkApplication and install it on right nodes
//
  ApplicationContainer sinkApps;
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
 // L2PacketSinkHelper sinkL2 ("ns3::TcpSocketFactory");
  for (uint32_t i = 0; i < rightNodes.GetN(); ++i)
    {
      sinkApps.Add(sink.Install (rightNodes.Get (i)));
   //   sinkApps.Add(sinkL2.Install (rightNodes.Get (i)));
    }
  sinkApps.Start (Seconds (0.0));


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
        //  dce.AddArgument ("ryu/lib/stplib.py");
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
         // dce.AddArgument ("--no-flood");
         // dce.AddArgument ("--hold-down");
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
  for (uint32_t i = 0; i < switchNodes.GetN(); i ++ )
 {
	Ptr<SdnSwitch> sdnS = CreateObject<SdnSwitch> ();
	sdnS->SetStartTime (Seconds (i + 1.0 + 0.1*(double)(i)));
	switchNodes.Get (i)->AddApplication (sdnS);
 }



//

 // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
   TIMER_NOW (t1);

   Simulator::Stop (Seconds(300));
   Simulator::Run ();
   TIMER_NOW (t2);

   uint32_t totalRx = 0;
   for (uint32_t i = 0; i < sinkApps.GetN(); ++i)
     {
       Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (i));
       if (sink1)
         {
           totalRx += sink1->GetTotalRx();
         }
     }

   double simTime = Simulator::Now().GetSeconds();
   double d1 = TIMER_DIFF (t1, t0) + TIMER_DIFF (t2, t1);

   std::cout << (controller ? "POX\t" : "Ryu\t") << num_controller << "\t" << numSwitches << "\t";
   std::cout << numHosts << "\t" << simTime << "\t" << d1;
   std::cout << "\t" << totalRx;
   std::cout << "\t" << (V4Ping::m_totalSend - V4Ping::m_totalRecv);
   std::cout << "\t" << V4Ping::m_totalSend << "\t" << V4Ping::m_totalRecv << std::endl;

   Simulator::Destroy ();
   return 0;
}
