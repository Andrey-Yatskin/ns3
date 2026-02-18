/*
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "tutorial-app.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FifthScriptExample");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms

int
main(int argc, char* argv[])
{

    bool verbose = true;
    uint32_t nCsma = 3;
    uint32_t nWifi = 3;
    bool tracing = false; 
    
    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);
    
    cmd.Parse(argc, argv);
    
    if (nWifi > 18)
    {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box"
                  << std::endl;
        return 1;
    }

    if (verbose)
    
// UdpEchoClientApplication ЗАМЕНИТЬ НА TCP...
    
    {
        //LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        //LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
        
        // ЗАМЕНИТЬ INFO НА ALL.
LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);

        // ЗАМЕНИТЬ INFO НА ALL. И УБРАТЬ ВОВСЕ
LogComponentEnable("TcpL4Protocol", LOG_LEVEL_INFO);
    }
    
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));
    Config::SetDefault("ns3::TcpL4Protocol::RecoveryType", TypeIdValue(TypeId::LookupByName("ns3::TcpClassicRecovery")));

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

// ЭТОТ СКРИПТ ПРИЗВАН ОБЪЯСНИТЬ ВОЗМОЖНОСТИ ТРАССИРОВКИ. ОДНАКО, В ИДЕАЛНЫХ УСЛОВИЯХ МЫ УВИДИМ БЕЗУПРЕЧНУЮ КАРТИНУ. ЧТОБЫ ВНЕСТИ ИНТЕРЕС ДОБАВИМ ОБЪЕКТ RATE ERROR MODEL. ЗАДАДИМ СВОЙСТВА И УСТАНОВИМ НА DEVICES(1).

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(nCsma);
    
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    
    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    
    NodeContainer wifiApNode = p2pNodes.Get(0);
    
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;
    
    NetDeviceContainer staDevices;
    
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    
    staDevices = wifi.Install(phy, mac, wifiStaNodes);
    
    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, wifiApNode);
 
    MobilityHelper mobility;
    
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));
    
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
                              
    mobility.Install(wifiStaNodes);
    
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);


    InternetStackHelper stack;
    stack.Install(csmaNodes);
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    address.Assign(staDevices);
    address.Assign(apDevices);
    
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(csmaNodes.Get(nCsma));
    serverApps.Start(Seconds(1));
    serverApps.Stop(Seconds(10));
    
    UdpEchoClientHelper echoClient(csmaInterfaces.GetAddress(nCsma), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    
    ApplicationContainer clientApps = echoClient.Install(wifiStaNodes.Get(nWifi - 1));
    clientApps.Start(Seconds(2));
    clientApps.Stop(Seconds(10));
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

// ПОСКОЛЬКУ ИСПОЛЬЗУЕМ TCP, ТО НУЖНО ЧТО-ТО НА УЗЛЕ НАЗНАЧЕНИЯ ДЛЯ TCP-СОЕДИНЕНИЙ И ПОЛУЧЕНИЯ ДАННЫХ. ОБЫЧНО ИСПОЛЬЗУЕТСЯ ПРИЛОЖЕНИЕ PACKET SINK. 

    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(p2pInterfaces.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(p2pNodes.Get(1));
    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(20.));

// СОЗДАЕМ СОКЕТ. ПЕРВАЯ СТРОКА, ПО СУТИ ТОТ ЖЕ ХЕЛПЕР, ТОЛЬКО БОЛЕЕ НИЗКОУРОВНЕВЫЙ. И СРАЗУ ПРИСОЕДИНЯЕМ ЕГО К УЗЛУ 0.

    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(p2pNodes.Get(0), TcpSocketFactory::GetTypeId());
//    ns3TcpSocket-
//>TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndChange));

// ПЕРВАЯ СТРОКА СОЗДАЕТ ПРИЛОЖЕНИЕ ТИПА TUTORIAL APP. ВТОРАЯ СТРОКА СООБЩАЕТ КАКОЙ СОКЕТ ИСПОЛЬЗОВАТЬ, К КАКОМУ АДРЕСУ ПОДКЛЮЧАТЬСЯ, СКОЛКО ДАННЫХ ОТПРАВЛЯТЬ ПО КАЖДОМУ СОБЫТИЮ ОТПРАВКИ, СКОЛЬКО СГЕНЕРИРОВАТЬ СОБЫТИЙ ОТПРАВКИ И БИТРЕЙТ, С КОТОРЫМ ВЫДАВАТЬ ДАННЫЕ ПО ЭТИМ СОБЫТИЯМ.

//    Ptr<TutorialApp> app = CreateObject<TutorialApp>();
//    app->Setup(ns3TcpSocket, sinkAddress, 1040, 1000, DataRate("1Mbps"));
//    p2pNodes.Get(0)->AddApplication(app);
//    app->SetStartTime(Seconds(1.));
//    app->SetStopTime(Seconds(20.));


//    p2pDevices.Get(1)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&RxDrop));

    Simulator::Stop(Seconds(20));
    
    if (tracing)
    {
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        pointToPoint.EnablePcapAll("third");
        phy.EnablePcap("third", apDevices.Get(0));
        csma.EnablePcap("third", csmaDevices.Get(0), true);
    }
    
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
