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
#include <yaml-cpp/yaml.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NetworkSlicingFdm");

struct SliceConfig {
    bool enabled;
    uint32_t packetSize;
    uint32_t lambda;
    std::string bearerType;
};

struct GnbBwpConfig {
    uint32_t bwpId;
    uint32_t numerology;
    double centerFrequency;
    double bandwidth;
    std::string pattern;
    double txPower;
};

struct SimConfig {
    uint16_t gNbNum;
    uint16_t ueNum;
    uint32_t simTimeMs;
    uint32_t udpAppStartTimeMs;
    std::string simTag;
    std::string outputDir;
    
    double bandCenterFrequency;
    double bandBandwidth;
    
    SliceConfig slice1;
    SliceConfig slice2;
    SliceConfig slice3;
    
    std::map<uint32_t, std::vector<GnbBwpConfig>> gnbBwpConfigs;
};

SimConfig LoadConfigFromYaml(const std::string& yamlFile)
{
    SimConfig config;
    
    try {
        YAML::Node root = YAML::LoadFile(yamlFile);
        
        config.gNbNum = root["simulation"]["gNbNum"].as<uint16_t>();
        config.ueNum = root["simulation"]["ueNum"].as<uint16_t>();
        config.simTimeMs = root["simulation"]["simTimeMs"].as<uint32_t>();
        config.udpAppStartTimeMs = root["simulation"]["udpAppStartTimeMs"].as<uint32_t>();
        config.simTag = root["simulation"]["simTag"].as<std::string>();
        config.outputDir = root["simulation"]["outputDir"].as<std::string>();
        
        config.bandCenterFrequency = root["band"]["centerFrequency"].as<double>();
        config.bandBandwidth = root["band"]["bandwidth"].as<double>();
        
        auto slice1 = root["slices"]["slice1"];
        config.slice1.enabled = slice1["enabled"].as<bool>();
        config.slice1.packetSize = slice1["packetSize"].as<uint32_t>();
        config.slice1.lambda = slice1["lambda"].as<uint32_t>();
        config.slice1.bearerType = slice1["bearerType"].as<std::string>();
        
        auto slice2 = root["slices"]["slice2"];
        config.slice2.enabled = slice2["enabled"].as<bool>();
        config.slice2.packetSize = slice2["packetSize"].as<uint32_t>();
        config.slice2.lambda = slice2["lambda"].as<uint32_t>();
        config.slice2.bearerType = slice2["bearerType"].as<std::string>();
        
        auto slice3 = root["slices"]["slice3"];
        config.slice3.enabled = slice3["enabled"].as<bool>();
        config.slice3.packetSize = slice3["packetSize"].as<uint32_t>();
        config.slice3.lambda = slice3["lambda"].as<uint32_t>();
        config.slice3.bearerType = slice3["bearerType"].as<std::string>();
        
        auto gnbConfigs = root["gnb_bwp_configs"];
        for (auto gnbNode : gnbConfigs) {
            uint32_t gnbId = gnbNode["gnbId"].as<uint32_t>();
            std::vector<GnbBwpConfig> bwps;
            
            for (auto bwpNode : gnbNode["bwps"]) {
                GnbBwpConfig bwp;
                bwp.bwpId = bwpNode["bwpId"].as<uint32_t>();
                bwp.numerology = bwpNode["numerology"].as<uint32_t>();
                bwp.centerFrequency = bwpNode["centerFrequency"].as<double>();
                bwp.bandwidth = bwpNode["bandwidth"].as<double>();
                bwp.pattern = bwpNode["pattern"].as<std::string>();
                bwp.txPower = bwpNode["txPower"].as<double>();
                bwps.push_back(bwp);
            }
            
            config.gnbBwpConfigs[gnbId] = bwps;
        }
        
    } catch (const YAML::Exception& e) {
        NS_FATAL_ERROR("Error parsing YAML file: " << e.what());
    }
    
    return config;
}

