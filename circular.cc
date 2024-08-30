
#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "myapp.h"

NS_LOG_COMPONENT_DEFINE ("MyBlackhole");

using namespace ns3;

void
ReceivePacket(Ptr<const Packet> p, const Address & addr)
{
	std::cout << Simulator::Now ().GetSeconds () << "\t" << p->GetSize() <<"\n";
}


int main (int argc, char *argv[])
{

    bool enableFlowMonitor = false;
    std::string phyMode ("DsssRate1Mbps");
    
    CommandLine cmd;
    cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
    cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
    cmd.Parse (argc, argv);
    
    //Setting up wifi
    WifiHelper wifi;

    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);

    YansWifiChannelHelper wifiChannel ;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel",
                                        "SystemLoss", DoubleValue(1),
                                        "HeightAboveZ", DoubleValue(1.5));

    // For range near 250m
    wifiPhy.Set ("TxPowerStart", DoubleValue(33));
    wifiPhy.Set ("TxPowerEnd", DoubleValue(33));
    wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
    wifiPhy.Set ("TxGain", DoubleValue(0));
    wifiPhy.Set ("RxGain", DoubleValue(0));
    wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-61.8));
    wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-64.8));

    wifiPhy.SetChannel (wifiChannel.Create ());

    // Add a non-QoS upper mac
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    wifiMac.SetType ("ns3::AdhocWifiMac");

    // Set 802.11b standard
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                    "DataMode",StringValue(phyMode),
                                    "ControlMode",StringValue(phyMode));

    uint32_t nNodes = 10; //assign the number of nodes
    NodeContainer c; 
    NodeContainer not_malicious_nodes;
    NodeContainer malicious_nodes;
    c.Create(nNodes); //We create nNodes number of nodes
    
    malicious_nodes.Add(c.Get(0));
    
    for (uint32_t i = 1; i < nNodes; i++){
        not_malicious_nodes.Add(c.Get(i));
    }
    
    //3 setting up wifi
    NetDeviceContainer devices;
    devices = wifi.Install (wifiPhy, wifiMac, c);
    
    //4 oadv
    AodvHelper aodv;
    AodvHelper malicious_aodv;  

    //5 install aodv and evil aodv on nodes using internet helper
    InternetStackHelper internet;
    internet.SetRoutingHelper (aodv);
    internet.Install (not_malicious_nodes);
    
    malicious_aodv.Set("IsMalicious",BooleanValue(true)); 
    internet.SetRoutingHelper (malicious_aodv);
    internet.Install (malicious_nodes);
    
    //6 Set up Addresses
    Ipv4AddressHelper ipv4;
    NS_LOG_INFO ("Assign IP Addresses.");
    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);
    //7 UDP connections
    NS_LOG_INFO ("Create Applications.");

    // UDP connection from N1 to N9
    uint16_t sinkPort = 6;
    Address sinkAddress (InetSocketAddress (ifcont.GetAddress (9), sinkPort)); // interface of n9
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (c.Get (9)); //n9 as sink
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (15.));
    
    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (c.Get (1), UdpSocketFactory::GetTypeId ()); //source 
    
    // Create UDP application at n1
    Ptr<MyApp> app = CreateObject<MyApp> ();
    app->Setup (ns3UdpSocket, sinkAddress, 1040, 5, DataRate ("250Kbps"));
    c.Get (1)->AddApplication (app);
    app->SetStartTime (Seconds (5.));
    app->SetStopTime (Seconds (15.));

    //8 mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();

    const int numNodes = 10;
    const double radius = 250.0;
    double angleStep = 2 * M_PI / numNodes;

    for (int i = 0; i < numNodes; ++i) {
        double angle = i * angleStep;
        double x = radius * cos(angle);
        double y = radius * sin(angle);
        positionAlloc->Add(Vector(x, y, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(c);

    //10 set up anim interface
    AnimationInterface anim ("circular.xml"); // Mandatory

    // Create a resource from an image file.
    uint32_t laptop_img = anim.AddResource ("/home/maria16/Pictures/laptop.png");
    uint32_t evillaptop_img = anim.AddResource ("/home/maria16/Pictures/evillaptop.png");
    uint32_t smartphone_img = anim.AddResource ("/home/maria16/Pictures/smartphone.png");

    //update node image and size
    anim.UpdateNodeImage (0, evillaptop_img);
    anim.UpdateNodeSize (0,30,30);

    for (uint32_t i = 1; i < nNodes; i++) {
        //benign nodes are green
        if(i==1 || i==7 || i==8){
            anim.UpdateNodeImage (i, smartphone_img);
            //anim.UpdateNodeSize (i,30,20);
            anim.UpdateNodeSize (i,30,20);
        }else {
            anim.UpdateNodeImage (i, laptop_img);
            //anim.UpdateNodeSize (i,60,60);
            anim.UpdateNodeSize (i,30,30);
        }
        
    }

    anim.EnablePacketMetadata(true);
    
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("circular.routes", std::ios::out);
    aodv.PrintRoutingTableAllAt (Seconds (10), routingStream);

    // Trace Received Packets
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback (&ReceivePacket));
    
    // Calculate Throughput using Flowmonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    // Now, do the actual simulation.
    //
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (Seconds(15.0));
    Simulator::Run ();

    
    monitor->CheckForLostPackets ();
    
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
        {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        if ((t.sourceAddress=="10.1.2.2" && t.destinationAddress == "10.1.2.10"))
        {
            std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
            std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
            std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
        }
        }

    monitor->SerializeToXmlFile("circular.flowmon", true, true);
    return 0;

}