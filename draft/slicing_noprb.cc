#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"
#include <fstream>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("3gppChannelNumsFdm");

void LoadConfigFromFile(const std::string& configFile,
                       uint16_t& gNbNum,
                       uint16_t& ueNum,
                       uint32_t& udpPacketSizeVideo,
                       uint32_t& udpPacketSizeVoice,
                       uint32_t& udpPacketSizeGaming,
                       uint32_t& lambdaVideo,
                       uint32_t& lambdaVoice,
                       uint32_t& lambdaGaming,
                       uint32_t& simTimeMs,
                       uint32_t& udpAppStartTimeMs,
                       double& centralFrequencyBand1,
                       double& bandwidthBand1,
                       double& centralFrequencyBand2,
                       double& bandwidthBand2,
                       double& totalTxPower,
                       std::string& simTag,
                       std::string& outputDir,
                       bool& enableVideo,
                       bool& enableVoice,
                       bool& enableGaming)
{
    std::ifstream file(configFile);
    if (!file.is_open())
    {
        NS_LOG_WARN("Could not open config file: " << configFile << ". Using default values.");
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value))
        {
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (key == "gNbNum") gNbNum = std::stoi(value);
            else if (key == "ueNum") ueNum = std::stoi(value);
            else if (key == "udpPacketSizeVideo") udpPacketSizeVideo = std::stoul(value);
            else if (key == "udpPacketSizeVoice") udpPacketSizeVoice = std::stoul(value);
            else if (key == "udpPacketSizeGaming") udpPacketSizeGaming = std::stoul(value);
            else if (key == "lambdaVideo") lambdaVideo = std::stoul(value);
            else if (key == "lambdaVoice") lambdaVoice = std::stoul(value);
            else if (key == "lambdaGaming") lambdaGaming = std::stoul(value);
            else if (key == "simTimeMs") simTimeMs = std::stoul(value);
            else if (key == "udpAppStartTimeMs") udpAppStartTimeMs = std::stoul(value);
            else if (key == "centralFrequencyBand1") centralFrequencyBand1 = std::stod(value);
            else if (key == "bandwidthBand1") bandwidthBand1 = std::stod(value);
            else if (key == "centralFrequencyBand2") centralFrequencyBand2 = std::stod(value);
            else if (key == "bandwidthBand2") bandwidthBand2 = std::stod(value);
            else if (key == "totalTxPower") totalTxPower = std::stod(value);
            else if (key == "simTag") simTag = value;
            else if (key == "outputDir") outputDir = value;
            else if (key == "enableVideo") enableVideo = (value == "true" || value == "1");
            else if (key == "enableVoice") enableVoice = (value == "true" || value == "1");
            else if (key == "enableGaming") enableGaming = (value == "true" || value == "1");
        }
    }
    file.close();
}

