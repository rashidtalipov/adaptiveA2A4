#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include <iostream>
#include <vector>
#include <stdio.h>
#include <iomanip>

using namespace ns3;

void LogUesAndAdaptThreshold(Ptr<LteHelper> lteHelper, NodeContainer enbNodes, NodeContainer ueNodes, std::string filename)
{
  uint16_t numberOfEnbs = enbNodes.GetN();

  std::vector<uint32_t> enbConnectionCount(numberOfEnbs, 0);

  // Подсчёт пользователей на каждой соте
  for (uint16_t i = 0; i < ueNodes.GetN(); ++i)
  {
    Ptr<NetDevice> ueDevice = ueNodes.Get(i)->GetDevice(0);
    Ptr<LteUeNetDevice> lteUeDev = DynamicCast<LteUeNetDevice>(ueDevice);
    uint16_t cellId = lteUeDev->GetRrc()->GetCellId();

    if (cellId >= 1 && cellId <= numberOfEnbs)
    {
      enbConnectionCount[cellId - 1]++;
    }
  }

  std::ofstream outFile;
  outFile.open(filename, std::ios::app);
  outFile << "Time: " << Simulator::Now().GetSeconds() << "s\n";

  // Адаптивное изменение ServingCellThreshold
  for (uint16_t enb = 0; enb < numberOfEnbs; ++enb)
  {
    uint32_t count = enbConnectionCount[enb];
    outFile << "eNodeB " << enb << " (CellId " << enb + 1 << ") has " << count << " connected UEs\n";

    // Получаем RRC
    Ptr<Node> enbNode = enbNodes.Get(enb);  // вот тут берём узел из контейнера enbNodes
    Ptr<NetDevice> enbDev = enbNode->GetDevice(0);
    Ptr<LteEnbNetDevice> lteEnbDev = DynamicCast<LteEnbNetDevice>(enbDev);
    Ptr<LteEnbRrc> rrc = lteEnbDev->GetRrc();

    // Меняем порог в зависимости от нагрузки
    uint32_t newThreshold = (count > 6) ? 15 : 30;
    rrc->SetAttribute("ServingCellThreshold", UintegerValue(newThreshold));
  }

  outFile << "------------------------\n";
  outFile.close();
}


void PeriodicThresholdUpdate(Time interval, Ptr<LteHelper> lteHelper, NodeContainer enbNodes, NodeContainer ueNodes, std::string filename)
{
  LogUesAndAdaptThreshold(lteHelper, enbNodes, ueNodes, filename);
  Simulator::Schedule(interval, &PeriodicThresholdUpdate, interval, lteHelper, enbNodes, ueNodes, filename);
}





#include <fstream>

void LogUesPerEnbToFile(Ptr<LteHelper> lteHelper, NodeContainer ueNodes, uint16_t numberOfEnbs, std::string filename)
{
  std::vector<uint32_t> enbConnectionCount(numberOfEnbs, 0);

  for (uint16_t i = 0; i < ueNodes.GetN(); ++i)
    {
      Ptr<NetDevice> ueDevice = ueNodes.Get(i)->GetDevice(0);
      Ptr<LteUeNetDevice> lteUeDev = DynamicCast<LteUeNetDevice>(ueDevice);
      uint16_t cellId = lteUeDev->GetRrc()->GetCellId();

      if (cellId >= 1 && cellId <= numberOfEnbs)
        {
          enbConnectionCount[cellId - 1]++;
        }
    }

  std::ofstream outFile;
  outFile.open(filename, std::ios::app); // откроем файл в режиме добавления

  outFile << "Time: " << Simulator::Now().GetSeconds() << "s\n";
  for (uint16_t enb = 0; enb < numberOfEnbs; ++enb)
    {
      outFile << "eNodeB " << enb << " (CellId " << enb + 1
              << ") has " << enbConnectionCount[enb] << " connected UEs\n";
    }
  outFile << "------------------------\n";

  outFile.close();
}

void PeriodicLogging(Time interval, Ptr<LteHelper> lteHelper, NodeContainer ueNodes, uint16_t numberOfEnbs, std::string filename)
{
  LogUesPerEnbToFile(lteHelper, ueNodes, numberOfEnbs, filename);
  Simulator::Schedule(interval, &PeriodicLogging, interval, lteHelper, ueNodes, numberOfEnbs, filename);
}





// Начальный счётчик размера пакетов
uint32_t ByteCounter = 0;
uint32_t oldByteCounter = 0;

// Функция подсчёта размера пакетов
void
ReceivePacket (Ptr<const Packet> packet, const Address &)
{
  ByteCounter += packet->GetSize ();
}

// Функция подсчёта пропускной способности
void
Throughput(bool firstWrite, Time binSize, std::string fileName)
{
  std::ofstream output;

  if (firstWrite == true)
    {
      output.open (fileName.c_str (), std::ofstream::out);
      firstWrite = false;
    }
  else
    {
      output.open (fileName.c_str (), std::ofstream::app);
    }

  // Подсчёт каждые 200 мс
  double  throughput = (ByteCounter - oldByteCounter)*8/binSize.GetSeconds ()/1024/1024;
  output << Simulator::Now().GetSeconds() << " " << throughput << std::endl;
  oldByteCounter = ByteCounter;
  Simulator::Schedule (binSize, &Throughput, firstWrite, binSize, fileName);
}

