#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

class MyApp : public Application 
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0), 
    m_peer (), 
    m_packetSize (0), 
    m_nPackets (0), 
    m_dataRate (0), 
    m_sendEvent (), 
    m_running (false), 
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void 
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void 
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void 
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  DceManagerHelper dceManager;
  dceManager.SetVirtualPath ("/home/ubuntu/dce/source/ns-3-dce/myscripts/python:/home/ubuntu/dce/source/ns-3-dce:/home/ubuntu/dce/build/lib/python27.zip:/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/plat-linux2:/home/ubuntu/dce/build/lib/python2.7/lib-tk:/home/ubuntu/dce/build/lib/python2.7/lib-old:/home/ubuntu/dce/build/lib/lib-dynload:/home/ubuntu/dce/build/lib/python2.7/site-packages");
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1<<20);

  dce.SetBinary ("python2.7.10");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-v");
  dce.AddArgument ("-S");
  dce.AddArgument ("server.py");
  dce.AddArgument ("8888");

  dce.AddEnvironment ("PATH", "/home/ubuntu/dce/source/ns-3-dce/myscripts/python:/home/ubuntu/dce/source/ns-3-dce:/home/ubuntu/dce/build/lib/python27.zip:/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/plat-linux2:/home/ubuntu/dce/build/lib/python2.7/lib-tk:/home/ubuntu/dce/build/lib/python2.7/lib-old:/home/ubuntu/dce/build/lib/lib-dynload:/home/ubuntu/dce/build/lib/python2.7/site-packages");

  dce.AddEnvironment ("PYTHONHOME", "/home/ubuntu/dce/source/ns-3-dce/myscripts/python:/home/ubuntu/dce/source/ns-3-dce:/home/ubuntu/dce/build/lib/python27.zip:/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/plat-linux2:/home/ubuntu/dce/build/lib/python2.7/lib-tk:/home/ubuntu/dce/build/lib/python2.7/lib-old:/home/ubuntu/dce/build/lib/lib-dynload:/home/ubuntu/dce/build/lib/python2.7/site-packages");
  dce.AddEnvironment ("PYTHONPATH", "/home/ubuntu/dce/source/ns-3-dce/myscripts/python:/home/ubuntu/dce/source/ns-3-dce:/home/ubuntu/dce/build/lib/python27.zip:/home/ubuntu/dce/build/lib/python2.7:/home/ubuntu/dce/build/lib/python2.7/plat-linux2:/home/ubuntu/dce/build/lib/python2.7/lib-tk:/home/ubuntu/dce/build/lib/python2.7/lib-old:/home/ubuntu/dce/build/lib/lib-dynload:/home/ubuntu/dce/build/lib/python2.7/site-packages");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.0));

  Address sinkAddress (InetSocketAddress (interfaces.GetAddress (0), 8888));
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (20.));

  Simulator::Stop (Seconds(30.0));
  Simulator::Run ();
  Simulator::Destroy ();
}