int main(int argc, char* argv[])
{
    uint16_t gNbNum = 4;
    uint16_t ueNum = 4;
    uint32_t udpPacketSizeVideo = 100;
    uint32_t udpPacketSizeVoice = 1252;
    uint32_t udpPacketSizeGaming = 500;
    uint32_t lambdaVideo = 50;
    uint32_t lambdaVoice = 100;
    uint32_t lambdaGaming = 250;
    uint32_t simTimeMs = 1400;
    uint32_t udpAppStartTimeMs = 400;
    double centralFrequencyBand1 = 28e9;
    double bandwidthBand1 = 100e6;
    double centralFrequencyBand2 = 28.2e9;
    double bandwidthBand2 = 100e6;
    double totalTxPower = 4;
    std::string simTag = "default";
    std::string outputDir = "./";
    bool enableVideo = true;
    bool enableVoice = true;
    bool enableGaming = true;
    std::string configFile = "";

    CommandLine cmd(__FILE__);
    cmd.AddValue("configFile", "Path to configuration file", configFile);
    cmd.AddValue("gNbNum", "Number of gNBs", gNbNum);
    cmd.AddValue("ueNum", "Number of UEs", ueNum);
    cmd.AddValue("packetSizeVideo", "Packet size for video traffic", udpPacketSizeVideo);
    cmd.AddValue("packetSizeVoice", "Packet size for voice traffic", udpPacketSizeVoice);
    cmd.AddValue("packetSizeGaming", "Packet size for gaming traffic", udpPacketSizeGaming);
    cmd.AddValue("lambdaVideo", "UDP packets per second for video", lambdaVideo);
    cmd.AddValue("lambdaVoice", "UDP packets per second for voice", lambdaVoice);
    cmd.AddValue("lambdaGaming", "UDP packets per second for gaming", lambdaGaming);
    cmd.AddValue("enableVideo", "Enable video traffic", enableVideo);
    cmd.AddValue("enableVoice", "Enable voice traffic", enableVoice);
    cmd.AddValue("enableGaming", "Enable gaming traffic", enableGaming);
    cmd.AddValue("simTimeMs", "Simulation time", simTimeMs);
    cmd.AddValue("centralFrequencyBand1", "Central frequency for band 1", centralFrequencyBand1);
    cmd.AddValue("bandwidthBand1", "Bandwidth for band 1", bandwidthBand1);
    cmd.AddValue("centralFrequencyBand2", "Central frequency for band 2", centralFrequencyBand2);
    cmd.AddValue("bandwidthBand2", "Bandwidth for band 2", bandwidthBand2);
    cmd.AddValue("totalTxPower", "Total transmission power", totalTxPower);
    cmd.AddValue("simTag", "Simulation tag", simTag);
    cmd.AddValue("outputDir", "Output directory", outputDir);
    cmd.Parse(argc, argv);

    if (!configFile.empty())
    {
        LoadConfigFromFile(configFile, gNbNum, ueNum,
                          udpPacketSizeVideo, udpPacketSizeVoice, udpPacketSizeGaming,
                          lambdaVideo, lambdaVoice, lambdaGaming,
                          simTimeMs, udpAppStartTimeMs,
                          centralFrequencyBand1, bandwidthBand1,
                          centralFrequencyBand2, bandwidthBand2,
                          totalTxPower, simTag, outputDir,
                          enableVideo, enableVoice, enableGaming);
    }

    NS_ABORT_IF(centralFrequencyBand1 > 100e9);
    NS_ABORT_IF(centralFrequencyBand2 > 100e9);

    Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(999999999));

    int64_t randomStream = 1;

    GridScenarioHelper gridScenario;
    gridScenario.SetRows(gNbNum / 2);
    gridScenario.SetColumns(gNbNum);
    gridScenario.SetHorizontalBsDistance(5.0);
    gridScenario.SetBsHeight(10.0);
    gridScenario.SetUtHeight(1.5);
    gridScenario.SetSectorization(GridScenarioHelper::SINGLE);
    gridScenario.SetBsNumber(gNbNum);
    gridScenario.SetUtNumber(ueNum);
    gridScenario.SetScenarioHeight(3);
    gridScenario.SetScenarioLength(3);
    randomStream += gridScenario.AssignStreams(randomStream);
    gridScenario.CreateScenario();

    Ptr<NrPointToPointEpcHelper> nrEpcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper>();

    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(nrEpcHelper);
    
    channelHelper->ConfigureFactories("UMi", "Default", "ThreeGpp");
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(0)));
    channelHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(0)));
    channelHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false));

    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;

    CcBwpCreator::SimpleOperationBandConf bandConfTdd(centralFrequencyBand1,
                                                      bandwidthBand1,
                                                      numCcPerBand);

    CcBwpCreator::SimpleOperationBandConf bandConfFdd(centralFrequencyBand2,
                                                      bandwidthBand2,
                                                      numCcPerBand);

    bandConfFdd.m_numBwp = 2;

    OperationBandInfo bandTdd = ccBwpCreator.CreateOperationBandContiguousCc(bandConfTdd);
    OperationBandInfo bandFdd = ccBwpCreator.CreateOperationBandContiguousCc(bandConfFdd);
    channelHelper->AssignChannelsToBands({bandTdd, bandFdd});

    allBwps = CcBwpCreator::GetAllBwps({bandTdd, bandFdd});
    
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(DirectPathBeamforming::GetTypeId()));

    nrEpcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));

    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<IsotropicAntennaModel>()));

    nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(4.0));

    uint32_t bwpIdForVoice = 0;
    uint32_t bwpIdForVideo = 1;
    uint32_t bwpIdForGaming = 2;

    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_CONV_VOICE", UintegerValue(bwpIdForVoice));
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_CONV_VIDEO", UintegerValue(bwpIdForVideo));
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_GAMING", UintegerValue(bwpIdForGaming));

    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_CONV_VOICE", UintegerValue(bwpIdForVoice));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_CONV_VIDEO", UintegerValue(bwpIdForVideo));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_GAMING", UintegerValue(bwpIdForGaming));

    NetDeviceContainer gnbNetDev =
        nrHelper->InstallGnbDevice(gridScenario.GetBaseStations(), allBwps);
    NetDeviceContainer ueNetDev =
        nrHelper->InstallUeDevice(gridScenario.GetUserTerminals(), allBwps);

    randomStream += nrHelper->AssignStreams(gnbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    NS_ASSERT(gnbNetDev.GetN() == 4);

    for (uint32_t gnbIdx = 0; gnbIdx < gnbNetDev.GetN(); ++gnbIdx)
    {
        uint32_t numerology = gnbIdx;
        
        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 0)->SetAttribute("Numerology", UintegerValue(numerology));
        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 0)->SetAttribute("Pattern", StringValue("F|F|F|F|F|F|F|F|F|F|"));
        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 0)->SetAttribute("TxPower", DoubleValue(4.0));

        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 1)->SetAttribute("Numerology", UintegerValue(numerology));
        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 1)->SetAttribute("Pattern", StringValue("DL|DL|DL|DL|DL|DL|DL|DL|DL|DL|"));
        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 1)->SetAttribute("TxPower", DoubleValue(4.0));

        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 2)->SetAttribute("Numerology", UintegerValue(numerology));
        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 2)->SetAttribute("Pattern", StringValue("UL|UL|UL|UL|UL|UL|UL|UL|UL|UL|"));
        NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), 2)->SetAttribute("TxPower", DoubleValue(0.0));

        NrHelper::GetBwpManagerGnb(gnbNetDev.Get(gnbIdx))->SetOutputLink(2, 1);
    }

    for (uint32_t i = 0; i < ueNetDev.GetN(); i++)
    {
        NrHelper::GetBwpManagerUe(ueNetDev.Get(i))->SetOutputLink(1, 2);
    }

    auto [remoteHost, remoteHostIpv4Address] =
        nrEpcHelper->SetupRemoteHost("100Gb/s", 2500, Seconds(0.000));

    InternetStackHelper internet;
    internet.Install(gridScenario.GetUserTerminals());

    Ipv4InterfaceContainer ueIpIface =
        nrEpcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNetDev));

    for (uint32_t i = 0; i < ueNetDev.GetN(); ++i)
    {
        auto gnbDev = DynamicCast<NrGnbNetDevice>(gnbNetDev.Get(i));
        auto ueDev = DynamicCast<NrUeNetDevice>(ueNetDev.Get(i));
        NS_ASSERT(gnbDev != nullptr);
        NS_ASSERT(ueDev != nullptr);
        nrHelper->AttachToGnb(ueDev, gnbDev);
    }

    uint16_t dlPortVideo = 1234;
    uint16_t dlPortVoice = 1235;
    uint16_t ulPortGaming = 1236;

    ApplicationContainer serverApps;

    UdpServerHelper dlPacketSinkVideo(dlPortVideo);
    UdpServerHelper dlPacketSinkVoice(dlPortVoice);
    UdpServerHelper ulPacketSinkVoice(ulPortGaming);

    serverApps.Add(dlPacketSinkVideo.Install(gridScenario.GetUserTerminals()));
    serverApps.Add(dlPacketSinkVoice.Install(gridScenario.GetUserTerminals()));
    serverApps.Add(ulPacketSinkVoice.Install(remoteHost));

    UdpClientHelper dlClientVideo;
    dlClientVideo.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    dlClientVideo.SetAttribute("PacketSize", UintegerValue(udpPacketSizeVideo));
    dlClientVideo.SetAttribute("Interval", TimeValue(Seconds(1.0 / lambdaVideo)));

    NrEpsBearer videoBearer(NrEpsBearer::GBR_CONV_VIDEO);
    Ptr<NrEpcTft> videoTft = Create<NrEpcTft>();
    NrEpcTft::PacketFilter dlpfVideo;
    dlpfVideo.localPortStart = dlPortVideo;
    dlpfVideo.localPortEnd = dlPortVideo;
    videoTft->Add(dlpfVideo);

    UdpClientHelper dlClientVoice;
    dlClientVoice.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    dlClientVoice.SetAttribute("PacketSize", UintegerValue(udpPacketSizeVoice));
    dlClientVoice.SetAttribute("Interval", TimeValue(Seconds(1.0 / lambdaVoice)));

    NrEpsBearer voiceBearer(NrEpsBearer::GBR_CONV_VOICE);
    Ptr<NrEpcTft> voiceTft = Create<NrEpcTft>();
    NrEpcTft::PacketFilter dlpfVoice;
    dlpfVoice.localPortStart = dlPortVoice;
    dlpfVoice.localPortEnd = dlPortVoice;
    voiceTft->Add(dlpfVoice);

    UdpClientHelper ulClientGaming;
    ulClientGaming.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    ulClientGaming.SetAttribute("PacketSize", UintegerValue(udpPacketSizeGaming));
    ulClientGaming.SetAttribute("Interval", TimeValue(Seconds(1.0 / lambdaGaming)));

    NrEpsBearer gamingBearer(NrEpsBearer::GBR_GAMING);
    Ptr<NrEpcTft> gamingTft = Create<NrEpcTft>();
    NrEpcTft::PacketFilter ulpfGaming;
    ulpfGaming.remotePortStart = ulPortGaming;
    ulpfGaming.remotePortEnd = ulPortGaming;
    ulpfGaming.direction = NrEpcTft::UPLINK;
    gamingTft->Add(ulpfGaming);

    ApplicationContainer clientApps;

    for (uint32_t i = 0; i < gridScenario.GetUserTerminals().GetN(); ++i)
    {
        Ptr<Node> ue = gridScenario.GetUserTerminals().Get(i);
        Ptr<NetDevice> ueDevice = ueNetDev.Get(i);
        Address ueAddress = ueIpIface.GetAddress(i);

        if (enableVoice)
        {
            dlClientVoice.SetAttribute("Remote",
                AddressValue(addressUtils::ConvertToSocketAddress(ueAddress, dlPortVoice)));
            clientApps.Add(dlClientVoice.Install(remoteHost));
            nrHelper->ActivateDedicatedEpsBearer(ueDevice, voiceBearer, voiceTft);
        }

        if (enableVideo)
        {
            dlClientVideo.SetAttribute("Remote",
                AddressValue(addressUtils::ConvertToSocketAddress(ueAddress, dlPortVideo)));
            clientApps.Add(dlClientVideo.Install(remoteHost));
            nrHelper->ActivateDedicatedEpsBearer(ueDevice, videoBearer, videoTft);
        }

        if (enableGaming)
        {
            ulClientGaming.SetAttribute("Remote",
                AddressValue(addressUtils::ConvertToSocketAddress(remoteHostIpv4Address, ulPortGaming)));
            clientApps.Add(ulClientGaming.Install(ue));
            nrHelper->ActivateDedicatedEpsBearer(ueDevice, gamingBearer, gamingTft);
        }
    }

    serverApps.Start(MilliSeconds(udpAppStartTimeMs));
    clientApps.Start(MilliSeconds(udpAppStartTimeMs));
    serverApps.Stop(MilliSeconds(simTimeMs));
    clientApps.Stop(MilliSeconds(simTimeMs));

    FlowMonitorHelper flowmonHelper;
    NodeContainer endpointNodes;
    endpointNodes.Add(remoteHost);
    endpointNodes.Add(gridScenario.GetUserTerminals());

    Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install(endpointNodes);
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));
    nrHelper->EnableTraces();
    
    Simulator::Stop(MilliSeconds(simTimeMs));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;

    std::ofstream outFile;
    std::string filename = outputDir + "/" + simTag;
    outFile.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!outFile.is_open())
    {
        std::cerr << "Can't open file " << filename << std::endl;
        return 1;
    }

    outFile.setf(std::ios_base::fixed);

    for (auto i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::stringstream protoStream;
        protoStream << (uint16_t)t.protocol;
        if (t.protocol == 6)
            protoStream.str("TCP");
        if (t.protocol == 17)
            protoStream.str("UDP");
            
        outFile << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> "
                << t.destinationAddress << ":" << t.destinationPort << ") proto "
                << protoStream.str() << "\n";
        outFile << "  Tx Packets: " << i->second.txPackets << "\n";
        outFile << "  Tx Bytes:   " << i->second.txBytes << "\n";
        outFile << "  TxOffered:  "
                << i->second.txBytes * 8.0 / ((simTimeMs - udpAppStartTimeMs) / 1000.0) / 1000.0 / 1000.0
                << " Mbps\n";
        outFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        
        if (i->second.rxPackets > 0)
        {
            double rxDuration = (simTimeMs - udpAppStartTimeMs) / 1000.0;
            averageFlowThroughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
            averageFlowDelay += 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets;

            outFile << "  Throughput: " << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000 << " Mbps\n";
            outFile << "  Mean delay:  " << 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets << " ms\n";
            outFile << "  Mean jitter:  " << 1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets << " ms\n";
        }
        else
        {
            outFile << "  Throughput:  0 Mbps\n";
            outFile << "  Mean delay:  0 ms\n";
            outFile << "  Mean jitter: 0 ms\n";
        }
        outFile << "  Rx Packets: " << i->second.rxPackets << "\n";
    }

    double meanFlowThroughput = averageFlowThroughput / stats.size();
    double meanFlowDelay = averageFlowDelay / stats.size();
    double throughputTolerance = meanFlowThroughput * 0.001;

    outFile << "\n\n  Mean flow throughput: " << meanFlowThroughput << "\n";
    outFile << "  Mean flow delay: " << meanFlowDelay << "\n";
    outFile.close();

    std::ifstream f(filename.c_str());
    if (f.is_open())
    {
        std::cout << f.rdbuf();
    }

    Simulator::Destroy();

    if (argc == 0 && (meanFlowThroughput < 0.709696 - throughputTolerance ||
                      meanFlowThroughput > 0.709696 + throughputTolerance))
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}