int
main (int argc, char *argv[])
{
  // Параметры
  uint16_t numberOfEnbs = 7;
  double interSiteDistance = 800;
  double speed = 60/3.6; // m/s
  double simTime = 60; // 1500 m / 20 m/s = 75 secs
  bool useIdealRrc = false;
  bool enableCtrlErrorModel = false;
  bool enableDataErrorModel = false;
  bool enableNsLogs = true;

  // Опции командной строки в данной модели не используются. Необходимо для работы Визуализатора
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Компоненты логирования, включаются только если enableNsLogs = true
  if (enableNsLogs)
    {
      LogLevel logLevel = (LogLevel) (LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_ALL);
      LogComponentEnable ("LteUeRrc", logLevel);
      // LogComponentEnable ("LteUeMac", logLevel);
      // LogComponentEnable ("LteUePhy", logLevel);

      LogComponentEnable ("LteEnbRrc", logLevel);
      // LogComponentEnable ("LteEnbMac", logLevel);
      // LogComponentEnable ("LteEnbPhy", logLevel);

      // LogComponentEnable ("LteHelper", logLevel);
      // LogComponentEnable ("EpcHelper", logLevel);
      // LogComponentEnable ("EpcEnbApplication", logLevel);
      // LogComponentEnable ("EpcMmeApplication", logLevel);
      // LogComponentEnable ("EpcPgwApplication", logLevel);
      // LogComponentEnable ("EpcSgwApplication", logLevel);
      // LogComponentEnable ("EpcX2", logLevel);

      // LogComponentEnable ("LteEnbNetDevice", logLevel);
      // LogComponentEnable ("LteUeNetDevice", logLevel);
      // LogComponentEnable ("A3RsrpHandoverAlgorithm", logLevel);
    }

  // Количество пользователей и мощность сигнала от вышек
  uint16_t numberOfUes = 100;
  uint16_t numBearersPerUe = 1;
  double eNodeB_txPower = 43; // dBm

  // Задание идеальной модели канала и моделей ошибок (конфигурируется в секции "Параметры")
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (useIdealRrc));
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (enableCtrlErrorModel));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (enableDataErrorModel));

  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (60 * 1024));

  // Создание помощника LTE 
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  // Модель распространения 
  lteHelper->SetPathlossModelType (TypeId::LookupByName ("ns3::LogDistancePropagationLossModel")); //JakesPropagationLossModel
