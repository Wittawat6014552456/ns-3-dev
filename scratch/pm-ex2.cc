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
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/evalvid-client-server-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/video-type-tag.h"
#include <string.h>
#include "ns3/random-variable-stream.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Pm2Example");

uint32_t Num;

uint32_t bytesTotal = 0;
uint32_t bytesTotal_cbr = 0;
uint32_t minStart;

void minValue(uint32_t intStart) {

  if (minStart > intStart) {
    minStart = intStart;
  }
  //cout << minStart << " \n";
}

void RxCallback (std::string path, Ptr<const Packet> packet, 
const Address &from)
{
SeqTsHeader seqTs;


bytesTotal += packet->GetSize ();
//std::cout <<  bytesTotal << std::endl;
}

void RxCallback_cbr (std::string path, Ptr<const Packet> packet, 
const Address &from)
{
SeqTsHeader seqTs;


bytesTotal_cbr += packet->GetSize ();
//std::cout <<  bytesTotal_cbr << std::endl;
}

void
TcEnqueue (Ptr<const QueueDiscItem> item)
{
  
   Ptr<Packet> packet = item->GetPacket ()->Copy ();
   string VideoType;
   
   // read the tag from the packet copy
  // VideoPacketTypeTag tagVideo;
  // packet->PeekPacketTag (tagVideo);
  // VideoType = tagVideo.Deserialize(TagBuffer (0));
  //std::cout << Num << std::endl;
  //  std::cout << "TcPacketsInQueue " << VideoType << std::endl;
  //if (Num < 50) {
    VideoPacketTypeTag videoTag;
    item->GetPacket()->PeekPacketTag(videoTag);

    if ((videoTag.GetVideoPacketType() == "H") || (videoTag.GetVideoPacketType() == "P") || (videoTag.GetVideoPacketType() == "B"))
    {
      Num = Num + 1;
      //std::cout << videoTag.GetInstanceTypeId() << std::endl;
      //std::cout << "videotype:" << videoTag.GetVideoPacketType()  << std::endl;
    }

    ////////std::cout << ":" << videoTag.GetVideoPacketType() << ":" << std::endl;
    //item->GetPacket ()->PeekPacketTag (videoTag);

    //std::cout << item->GetPacket()->GetPacketTagIterator(item->GetPacket()->PrintPacketTags(std::cout << std::endl)) << std::endl;
    //item->GetPacket()->PrintPacketTags(std::cout << std::endl);
   //packet->PrintPacketTags(std::cout << std::endl);
  //}
}

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 3;
  uint32_t nWifi = 10; //3

  std::string maxSize_Q = "25p"; //"50p"
  uint32_t minTh_REDQ = 1;  //5
  uint32_t minTh2_REDQ = 5;  //15
  uint32_t maxTh_REDQ = 25; //25

  std::string socketType = "ns3::UdpSocketFactory";
  double simulationTime = 90; //seconds
  double red_type = 1;
  std::string rootQueueDisc = "RED";


  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("red_type", "Number of wifi STA devices", red_type);

  cmd.AddValue ("minTh_REDQ", "Min th1 red Q", minTh_REDQ);
  cmd.AddValue ("minTh2_REDQ", "Min th2 red Q", minTh2_REDQ);

  cmd.AddValue ("rootQueueDisc", "root of queue disc", rootQueueDisc);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (nWifi > 18)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  // csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(5000000)));
  // csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  // csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps")); //5Mbps
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));


  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);


  //if (rootQueueDisc == "Red")
  //{
    TrafficControlHelper tchRed;

    if (rootQueueDisc == "RED")
    {
      tchRed.SetRootQueueDisc ("ns3::RedQueueDisc");
      Config::SetDefault ("ns3::RedQueueDisc::MaxSize", StringValue (maxSize_Q));
      Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (minTh_REDQ));
      Config::SetDefault ("ns3::RedQueueDisc::MinTh2", DoubleValue (minTh2_REDQ));
      Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (maxTh_REDQ));
      Config::SetDefault ("ns3::RedQueueDisc::type_red", DoubleValue (red_type));

    }else if (rootQueueDisc == "FIFO")
    {
      std::cout << "FIFO" << std::endl; 
      tchRed.SetRootQueueDisc ("ns3::FifoQueueDisc");
      Config::SetDefault ("ns3::FifoQueueDisc::MaxSize", StringValue (maxSize_Q));
    }

    QueueDiscContainer queueDiscs = tchRed.Install(staDevices.Get(nWifi-1)); //  (staDevices.Get(nWifi-1));
  
    //monitoring QueueDisc
    Ptr<QueueDisc> q = queueDiscs.Get(0);

    //q->TraceConnectWithoutContext ("PacketsInQueue", MakeBoundCallback (&TcPacketsInQueueTrace,q));
    q->TraceConnectWithoutContext ("Drop", MakeCallback (&TcEnqueue));
  // }
  


  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);


  uint16_t port = 9;

  double min = 0.0;
  double max = 5.0;
 
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (min));
  x->SetAttribute ("Max", DoubleValue (max));
 
  // The values returned by a uniformly distributed random
  // variable should always be within the range
  //
  //     [min, max)  .
  //
  uint32_t value;
  std::string strValue;

  value = x->GetValue ();
  minStart = value;
  minValue(value);
  strValue = std:: to_string(value);
  //std::cout << strValue << std::endl;
  
  //Create one udpServer applications on node one.
  
  EvalvidServerHelper server (port);
  server.SetAttribute ("SenderTraceFilename", StringValue("st_highway_cif.st"));  //st_paris_cif.st
  server.SetAttribute ("SenderDumpFilename", StringValue("sd_a01"));
  server.SetAttribute ("PacketPayload",UintegerValue(1014));
  ApplicationContainer apps = server.Install(wifiStaNodes.Get(nWifi-1)); // wifi node 3  
  apps.Start (Seconds (value));
  apps.Stop (Seconds (90.0));



  //std::cout << wifiStaNodes.Get(nWifi-1).GetAddress.value() << std::endl;

  //
  // Create one EvalvidClient application
  //
  EvalvidClientHelper client ("10.1.3.10",port);  // wifi node 3  10.1.3.3
  client.SetAttribute ("ReceiverDumpFilename", StringValue("rd_a01"));
  apps = client.Install(csmaNodes.Get(nCsma));  //  csma node 3  
  apps.Start (Seconds (value));
  apps.Stop (Seconds (91.0));

  value = x->GetValue ();
  minValue(value);
  strValue = std:: to_string(value);
  //std::cout << strValue << std::endl;
    OnOffHelper onoff(
        "ns3::UdpSocketFactory",
        InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    onoff.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetAttribute("DataRate", StringValue("256Kbps"));
    onoff.SetAttribute("PacketSize", StringValue("512"));
    onoff.Install(wifiStaNodes.Get(nWifi-2));
    ApplicationContainer apps_onoff_1 = onoff.Install(wifiStaNodes.Get(nWifi-2));  
    apps_onoff_1.Start(Seconds(value));
    apps_onoff_1.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_2(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_2.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_2.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_2.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_2.SetAttribute("PacketSize", StringValue("512"));
    // onoff_2.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_2 = onoff_2.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_2.Start(Seconds(value));
    // apps_onoff_2.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_3(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_3.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_3.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_3.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_3.SetAttribute("PacketSize", StringValue("512"));
    // onoff_3.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_3 = onoff_3.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_3.Start(Seconds(value));
    // apps_onoff_3.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_4(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_4.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_4.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_4.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_4.SetAttribute("PacketSize", StringValue("512"));
    // onoff_4.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_4 = onoff_4.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_4.Start(Seconds(value));
    // apps_onoff_4.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_5(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_5.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_5.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_5.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_5.SetAttribute("PacketSize", StringValue("512"));
    // onoff_5.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_5 = onoff_5.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_5.Start(Seconds(value));
    // apps_onoff_5.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_6(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_6.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_6.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_6.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_6.SetAttribute("PacketSize", StringValue("512"));
    // onoff_6.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_6 = onoff_6.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_6.Start(Seconds(value));
    // apps_onoff_6.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_7(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_7.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_7.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_7.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_7.SetAttribute("PacketSize", StringValue("512"));
    // onoff_7.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_7 = onoff_7.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_7.Start(Seconds(value));
    // apps_onoff_7.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_8(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_8.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_8.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_8.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_8.SetAttribute("PacketSize", StringValue("512"));
    // onoff_8.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_8 = onoff_8.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_8.Start(Seconds(value));
    // apps_onoff_8.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_9(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_9.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_9.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_9.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_9.SetAttribute("PacketSize", StringValue("512"));
    // onoff_9.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_9 = onoff_9.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_9.Start(Seconds(value));
    // apps_onoff_9.Stop(Seconds(simulationTime));

    // value = x->GetValue ();
    // minValue(value);
    // strValue = std:: to_string(value);
    // std::cout << strValue << std::endl;
    // OnOffHelper onoff_10(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.2.3", port));  // 10.1.2.4
    // onoff_10.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff_10.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff_10.SetAttribute("DataRate", StringValue("256Kbps"));
    // onoff_10.SetAttribute("PacketSize", StringValue("512"));
    // onoff_10.Install(wifiStaNodes.Get(nWifi-2));
    // ApplicationContainer apps_onoff_10 = onoff_10.Install(wifiStaNodes.Get(nWifi-2));  
    // apps_onoff_10.Start(Seconds(value));
    // apps_onoff_10.Stop(Seconds(simulationTime));


    

   


    Address localAddress (InetSocketAddress ("10.1.2.3", port)); //10.1.2.4
    PacketSinkHelper packetSinkHelper (socketType, localAddress);
    ApplicationContainer sinkApp = packetSinkHelper.Install(csmaNodes.Get(nCsma-1));  // node 3  // (csmaNodes.Get(nCsma));

    Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                      MakeCallback (&RxCallback_cbr));


      Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::EvalvidClient/Rx",
              MakeCallback (&RxCallback));

    // OnOffHelper onoff(
    //     "ns3::UdpSocketFactory",
    //     InetSocketAddress("10.1.3.3", port));  // 10.1.2.4
    // onoff.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff.SetAttribute("DataRate", StringValue("10Mbps"));
    // onoff.SetAttribute("PacketSize", StringValue("512"));
    // onoff.Install(csmaNodes.Get(nCsma));
    // ApplicationContainer apps = onoff.Install(csmaNodes.Get(nCsma));  // node 2
    // apps.Start(Seconds(5.0));
    // apps.Stop(Seconds(simulationTime));


    // Address localAddress (InetSocketAddress ("10.1.3.3", port)); //10.1.2.4
    // PacketSinkHelper packetSinkHelper (socketType, localAddress);
    // ApplicationContainer sinkApp = packetSinkHelper.Install(wifiStaNodes.Get(nWifi-1));  // node 3  // (csmaNodes.Get(nCsma));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simulationTime));


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  pointToPoint.EnablePcapAll ("pm-ex2");
  phy.EnablePcap ("pm-ex2", apDevices.Get (0));
  csma.EnablePcap ("pm-ex2", csmaDevices.Get (0), true);

  Simulator::Run ();

  // uint32_t intTotalByte = 0;
  // uint32_t intTotalBits = 0;
  // double intThroughput = 0;

  // intTotalByte = bytesTotal+bytesTotal_cbr;
  // intTotalBits = (intTotalByte * 8);

  // intThroughput = ((intTotalBits*0.000001)/(90-minStart)) ;

  //std::cout <<  "Total bit " << intTotalBits  << std::endl;

  //std::cout <<  "Throughput " << intThroughput << " Mbps" << std::endl;

  //std::cout <<  "Total : Video " << bytesTotal << " : cbr " << bytesTotal_cbr << " min : " << minStart << " Throughput : " << intThroughput << " Mbps" << std::endl;

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
   std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
   std::cout << std::endl << "*** Flow monitor statistics ***" << std::endl;
   std::cout << "  Tx Packets/Bytes:   " << stats[2].txPackets
             << " / " << stats[2].txBytes << std::endl;
   std::cout << "  Offered Load: " << stats[2].txBytes * 8.0 / (stats[2].timeLastTxPacket.GetSeconds () - stats[2].timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
   std::cout << "  Rx Packets/Bytes:   " << stats[2].rxPackets
             << " / " << stats[2].rxBytes << std::endl;
   uint32_t packetsDroppedByQueueDisc = 0;
   uint64_t bytesDroppedByQueueDisc = 0;
   if (stats[2].packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE_DISC)
     {
       packetsDroppedByQueueDisc = stats[2].packetsDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
       bytesDroppedByQueueDisc = stats[2].bytesDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
     }
   std::cout << "  Packets/Bytes Dropped by Queue Disc:   " << packetsDroppedByQueueDisc
             << " / " << bytesDroppedByQueueDisc << std::endl;
   uint32_t packetsDroppedByNetDevice = 0;
   uint64_t bytesDroppedByNetDevice = 0;
   if (stats[2].packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE)
     {
       packetsDroppedByNetDevice = stats[2].packetsDropped[Ipv4FlowProbe::DROP_QUEUE];
       bytesDroppedByNetDevice = stats[2].bytesDropped[Ipv4FlowProbe::DROP_QUEUE];
     }
   std::cout << "  Packets/Bytes Dropped by NetDevice:   " << packetsDroppedByNetDevice
             << " / " << bytesDroppedByNetDevice << std::endl;
   std::cout << "  Throughput: " << stats[2].rxBytes * 8.0 / (stats[2].timeLastRxPacket.GetSeconds () - stats[2].timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
   std::cout << "  Mean delay:   " << stats[2].delaySum.GetSeconds () / stats[2].rxPackets << std::endl;
   std::cout << "  Mean jitter:   " << stats[2].jitterSum.GetSeconds () / (stats[2].rxPackets - 1) << std::endl;
   auto dscpVec = classifier->GetDscpCounts (1);
   for (auto p : dscpVec)
     {
       std::cout << "  DSCP value:   0x" << std::hex << static_cast<uint32_t> (p.first) << std::dec
                 << "  count:   "<< p.second << std::endl;
     }

  Simulator::Destroy ();

  std::cout << std::endl << "*** Application statistics ***" << std::endl;
   double thr = 0;
   uint64_t totalPacketsThr = DynamicCast<PacketSink> (sinkApp.Get (0))->GetTotalRx ();
   thr = totalPacketsThr * 8 / (simulationTime * 1000000.0); //Mbit/s
   std::cout << "  Rx Bytes: " << totalPacketsThr << std::endl;
   std::cout << "  Average Goodput: " << thr << " Mbit/s" << std::endl;

  // if (rootQueueDisc == "Red")
  // {
     std::cout << std::endl << "*** TC Layer statistics ***" << std::endl;
     std::cout << q->GetStats () << std::endl;
  // }

  return 0;

  
}