int main(int argc, char* argv[])
{
    std::string configFile = "config.yaml";

    CommandLine cmd(__FILE__);
    cmd.AddValue("configFile", "Path to YAML configuration file", configFile);
    cmd.Parse(argc, argv);

    SimConfig config = LoadConfigFromYaml(configFile);

    NS_ABORT_IF(config.bandCenterFrequency > 100e9);

    Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(999999999));

    int64_t randomStream = 1;

    GridScenarioHelper gridScenario;
    gridScenario.SetRows(config.gNbNum / 2);
    gridScenario.SetColumns(config.gNbNum);
    gridScenario.SetHorizontalBsDistance(5.0);
    gridScenario.SetBsHeight(10.0);
    gridScenario.SetUtHeight(1.5);
    gridScenario.SetSectorization(GridScenarioHelper::SINGLE);
    gridScenario.SetBsNumber(config.gNbNum);
    gridScenario.SetUtNumber(config.ueNum);
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

    CcBwpCreator::SimpleOperationBandConf bandConf(config.bandCenterFrequency,
                                                    config.bandBandwidth,
                                                    numCcPerBand);

    bandConf.m_numBwp = 3;

    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);
    channelHelper->AssignChannelsToBands({band});

    allBwps = CcBwpCreator::GetAllBwps({band});
    
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

    uint32_t bwpIdForSlice1 = 0;
    uint32_t bwpIdForSlice2 = 1;
    uint32_t bwpIdForSlice3 = 2;

    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_CONV_VOICE", UintegerValue(bwpIdForSlice1));
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_CONV_VIDEO", UintegerValue(bwpIdForSlice2));
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_GAMING", UintegerValue(bwpIdForSlice3));

    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_CONV_VOICE", UintegerValue(bwpIdForSlice1));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_CONV_VIDEO", UintegerValue(bwpIdForSlice2));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_GAMING", UintegerValue(bwpIdForSlice3));

    NetDeviceContainer gnbNetDev =
        nrHelper->InstallGnbDevice(gridScenario.GetBaseStations(), allBwps);
    NetDeviceContainer ueNetDev =
        nrHelper->InstallUeDevice(gridScenario.GetUserTerminals(), allBwps);

    randomStream += nrHelper->AssignStreams(gnbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    for (uint32_t gnbIdx = 0; gnbIdx < gnbNetDev.GetN(); ++gnbIdx)
    {
        if (config.gnbBwpConfigs.find(gnbIdx) != config.gnbBwpConfigs.end())
        {
            auto& bwpConfigs = config.gnbBwpConfigs[gnbIdx];
            
            for (auto& bwpConfig : bwpConfigs)
            {
                NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), bwpConfig.bwpId)
                    ->SetAttribute("Numerology", UintegerValue(bwpConfig.numerology));
                NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), bwpConfig.bwpId)
                    ->SetAttribute("Pattern", StringValue(bwpConfig.pattern));
                NrHelper::GetGnbPhy(gnbNetDev.Get(gnbIdx), bwpConfig.bwpId)
                    ->SetAttribute("TxPower", DoubleValue(bwpConfig.txPower));
            }
        }
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

    uint16_t dlPortSlice1 = 1234;
    uint16_t dlPortSlice2 = 1235;
    uint16_t ulPortSlice3 = 1236;

    ApplicationContainer serverApps;

    UdpServerHelper dlPacketSinkSlice1(dlPortSlice1);
    UdpServerHelper dlPacketSinkSlice2(dlPortSlice2);
    UdpServerHelper ulPacketSinkSlice3(ulPortSlice3);

    serverApps.Add(dlPacketSinkSlice1.Install(gridScenario.GetUserTerminals()));
    serverApps.Add(dlPacketSinkSlice2.Install(gridScenario.GetUserTerminals()));
    serverApps.Add(ulPacketSinkSlice3.Install(remoteHost));

    UdpClientHelper dlClientSlice1;
    dlClientSlice1.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    dlClientSlice1.SetAttribute("PacketSize", UintegerValue(config.slice1.packetSize));
    dlClientSlice1.SetAttribute("Interval", TimeValue(Seconds(1.0 / config.slice1.lambda)));

    NrEpsBearer slice1Bearer(NrEpsBearer::GBR_CONV_VOICE);
    Ptr<NrEpcTft> slice1Tft = Create<NrEpcTft>();
    NrEpcTft::PacketFilter dlpfSlice1;
    dlpfSlice1.localPortStart = dlPortSlice1;
    dlpfSlice1.localPortEnd = dlPortSlice1;
    slice1Tft->Add(dlpfSlice1);

    UdpClientHelper dlClientSlice2;
    dlClientSlice2.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    dlClientSlice2.SetAttribute("PacketSize", UintegerValue(config.slice2.packetSize));
    dlClientSlice2.SetAttribute("Interval", TimeValue(Seconds(1.0 / config.slice2.lambda)));

    NrEpsBearer slice2Bearer(NrEpsBearer::GBR_CONV_VIDEO);
    Ptr<NrEpcTft> slice2Tft = Create<NrEpcTft>();
    NrEpcTft::PacketFilter dlpfSlice2;
    dlpfSlice2.localPortStart = dlPortSlice2;
    dlpfSlice2.localPortEnd = dlPortSlice2;
    slice2Tft->Add(dlpfSlice2);

    UdpClientHelper ulClientSlice3;
    ulClientSlice3.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    ulClientSlice3.SetAttribute("PacketSize", UintegerValue(config.slice3.packetSize));
    ulClientSlice3.SetAttribute("Interval", TimeValue(Seconds(1.0 / config.slice3.lambda)));

    NrEpsBearer slice3Bearer(NrEpsBearer::GBR_GAMING);
    Ptr<NrEpcTft> slice3Tft = Create<NrEpcTft>();
    NrEpcTft::PacketFilter ulpfSlice3;
    ulpfSlice3.remotePortStart = ulPortSlice3;
    ulpfSlice3.remotePortEnd = ulPortSlice3;
    ulpfSlice3.direction = NrEpcTft::UPLINK;
    slice3Tft->Add(ulpfSlice3);

    ApplicationContainer clientApps;

    for (uint32_t i = 0; i < gridScenario.GetUserTerminals().GetN(); ++i)
    {
        Ptr<Node> ue = gridScenario.GetUserTerminals().Get(i);
        Ptr<NetDevice> ueDevice = ueNetDev.Get(i);
        Address ueAddress = ueIpIface.GetAddress(i);

        if (config.slice1.enabled)
        {
            dlClientSlice1.SetAttribute("Remote",
                AddressValue(addressUtils::ConvertToSocketAddress(ueAddress, dlPortSlice1)));
            clientApps.Add(dlClientSlice1.Install(remoteHost));
            nrHelper->ActivateDedicatedEpsBearer(ueDevice, slice1Bearer, slice1Tft);
        }

        if (config.slice2.enabled)
        {
            dlClientSlice2.SetAttribute("Remote",
                AddressValue(addressUtils::ConvertToSocketAddress(ueAddress, dlPortSlice2)));
            clientApps.Add(dlClientSlice2.Install(remoteHost));
            nrHelper->ActivateDedicatedEpsBearer(ueDevice, slice2Bearer, slice2Tft);
        }

        if (config.slice3.enabled)
        {
            ulClientSlice3.SetAttribute("Remote",
                AddressValue(addressUtils::ConvertToSocketAddress(remoteHostIpv4Address, ulPortSlice3)));
            clientApps.Add(ulClientSlice3.Install(ue));
            nrHelper->ActivateDedicatedEpsBearer(ueDevice, slice3Bearer, slice3Tft);
        }
    }

    serverApps.Start(MilliSeconds(config.udpAppStartTimeMs));
    clientApps.Start(MilliSeconds(config.udpAppStartTimeMs));
    serverApps.Stop(MilliSeconds(config.simTimeMs));
    clientApps.Stop(MilliSeconds(config.simTimeMs));

    FlowMonitorHelper flowmonHelper;
    NodeContainer endpointNodes;
    endpointNodes.Add(remoteHost);
    endpointNodes.Add(gridScenario.GetUserTerminals());

    Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install(endpointNodes);
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));
    nrHelper->EnableTraces();
    
    Simulator::Stop(MilliSeconds(config.simTimeMs));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;

    std::ofstream outFile;
    std::string filename = config.outputDir + "/" + config.simTag;
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
                << i->second.txBytes * 8.0 / ((config.simTimeMs - config.udpAppStartTimeMs) / 1000.0) / 1000.0 / 1000.0
                << " Mbps\n";
        outFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        
        if (i->second.rxPackets > 0)
        {
            double rxDuration = (config.simTimeMs - config.udpAppStartTimeMs) / 1000.0;
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

    outFile << "\n\n  Mean flow throughput: " << meanFlowThroughput << "\n";
    outFile << "  Mean flow delay: " << meanFlowDelay << "\n";
    outFile.close();

    std::ifstream f(filename.c_str());
    if (f.is_open())
    {
        std::cout << f.rdbuf();
    }

    Simulator::Destroy();
    return EXIT_SUCCESS;
}
