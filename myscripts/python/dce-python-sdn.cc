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
 *  l0---     --- c0---     ---r0
 *       \   /         \   /
 *   .    \ /           \ /     .
 *   .     s0----...----sn      .
 *   .    /               \     .
 *       /                 \
 *  ln---                   ---rn
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

#define REALLY_BIG_TIME 1000000

typedef struct timeval TIMER_TYPE;
#define TIMER_NOW(_t) gettimeofday (&_t,NULL);
#define TIMER_SECONDS(_t) ((double)(_t).tv_sec + (_t).tv_usec * 1e-6)
#define TIMER_DIFF(_t1, _t2) (TIMER_SECONDS (_t1) - TIMER_SECONDS (_t2))

NS_LOG_COMPONENT_DEFINE ("DcePythonSdn");

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
  bool verbose = true;
  uint32_t maxBytes = 5000;
  uint32_t controller = RYU;
  uint32_t appChoice = ON_OFF;

  uint32_t numHosts    = 2;
  uint32_t numSwitches = 2;
  uint32_t numControllers = 1;

  std::ostringstream oss;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("controller",
                "Controller to use: (0) Ryu; (1) POX", controller);
  cmd.AddValue ("appChoice",
                "Application to use: (0) Bulk Send; (1) Ping; (2) On Off", appChoice);
  cmd.AddValue ("numSwitches", "Number of switches", numSwitches);
  cmd.AddValue ("numHosts", "Number of hosts per end switch", numHosts);
  cmd.AddValue ("numControllers", "Number of controllers; switches will be assigned equally across controllers", numControllers);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("DcePythonSdn", LOG_LEVEL_INFO);
      LogComponentEnable ("SdnSwitch", LOG_LEVEL_INFO);
    }

  NS_ASSERT_MSG (numSwitches % numControllers == 0, "Number of switches must be a multiple of number of controllers.");
  NS_ASSERT_MSG (numSwitches % 2 == 0, "Number of switches must be even.");

  TIMER_TYPE t0, t1, t2;
  TIMER_NOW (t0);

  NodeContainer nodes, controllerNodes, switchNodes, leftNodes, rightNodes;
  controllerNodes.Create (numControllers);
  switchNodes.Create (numSwitches);
  leftNodes.Create (numHosts*numSwitches/2);
  rightNodes.Create (numHosts*numSwitches/2);

  nodes.Add (controllerNodes);
  nodes.Add (switchNodes);
  nodes.Add (leftNodes);
  nodes.Add (rightNodes);

  NS_LOG_INFO ("Create channels.");
  Layer2P2PHelper layer2P2P;
  layer2P2P.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  layer2P2P.SetChannelAttribute ("Delay", StringValue ("1ms"));
  
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  std::vector<NetDeviceContainer> leftHostToSwitchContainers;
  std::vector<NetDeviceContainer> rightHostToSwitchContainers;
  std::vector<NetDeviceContainer> switchToSwitchContainers;
  std::vector<NetDeviceContainer> switchToControllerContainers;
  
  // Host to Switch connections
  NS_LOG_INFO ("Create host-to-switch channels.");
  for (uint32_t i = 0; i < numSwitches/2; ++i)
    {
      for (uint32_t j = 0; j < numHosts; ++j)
        {
          leftHostToSwitchContainers.push_back (layer2P2P.Install (leftNodes.Get (j+i*numHosts), switchNodes.Get (i)));
          rightHostToSwitchContainers.push_back (layer2P2P.Install (switchNodes.Get (numSwitches/2 + i), rightNodes.Get (j+i*numHosts)));
        }
    }

  // Switch to Switch connections
  NS_LOG_INFO ("Create switch-to-switch channels.");
  for (uint32_t j = 0; j < numSwitches; ++j)
     {
       if (j > 0)
         {
           switchToSwitchContainers.push_back (layer2P2P.Install (switchNodes.Get (j-1), switchNodes.Get (j)));
         }
     }
  
  // Switch to Controller connections
  NS_LOG_INFO ("Create switch-to-controller channels.");
  uint32_t numConRegions = numSwitches / numControllers;
  for (uint32_t j = 0; j < switchNodes.GetN(); ++j)
     {
       switchToControllerContainers.push_back (pointToPoint.Install (switchNodes.Get (j), controllerNodes.Get (j / numConRegions)));
     }

  DceManagerHelper dceManager;
  dceManager.SetVirtualPath ("/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/ryu");
  dceManager.Install (controllerNodes);


  InternetStackHelper stack;
  stack.Install (nodes);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  std::vector<Ipv4InterfaceContainer> leftHostToSwitchIPContainers;

  oss.str ("");
  oss << "10.0.0.0";
  ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");

  for (uint32_t i=0; i < leftHostToSwitchContainers.size(); ++i)
    {
      leftHostToSwitchIPContainers.push_back(ipv4.Assign (leftHostToSwitchContainers[i]));

      NS_LOG_INFO ("Host " << i << " address: " << leftHostToSwitchIPContainers[i].GetAddress(0));
    }

  std::vector<Ipv4InterfaceContainer> rightHostToSwitchIPContainers;
  for (uint32_t i=0; i < rightHostToSwitchContainers.size(); ++i)
    {
      rightHostToSwitchIPContainers.push_back(ipv4.Assign (rightHostToSwitchContainers[i]));

      NS_LOG_INFO ("Host " << i << " address: " << rightHostToSwitchIPContainers[i].GetAddress(1));
    }

  std::vector<Ipv4InterfaceContainer> switchToSwitchIPContainers;
  for (uint32_t i=0; i < switchToSwitchContainers.size(); ++i)
    {
      switchToSwitchIPContainers.push_back(ipv4.Assign (switchToSwitchContainers[i]));
    }

  std::vector<Ipv4InterfaceContainer> switchToControllerIPContainers;
  for (uint32_t i=0; i < switchToControllerContainers.size(); ++i)
    {
      oss.str ("");
      oss << "192.168." << i+1 << ".0";
      ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");
      switchToControllerIPContainers.push_back(ipv4.Assign (switchToControllerContainers[i]));
    }

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
                  sourceAppL2R.Start (Seconds (randTime->GetValue()));
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

          OnOffHelper source ("ns3::TcpSocketFactory",
                              InetSocketAddress (rightHostToSwitchIPContainers[i].GetAddress(1), port));
          source.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
          source.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

          sourceApp = source.Install (leftNodes.Get (i));
          sourceApp.Start (Seconds (randTime->GetValue()));
          sourceApps.Add(sourceApp);
        }
    }

