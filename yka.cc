/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
    //LogComponentEnable("TcpSocketImpl", LOG_LEVEL_ALL);
    //LogComponentEnable("onOffhgh", LOG_LEVEL_ALL);

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);
    
    //sink for reciever????
    PacketSinkHelper sink ("ns3::TcpSocketFactory",Address
                           (InetSocketAddress (Ipv4Address::GetAny (), 10)));
                         
    //set a node as reciever
    ApplicationContainer app = sink.Install (nodes.Get(0));
    
    app.Start (Seconds (1.0));
    app.Stop (Seconds (10.0));

    OnOffHelper onOffHelper ("ns3::TcpSocketFactory", Address
                       (InetSocketAddress (Ipv4Address ("10.1.1.1"), 10)));
    onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

    onOffHelper.SetAttribute ("DataRate",StringValue ("2Mbps"));
    onOffHelper.SetAttribute ("PacketSize",UintegerValue(1280));
   // ApplicationContainer
    app = onOffHelper.Install (nodes.Get (1));

    app.Start(Seconds(2));
    app.Stop(Seconds(10));
    
    pointToPoint.EnablePcapAll ("testtcp");
    
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
