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


#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/layer2-p2p-module.h"
#include "ns3/dce-module.h"
#include "ns3/SdnSwitch.h"
#include "ns3/log.h"
#include "ns3/netanim-module.h"

#include <vector>
#include <stdint.h>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DcePythonRap");

int main (int argc, char *argv[])
{
  uint32_t nWifis = 2;
  uint32_t nStas = 3;
  bool writeMobility = false;
  bool defend = false;
  bool pinging = false;
  bool useKernel = 0;

  CommandLine cmd;
  cmd.AddValue ("nWifis", "Number of wifi networks", nWifis);
  cmd.AddValue ("nStas", "Number of stations per wifi network", nStas);
  cmd.AddValue ("writeMobility", "Write mobility trace", writeMobility);
  cmd.AddValue ("defend", "Use RAP control instead of learning switch", defend);
  cmd.AddValue ("pinging", "Use ping instead of OnOff traffic", pinging);
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.Parse (argc, argv);

  NodeContainer backboneNodes;
  NetDeviceContainer backboneDevices;
  Ipv4InterfaceContainer backboneInterfaces;
  std::vector<NodeContainer> staNodes;
  std::vector<NetDeviceContainer> staDevices;
  std::vector<NetDeviceContainer> apDevices;
  std::vector<Ipv4InterfaceContainer> staInterfaces;
  std::vector<Ipv4InterfaceContainer> apInterfaces;

  //LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("DcePythonRap", LOG_LEVEL_INFO);
  //LogComponentEnable ("SdnSwitch", LOG_LEVEL_FUNCTION);

  Ipv4AddressHelper ip;
  ip.SetBase ("10.1.1.0", "255.255.255.0");

  NS_LOG_INFO("Set up SDN network device connections.");
  NodeContainer controllerNodes;
  NodeContainer switchNodes;
  NodeContainer hostNodes;

  controllerNodes.Create(1);
  switchNodes.Create(1);
  hostNodes.Create(2);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (controllerNodes);
  mobility.Install (switchNodes);

  backboneNodes.Create (nWifis);

  NS_LOG_INFO ("Create channels.");
  Layer2P2PHelper layer2P2P;
  layer2P2P.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  layer2P2P.SetChannelAttribute ("Delay", StringValue ("1ms"));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NS_LOG_INFO ("Create switch-to-host channels.");
  std::vector<NetDeviceContainer> switchToHostContainers;
  for (uint32_t i = 0; i < nWifis; ++i)
    {
      switchToHostContainers.push_back (layer2P2P.Install (hostNodes.Get (i), switchNodes.Get (0)));
    }

  NS_LOG_INFO ("Create switch-to-backbone channels.");
  std::vector<NetDeviceContainer> switchToBackboneContainers;
  for (uint32_t i = 0; i < nWifis; ++i)
    {
      switchToBackboneContainers.push_back (layer2P2P.Install (backboneNodes.Get (i), switchNodes.Get (0)));
    }

  NS_LOG_INFO ("Create switch-to-controller channels.");
  std::vector<NetDeviceContainer> switchToControllerContainers;
  switchToControllerContainers.push_back (pointToPoint.Install (switchNodes.Get (0), controllerNodes.Get (0)));

  DceManagerHelper dceManager;
  InternetStackHelper stack;
  LinuxStackHelper linuxStack;
  if (!useKernel)
    {
      dceManager.Install (controllerNodes);
      dceManager.Install (backboneNodes);
      stack.Install (controllerNodes);
      stack.Install (backboneNodes);
      stack.Install (switchNodes);
      stack.Install (hostNodes);
    }
  else
    {
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      dceManager.Install (controllerNodes);
      dceManager.Install (backboneNodes);
      dceManager.Install (switchNodes);
      dceManager.Install (hostNodes);
      linuxStack.Install (controllerNodes);
      linuxStack.Install (backboneNodes);
      linuxStack.Install (switchNodes);
      linuxStack.Install (hostNodes);
    }

  double wifiX = 0.0;

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);

  for (uint32_t i = 0; i < nWifis; ++i)
    {
      // calculate ssid for wifi subnetwork
      std::ostringstream oss;
      oss << "wifi-default-" << i;
      Ssid ssid = Ssid (oss.str ());

      NodeContainer sta;
      NetDeviceContainer staDev;
      NetDeviceContainer apDev;
      Ipv4InterfaceContainer staInterface;
      Ipv4InterfaceContainer apInterface;
      MobilityHelper wifiMobility;
      BridgeHelper bridge;
      WifiHelper wifi = WifiHelper::Default ();
      NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
      YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
      wifiPhy.SetChannel (wifiChannel.Create ());

      sta.Create (nStas);
      wifiMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (wifiX),
                                     "MinY", DoubleValue (0.0),
                                     "DeltaX", DoubleValue (5.0),
                                     "DeltaY", DoubleValue (5.0),
                                     "GridWidth", UintegerValue (1),
                                     "LayoutType", StringValue ("RowFirst"));


      // setup the AP.
      wifiMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      wifiMobility.Install (backboneNodes.Get (i));
      wifiMac.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid));
      apDev = wifi.Install (wifiPhy, wifiMac, backboneNodes.Get (i));

      NetDeviceContainer bridgeDev;
      bridgeDev = bridge.Install (backboneNodes.Get (i),
    		  NetDeviceContainer (apDev, switchToBackboneContainers[i].Get (0)));

      // assign AP IP address to bridge, not wifi
      apInterface = ip.Assign (bridgeDev);

      // setup the STAs
      if (useKernel)
        {
          linuxStack.Install (sta);
          dceManager.Install (sta);
        }
      else
        {
          stack.Install (sta);
        }
      wifiMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", StringValue ("Time"),
                                 "Time", StringValue ("2s"),
                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                 "Bounds", RectangleValue (Rectangle (wifiX, wifiX+5.0,0.0, (nStas+1)*5.0)));
      wifiMobility.Install (sta);
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid),
                       "ActiveProbing", BooleanValue (false));
      staDev = wifi.Install (wifiPhy, wifiMac, sta);
      staInterface = ip.Assign (staDev);

      // save everything in containers.
      staNodes.push_back (sta);
      apDevices.push_back (apDev);
      apInterfaces.push_back (apInterface);
      staDevices.push_back (staDev);
      staInterfaces.push_back (staInterface);

      wifiX += 20.0;
    }

  NS_LOG_INFO("Set up SDN network IP connections.");

  std::vector<Ipv4InterfaceContainer> switchToBackboneIPContainers; // Consider using static routes for wifi?
  for (uint32_t i=0; i < switchToBackboneContainers.size(); ++i)
    {
      switchToBackboneIPContainers.push_back(ip.Assign (switchToBackboneContainers[i]));
    }

  std::vector<Ipv4InterfaceContainer> switchToHostIPContainers;
  for (uint32_t i=0; i < switchToHostContainers.size(); ++i)
    {
      switchToHostIPContainers.push_back(ip.Assign (switchToHostContainers[i]));
    }

  std::vector<Ipv4InterfaceContainer> switchToControllerIPContainers;
  ip.SetBase ("192.168.1.0", "255.255.255.0");
  switchToControllerIPContainers.push_back(ip.Assign (switchToControllerContainers[0]));

  //
  // Set up controller node application
  //
  DceApplicationHelper dce;
  ApplicationContainer sdnApps;

  NS_LOG_INFO("Set up SDN controllers.");
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
	  dce.AddArgument ("-S");

	  dce.AddArgument ("ryu-manager");
	  dce.AddArgument ("--verbose");
	  if (defend)
	  {
		  dce.AddArgument ("ryu/app/coupler.py");
	  }
	  else
	  {
		  dce.AddArgument ("ryu/app/simple_switch.py");
	  }

	  sdnApps.Add (dce.Install (controllerNodes.Get (i)));
    }
  sdnApps.Start (Seconds (0.01));

