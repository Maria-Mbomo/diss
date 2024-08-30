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

#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;

void
ReceivePacket(Ptr<const Packet> p, const Address & addr)
{
	std::cout << Simulator::Now ().GetSeconds () << "\t" << p->GetSize() <<"\n";
}

void RunSimulation(uint32_t numNodes, double nodeSpeed, double& totalThroughput, double& totalEED, double& totalPDR) {
    bool enableFlowMonitor = false;
    std::string phyMode ("DsssRate1Mbps");

    CommandLine cmd;
    cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
    cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
    //cmd.Parse (argc, argv);

    uint32_t nBlackholeNodes = 1; // Number of blackhole nodes
    
    //NS_LOG_INFO ("Create nodes.");
    NodeContainer c; // ALL Nodes
    NodeContainer not_malicious;
    NodeContainer malicious;
    c.Create(numNodes);

    for (uint32_t i = 0; i < nBlackholeNodes; i++) {
        malicious.Add (c.Get (i)); 
    }

    for (uint32_t i = nBlackholeNodes; i < numNodes; i++) {
        not_malicious.Add (c.Get (i));
    }

    // Set up WiFi
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
    WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac"); 

    // Set 802.11b standard
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                    "DataMode",StringValue(phyMode),
                                    "ControlMode",StringValue(phyMode));

    NetDeviceContainer devices;
    devices = wifi.Install (wifiPhy, wifiMac, c);

    //  Enable AODV
    AodvHelper aodv;
    AodvHelper malicious_aodv; 
    
    // Set up internet stack
    InternetStackHelper internet;
    internet.SetRoutingHelper (aodv);
    internet.Install (not_malicious); 
    malicious_aodv.Set("IsMalicious",BooleanValue(true)); 
   
    internet.SetRoutingHelper (malicious_aodv); 
    internet.Install(malicious); 
        
    // Set up Addresses
    Ipv4AddressHelper ipv4;
    //NS_LOG_INFO ("Assign IP Addresses.");
    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);


    //NS_LOG_INFO ("Create Applications.");


    uint16_t sinkPort = 6;
    Address sinkAddress (InetSocketAddress (ifcont.GetAddress (numNodes - 1), sinkPort)); // interface of n3
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (c.Get (numNodes -1)); //setting sink
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (100.));

    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (c.Get (1), UdpSocketFactory::GetTypeId ()); //source at n1

    // Create UDP application at n1
    Ptr<MyApp> app = CreateObject<MyApp> ();
    app->Setup (ns3UdpSocket, sinkAddress, 1040, 5, DataRate ("250Kbps"));
    c.Get (1)->AddApplication (app);
    app->SetStartTime (Seconds (40.));
    app->SetStopTime (Seconds (100.));

    // Set Mobility for all nodes

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();

    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable>();
    
    x->SetAttribute("Min", DoubleValue(0.0));
    x->SetAttribute("Max", DoubleValue(600.0));
    y->SetAttribute("Min", DoubleValue(0.0));
    y->SetAttribute("Max", DoubleValue(600.0));

    std::vector<Vector> positions;
    RngSeedManager::SetSeed (1);
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();

    for (uint32_t i = 0; i < numNodes; i++)
    {
        Vector pos;
        bool tooClose;
        do
        {
        tooClose = false;
        pos.x = rand->GetValue(0, 600);
        pos.y = rand->GetValue(0, 600);
        pos.z = 0;

        // Ensure the node is at least 120m away from all others
        for (std::vector<Vector>::iterator it = positions.begin(); it != positions.end(); ++it)
        {
            if (CalculateDistance(pos, *it) < 120)
            {
            tooClose = true;
            break;
            }
        }
        } while (tooClose);

        positions.push_back(pos);
        positionAlloc->Add(pos);
    }


    if( nodeSpeed > 0 ){
        std::ostringstream ss;  
        ss << nodeSpeed;  

        std::string tempString = "ns3::ConstantRandomVariable[Constant=";
        tempString += ss.str();  // Append the string form of number to tempString
        tempString += "]";

        mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                            "Bounds", RectangleValue (Rectangle (0, 1100, 0, 1100)),
                            "Speed", StringValue (tempString),
                            "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0]")
                            //"PositionAllocator", PointerValue (positionAlloc)
                            );
    }else{
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    }
    
    mobility.Install(c);

    AnimationInterface anim ("autosim.xml"); // Mandatory 
    anim.EnablePacketMetadata(true);

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowmon = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();

    totalThroughput = 0;
    totalEED = 0;
    totalPDR = 0;

    double totalReceivedPackets = 0;
    double totalSentPackets = 0;
    double totalDelay = 0;

    for (std::map<FlowId, FlowMonitor::FlowStats>::iterator it = stats.begin(); it != stats.end(); ++it) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
        double throughput = (it->second.rxBytes * 8.0) / (it->second.timeLastRxPacket.GetSeconds() - it->second.timeFirstTxPacket.GetSeconds()) / 1024;
        double delay = it->second.delaySum.GetSeconds() / it->second.rxPackets;
        double deliveryRatio = double(it->second.rxPackets) / it->second.txPackets;

        totalThroughput += throughput;
        totalDelay += delay;
        totalReceivedPackets += it->second.rxPackets;
        totalSentPackets += it->second.txPackets;
    }

    totalEED = totalDelay / stats.size();
    totalPDR = (totalReceivedPackets / totalSentPackets) * 100;
    totalThroughput /= stats.size();

    Simulator::Destroy();
}