//
// Create a PacketSinkApplication and install it on right nodes
//
  ApplicationContainer sinkApps;
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  for (uint32_t i = 0; i < rightNodes.GetN (); ++i)
    {
      sinkApps.Add(sink.Install (rightNodes.Get (i)));
    }
  sinkApps.Start (Seconds (0.0));

//
// Set up controller node application
//
  DceApplicationHelper dce;
  ApplicationContainer apps;

  for (uint32_t i = 0; i < controllerNodes.GetN (); ++i)
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
          dce.AddArgument ("ryu/app/simple_switch.py");
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

      apps = dce.Install (controllerNodes.Get (i));
      apps.Start (Seconds (0.0));
    }

//
// Install Switch.
//
  for (uint32_t j = 0; j < switchNodes.GetN(); ++j)
    {
      Ptr<SdnSwitch> sdnS = CreateObject<SdnSwitch> ();
      sdnS->SetStartTime (Seconds (j + 1.0 + 0.1*(double)(j)));
      switchNodes.Get (j)->AddApplication (sdnS);
    }

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

  std::cout << (controller ? "POX\t" : "Ryu\t") << numControllers << "\t" << numSwitches << "\t";
  std::cout << numHosts << "\t" << simTime << "\t" << d1;
  std::cout << "\t" << totalRx;
  std::cout << "\t" << (V4Ping::m_totalSend - V4Ping::m_totalRecv);
  std::cout << "\t" << V4Ping::m_totalSend << "\t" << V4Ping::m_totalRecv << std::endl;

  Simulator::Destroy ();
  return 0;
}