//  ApplicationContainer allNatApps;
//  for (uint32_t i = 0; i < backboneNodes.GetN (); ++i)
//    {
//	  dce.SetStackSize (1<<20);
//
//	  dce.SetBinary ("iptables");
//	  dce.ResetArguments ();
//	  dce.ResetEnvironment ();
//	  dce.AddArgument ("-F");
//
//	  ApplicationContainer natApps0 = dce.Install (backboneNodes.Get (i));
//	  natApps0.Start(Seconds(0.01));
//	  allNatApps.Add(natApps0);
//
//	  dce.SetBinary ("iptables");
//	  dce.ResetArguments ();
//	  dce.ResetEnvironment ();
//	  dce.AddArgument ("-t");
//	  dce.AddArgument ("nat");
//	  dce.AddArgument ("-F");
//
//	  ApplicationContainer natApps1 = dce.Install (backboneNodes.Get (i));
//	  natApps1.Start(Seconds(0.02));
//	  allNatApps.Add(natApps1);
//
//	  dce.SetBinary ("iptables");
//	  dce.ResetArguments ();
//	  dce.ResetEnvironment ();
//	  dce.AddArgument ("-P");
//	  dce.AddArgument ("INPUT");
//	  dce.AddArgument ("ACCEPT");
//
//	  ApplicationContainer natApps2 = dce.Install (backboneNodes.Get (i));
//	  natApps2.Start(Seconds(0.03));
//	  allNatApps.Add(natApps2);
//
//	  dce.SetBinary ("iptables");
//	  dce.ResetArguments ();
//	  dce.ResetEnvironment ();
//	  dce.AddArgument ("-P");
//	  dce.AddArgument ("OUTPUT");
//	  dce.AddArgument ("ACCEPT");
//
//	  ApplicationContainer natApps3 = dce.Install (backboneNodes.Get (i));
//	  natApps3.Start(Seconds(0.04));
//	  allNatApps.Add(natApps3);
//
//	  dce.SetBinary ("iptables");
//	  dce.ResetArguments ();
//	  dce.ResetEnvironment ();
//	  dce.AddArgument ("-P");
//	  dce.AddArgument ("FORWARD");
//	  dce.AddArgument ("DROP");
//
//	  ApplicationContainer natApps4 = dce.Install (backboneNodes.Get (i));
//	  natApps4.Start(Seconds(0.05));
//	  allNatApps.Add(natApps4);
//    }

  //
  // Install Switch.
  //
  NS_LOG_INFO("Set up SDN switches.");
  for (uint32_t j = 0; j < switchNodes.GetN(); ++j)
    {
      Ptr<SdnSwitch> sdnS = CreateObject<SdnSwitch> ();
      if (useKernel)
        {
    	  sdnS->SetAttribute ("Kernel", BooleanValue (true));
        }
      sdnS->SetStartTime (Seconds (0.5));

      switchNodes.Get (j)->AddApplication (sdnS);
    }

  std::string protocol = "ns3::TcpSocketFactory";
  if (useKernel)
  {
//	  LinuxStackHelper::PopulateRoutingTables();
	  protocol = "ns3::LinuxTcpSocketFactory";
  }

  Address sta0Dest;
  ApplicationContainer host1Apps;
  NS_LOG_INFO("Create Applications.");
  if (pinging)
    {
	  V4PingHelper ping2Sta0 = V4PingHelper (staInterfaces[0].GetAddress (1));
	  if (useKernel)
	    {
		  ping2Sta0.SetAttribute ("Kernel", BooleanValue (true));
	    }
	  ping2Sta0.SetAttribute ("Verbose", BooleanValue (true));
	  host1Apps.Add(ping2Sta0.Install (hostNodes.Get (1)));
    }
  else
    {
	  sta0Dest = InetSocketAddress (staInterfaces[0].GetAddress (1), 1025);
	  OnOffHelper onoff2Sta0 = OnOffHelper (protocol, sta0Dest);
	  onoff2Sta0.SetConstantRate (DataRate ("500kb/s"));
	  host1Apps.Add(onoff2Sta0.Install (hostNodes.Get (1)));
    }
  host1Apps.Start (Seconds (2.0));
  host1Apps.Stop (Seconds (5.0));

  Address sta1Dest;
  ApplicationContainer sta0Apps;
  if (pinging)
    {
	  V4PingHelper ping2Sta1 = V4PingHelper (staInterfaces[1].GetAddress (1));
	  if (useKernel)
	    {
		  ping2Sta1.SetAttribute ("Kernel", BooleanValue (true));
	    }
	  ping2Sta1.SetAttribute ("Verbose", BooleanValue (true));
	  sta0Apps.Add(ping2Sta1.Install (staNodes[0].Get (0)));
    }
  else
    {
      sta1Dest = InetSocketAddress (staInterfaces[1].GetAddress (1), 1025);
      OnOffHelper onoff2Sta1 = OnOffHelper (protocol, sta1Dest);
      onoff2Sta1.SetConstantRate (DataRate ("500kb/s"));
      sta0Apps.Add(onoff2Sta1.Install (staNodes[0].Get (0)));
    }
  sta0Apps.Start (Seconds (5.0));
  sta0Apps.Stop (Seconds (8.0));

  Address host0Dest;
  ApplicationContainer sta1Apps;
  if (pinging)
    {
	  V4PingHelper ping2Host0 = V4PingHelper (switchToHostIPContainers[0].GetAddress (0));
	  if (useKernel)
	    {
		  ping2Host0.SetAttribute ("Kernel", BooleanValue (true));
	    }
	  ping2Host0.SetAttribute ("Verbose", BooleanValue (true));
	  sta1Apps.Add(ping2Host0.Install (staNodes[1].Get (0)));
    }
  else
    {
	  host0Dest = InetSocketAddress (switchToHostIPContainers[0].GetAddress (0), 1025);
	  OnOffHelper onoff2Host0 = OnOffHelper (protocol, host0Dest);
	  onoff2Host0.SetConstantRate (DataRate ("500kb/s"));
	  sta1Apps.Add(onoff2Host0.Install (staNodes[1].Get (0)));
    }
  sta1Apps.Start (Seconds (8.0));
  sta1Apps.Stop (Seconds (11.0));

  Address host1Dest;
  ApplicationContainer host0Apps;
  if (pinging)
    {
	  V4PingHelper ping2Host1 = V4PingHelper (switchToHostIPContainers[1].GetAddress (0));
	  if (useKernel)
	    {
		  ping2Host1.SetAttribute ("Kernel", BooleanValue (true));
	    }
	  ping2Host1.SetAttribute ("Verbose", BooleanValue (true));
	  host0Apps.Add(ping2Host1.Install (hostNodes.Get (0)));
    }
  else
    {
	  host1Dest = InetSocketAddress (switchToHostIPContainers[1].GetAddress (0), 1025);
	  OnOffHelper onoff2Host1 = OnOffHelper (protocol, host1Dest);
	  onoff2Host1.SetConstantRate (DataRate ("500kb/s"));
	  host0Apps.Add(onoff2Host1.Install (hostNodes.Get (0)));
    }
  host0Apps.Start (Seconds (11.0));
  host0Apps.Stop (Seconds (14.0));

  PacketSinkHelper sink = PacketSinkHelper (protocol,
		  Address (InetSocketAddress (Ipv4Address::GetAny (), 1025)));
  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < staNodes[0].GetN(); ++i)
  {
	  sinkApps.Add(sink.Install (staNodes[0].Get(i)));
  }
  for (uint32_t i = 0; i < staNodes[1].GetN(); ++i)
  {
	  sinkApps.Add(sink.Install (staNodes[1].Get(i)));
  }
  for (uint32_t i = 0; i < hostNodes.GetN(); ++i)
  {
	  sinkApps.Add(sink.Install (hostNodes.Get(i)));
  }
  sinkApps.Start(Seconds(0.1));

  wifiPhy.EnablePcap ("wifi-wired-bridging", apDevices[0]);
  wifiPhy.EnablePcap ("wifi-wired-bridging", apDevices[1]);

  AnimationInterface anim ("dce-python-rap.xml");

  anim.SetConstantPosition (controllerNodes.Get(0), 0, 0);
  anim.SetConstantPosition (switchNodes.Get(0), 5, 0);
  anim.SetConstantPosition (hostNodes.Get(0), 10, 3);
  anim.SetConstantPosition (hostNodes.Get(1), 10, 6);

  if (writeMobility)
    {
      AsciiTraceHelper ascii;
      MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("wifi-wired-bridging.mob"));
    }

  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop (Seconds (15.0));
  Simulator::Run ();

  uint32_t totalRx = 0;
  for (uint32_t i = 0; i < sinkApps.GetN(); ++i)
  {
	  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (i));
	  if (sink1)
	    {
	      totalRx += sink1->GetTotalRx();
	    }
  }
  std::cout << Simulator::Now().GetSeconds() << "\t" << totalRx << std::endl;

  Simulator::Destroy ();
}