// Мощности
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (eNodeB_txPower));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23)); // dBm
  Config::SetDefault ("ns3::LteUePhy::NoiseFigure", DoubleValue (7)); 
  Config::SetDefault ("ns3::LteEnbPhy::NoiseFigure", DoubleValue (2));
  Config::SetDefault ("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue (true));
  Config::SetDefault ("ns3::LteUePowerControl::ClosedLoop", BooleanValue (true));
  Config::SetDefault ("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue (true));

  // Частоты
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (100)); //2120MHz
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (18100)); //1930MHz
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100)); //5MHz
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (100)); //5MHz
  
  lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
  lteHelper->SetFadingModelAttribute("TraceFilename", StringValue("src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad"));
  lteHelper->SetFadingModelAttribute("TraceLength", TimeValue(Seconds(10.0)));
  lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(10000));
  lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
  lteHelper->SetFadingModelAttribute("RbNum", UintegerValue(100));

  // Алгоритм хендовера
  // lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
  // lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis", DoubleValue (3.0));
  // lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger", TimeValue (MilliSeconds (256)));
   lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
   lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold", UintegerValue (30));
   lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset", UintegerValue (1));

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Создание удалённого узла
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (numberOfEnbs);
  ueNodes.Create (numberOfUes);
  
    Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator>();
    positionAllocEnb->Add(Vector(0, 0, 35)); // Центральная сота
    positionAllocEnb->Add(Vector(1000, 0, 35)); // Сота справа
    positionAllocEnb->Add(Vector(-1000, 0, 35)); // Сота слева
    positionAllocEnb->Add(Vector(500, 866, 35)); // Верх справа
    positionAllocEnb->Add(Vector(-500, 866, 35)); // Верх слева
    positionAllocEnb->Add(Vector(500, -866, 35)); // Низ справа
    positionAllocEnb->Add(Vector(-500, -866, 35)); // Низ слева

    MobilityHelper txMob;
    txMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    txMob.SetPositionAllocator(positionAllocEnb);
    txMob.Install(enbNodes);

  // Расположение и модель движения пользователей
  Ptr<ListPositionAllocator> positionAllocUe = CreateObject<ListPositionAllocator> ();
  
    MobilityHelper rxMob;
    Ptr<PositionAllocator> positionAlloc2 = CreateObject<RandomBoxPositionAllocator>();
    positionAlloc2->SetAttribute("X", StringValue("ns3::UniformRandomVariable[Min=-1500.0|Max=1500.0]"));
    positionAlloc2->SetAttribute("Y", StringValue("ns3::UniformRandomVariable[Min=-1500.0|Max=1500.0]"));
    positionAlloc2->SetAttribute("Z", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    rxMob.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                           "Bounds", RectangleValue(Rectangle(-1500, 1500, -1500, 1500)),
                           "Distance", DoubleValue(50.0),
                           "Speed", StringValue("ns3::ConstantRandomVariable[Constant=37]"));
    rxMob.SetPositionAllocator(positionAlloc2);
    rxMob.Install(ueNodes);

  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;

  int64_t randomStream = 1;

  // Установка стека протоколов LTE на базовые станции и оборудование пользователей
  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  randomStream += lteHelper->AssignStreams (enbDevs, randomStream);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);
  randomStream += lteHelper->AssignStreams (ueDevs, randomStream);

  // Установка интернета
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  // Изначальное подключение пользователей к ближайшей станции
  lteHelper->Attach (ueDevs);

  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  DataRateValue dataRateValue = DataRate ("18.6Mbps"); // 18.6Mbps

  uint64_t bitRate = dataRateValue.Get ().GetBitRate ();

  uint32_t packetSize = 1024; //bytes

  double interPacketInterval = static_cast<double> (packetSize * 8) / bitRate;

  Time udpInterval = Seconds (interPacketInterval);

  // Генерация потока пакетов от (удалённый узел ->) базовых станций к пользователям
  for (uint32_t u = 0; u < numberOfUes; ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      // Задание шлюза по умолчанию для LTE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
          ApplicationContainer ulClientApps;
          ApplicationContainer ulServerApps;
          ApplicationContainer dlClientApps;
          ApplicationContainer dlServerApps;

          ++dlPort;
          ++ulPort;

          UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
          dlClientHelper.SetAttribute ("Interval", TimeValue (udpInterval));
          dlClientHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
          dlClientHelper.SetAttribute ("MaxPackets", UintegerValue (1000000));
          dlClientApps.Add (dlClientHelper.Install (remoteHost));

          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          dlServerApps.Add (dlPacketSinkHelper.Install (ue));

          UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          ulClientHelper.SetAttribute ("Interval", TimeValue (udpInterval));
          dlClientHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
          ulClientHelper.SetAttribute ("MaxPackets", UintegerValue (1000000));
          ulClientApps.Add (ulClientHelper.Install (ue));

          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          ulServerApps.Add (ulPacketSinkHelper.Install (remoteHost));

          Ptr<EpcTft> tft = Create<EpcTft> ();
          EpcTft::PacketFilter dlpf;
          dlpf.localPortStart = dlPort;
          dlpf.localPortEnd = dlPort;
          tft->Add (dlpf);
          EpcTft::PacketFilter ulpf;
          ulpf.remotePortStart = ulPort;
          ulpf.remotePortEnd = ulPort;
          tft->Add (ulpf);
          EpsBearer bearer (EpsBearer::NGBR_IMS);
          lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), bearer, tft);

          dlServerApps.Start (Seconds (0.27));
          dlClientApps.Start (Seconds (0.27));
          ulServerApps.Start (Seconds (0.27));
          ulClientApps.Start (Seconds (0.27));
        } 
    }

  // Добавление интерфейса X2 между базовыми станциями (без него хендовер не будет работать)
  lteHelper->AddX2Interface (enbNodes);
  // Включение трейсов (логирование KPI в файл) и задание интервала снятия показаний
  lteHelper->EnableTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.05)));
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.05)));

  // Отслеживание приёма пакетов пользователями
  std::ostringstream oss;
  oss << "/NodeList/" << ueNodes.Get (0)->GetId () << "/ApplicationList/0/$ns3::PacketSink/Rx";
  Config::ConnectWithoutContext (oss.str (), MakeCallback (&ReceivePacket));

  // Логирование пропускной способности в файл
  bool firstWrite = true;
  std::string rrcType = useIdealRrc == 1 ? "ideal_rrc":"real_rrc";
  std::string fileName = "rlf_dl_thrput_" + std::to_string (enbNodes.GetN ()) + "_eNB_" + rrcType;
  Time binSize = Seconds (0.1);
  Simulator::Schedule (Seconds(0.47), &Throughput, firstWrite, binSize, fileName);
  
  
  std::string logFile1 = "ue_per_enb_log.txt";

  // Очистим файл перед началом симуляции
  std::ofstream clearFile1(logFile1, std::ios::out);
  clearFile1.close();

  Simulator::Schedule(Seconds(0.1), &PeriodicLogging, Seconds(0.1), lteHelper, ueNodes, numberOfEnbs, logFile1);
  
  
  std::string logFile = "threshold_update_log.txt";
  std::ofstream clearFile(logFile, std::ios::out);  // очистка
  clearFile.close();

  Simulator::Schedule(Seconds(1.0), &PeriodicThresholdUpdate, Seconds(1.0), lteHelper, enbNodes, ueNodes, logFile);



  Simulator::Stop (Seconds (simTime));

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
