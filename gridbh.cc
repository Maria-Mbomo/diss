
#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"
#include "myapp.h"


NS_LOG_COMPONENT_DEFINE ("GridBlackhole");

using namespace ns3;

void ReceivePacket (Ptr<const Packet> p, const Address & addr)
{
    std::cout << Simulator::Now().GetSeconds() << "\t" << p->GetSize() << "\n";
}

int main (int argc, char *argv[])
{
    bool enableFlowMonitor = false;
    std::string phyMode ("DsssRate1Mbps");

    CommandLine cmd;
    cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
    cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
    cmd.Parse (argc, argv);

    uint32_t nNodes = 9;  // Number of nodes
    uint32_t nBlackholeNodes = 1; // Number of blackhole nodes

    // Setting up wifi
    WifiHelper wifi;
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel", "SystemLoss", DoubleValue(1), "HeightAboveZ", DoubleValue(1.5));

    // For range near 250m
    wifiPhy.Set ("TxPowerStart", DoubleValue (33));
    wifiPhy.Set ("TxPowerEnd", DoubleValue (33));
    wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
    wifiPhy.Set ("TxGain", DoubleValue (0));
    wifiPhy.Set ("RxGain", DoubleValue (0));
    wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-61.8));
    wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-64.8));

    wifiPhy.SetChannel (wifiChannel.Create ());

    // Add a non-QoS upper mac
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    wifiMac.SetType ("ns3::AdhocWifiMac");

    // Set 802.11b standard
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode), "ControlMode", StringValue (phyMode));

    NodeContainer c;
    NodeContainer notMaliciousNodes;
    NodeContainer maliciousNodes;
    c.Create (nNodes); // Create nNodes number of nodes

    for (uint32_t i = 0; i < nBlackholeNodes; i++) {
        maliciousNodes.Add (c.Get (i)); // First node as malicious
    }

    for (uint32_t i = nBlackholeNodes; i < nNodes; i++) {
        notMaliciousNodes.Add (c.Get (i));
    }
    
    // Set up WiFi
    NetDeviceContainer devices;
    devices = wifi.Install (wifiPhy, wifiMac, c);

    // Set up AODV
    AodvHelper aodv;
    AodvHelper maliciousAodv;

    // Install AODV and malicious AODV on nodes using Internet helper
    InternetStackHelper internet;
    internet.SetRoutingHelper (aodv);
    internet.Install (notMaliciousNodes);

    maliciousAodv.Set ("IsMalicious", BooleanValue (true)); // Enable malicious behavior
    internet.SetRoutingHelper (maliciousAodv);
    internet.Install (maliciousNodes);

    // Set up unique IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);

    // UDP connections
    NS_LOG_INFO ("Create Applications.");
    uint16_t sinkPort = 6;
    Address sinkAddress (InetSocketAddress (ifcont.GetAddress (nNodes - 1), sinkPort)); // Interface of the last node
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (c.Get (nNodes - 1)); // Last node as sink
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (15.)); //100.

    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (c.Get (4), UdpSocketFactory::GetTypeId ()); // Source 

    // Create UDP application at n1
    Ptr<MyApp> app = CreateObject<MyApp> ();
    app->Setup (ns3UdpSocket, sinkAddress, 1040, 5, DataRate ("250Kbps"));
    c.Get (1)->AddApplication (app);
    app->SetStartTime (Seconds (5.)); //40.
    app->SetStopTime (Seconds (15.)); //100.

    // Setting up mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

    // Grid size
    int gridSize = 3;
    //double step = 500.0 / (gridSize - 1); // Calculate the step between nodes
    double step = 500.0 / (gridSize - 1); 

    // Loop to create positions in a 3x3 grid
    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            positionAlloc->Add(Vector(i * step, j * step, 0));
        }
    }

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install (c);

    AnimationInterface anim ("grid_blackhole.xml"); // Mandatory

    // Create a resource from an image file.
    uint32_t laptop_img = anim.AddResource ("/home/maria16/Pictures/laptop.png");
    uint32_t evillaptop_img = anim.AddResource ("/home/maria16/Pictures/evillaptop.png");
    uint32_t smartphone_img = anim.AddResource ("/home/maria16/Pictures/smartphone.png");

    //update node image and size
    anim.UpdateNodeImage (0, evillaptop_img);
    //anim.UpdateNodeSize (0,60,60);
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
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("randombhroutes", std::ios::out);
    aodv.PrintRoutingTableAllAt (Seconds (10), routingStream);

     // Trace Received Packets
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback (&ReceivePacket));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (Seconds(15.0)); 
    Simulator::Run ();

    monitor->CheckForLostPackets ();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
        {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        if ((t.sourceAddress=="10.1.1.5" && t.destinationAddress == "10.1.1.9"))
        {
            std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
            std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
            std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
        }
        }

    monitor->SerializeToXmlFile("gridstats.flowmon", true, true);

    //Simulator::Destroy ();
    return 0;
}