int main(int argc, char *argv[]) {
    uint32_t nodeCounts[] = {5, 10, 15, 20, 30};
    double nodeSpeeds[] = {5.0, 10.0, 15.0, 20.0, 30.0};
    uint32_t numRuns = 20;

    std::vector<double> avgThroughputs, avgEEDs, avgPDRs;

    for (uint32_t i = 0; i < sizeof(nodeCounts)/sizeof(nodeCounts[0]); ++i) {
        uint32_t nodes = nodeCounts[i];
        double totalThroughput = 0, totalEED = 0, totalPDR = 0;
        for (uint32_t run = 0; run < numRuns; ++run) {
            double runThroughput, runEED, runPDR;
            RunSimulation(nodes, 0.0, runThroughput, runEED, runPDR);
            totalThroughput += runThroughput;
            totalEED += runEED;
            totalPDR += runPDR;
        }
        avgThroughputs.push_back(totalThroughput / numRuns);
        avgEEDs.push_back(totalEED / numRuns);
        avgPDRs.push_back(totalPDR / numRuns);
    }

    std::ofstream outFile;
    outFile.open("results_nodes.txt");
    outFile << "Nodes Throughput EED PDR\n";
    for (size_t i = 0; i < avgThroughputs.size(); ++i) {
        outFile << nodeCounts[i] << " " << avgThroughputs[i] << " " << avgEEDs[i] << " " << avgPDRs[i] << "\n";
    }
    outFile.close();

    avgThroughputs.clear();
    avgEEDs.clear();
    avgPDRs.clear();

    for (uint32_t i = 0; i < sizeof(nodeSpeeds)/sizeof(nodeSpeeds[0]); ++i) {
        double speed = nodeSpeeds[i];
        double totalThroughput = 0, totalEED = 0, totalPDR = 0;
        for (uint32_t run = 0; run < numRuns; ++run) {
            double runThroughput, runEED, runPDR;
            RunSimulation(20, speed, runThroughput, runEED, runPDR);
            totalThroughput += runThroughput;
            totalEED += runEED;
            totalPDR += runPDR;
        }
        avgThroughputs.push_back(totalThroughput / numRuns);
        avgEEDs.push_back(totalEED / numRuns);
        avgPDRs.push_back(totalPDR / numRuns);
    }

    outFile.open("results_speeds.txt");
    outFile << "Speed Throughput EED PDR\n";
    for (size_t i = 0; i < avgThroughputs.size(); ++i) {
        outFile << nodeSpeeds[i] << " " << avgThroughputs[i] << " " << avgEEDs[i] << " " << avgPDRs[i] << "\n";
    }
    outFile.close();

    // Run the plot script
    system("python plot_results.py");

    return 0;
}
