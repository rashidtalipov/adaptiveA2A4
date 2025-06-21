#include "adaptive-a2-a4-rsrq-handover-algorithm.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/lte-ue-rrc.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AdaptiveA2A4RsrqHandoverAlgorithm");
NS_OBJECT_ENSURE_REGISTERED (AdaptiveA2A4RsrqHandoverAlgorithm);

TypeId
AdaptiveA2A4RsrqHandoverAlgorithm::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AdaptiveA2A4RsrqHandoverAlgorithm")
    .SetParent<A2A4RsrqHandoverAlgorithm> ()
    .SetGroupName ("Lte")
    .AddConstructor<AdaptiveA2A4RsrqHandoverAlgorithm> ()

    // число UE, задающее порог перегрузки
    .AddAttribute ("LoadThresholdUes",
                   "Number of UEs above which the threshold is lowered",
                   UintegerValue (13),
                   MakeUintegerAccessor (&AdaptiveA2A4RsrqHandoverAlgorithm::m_loadThreshold),
                   MakeUintegerChecker<uint32_t> ())

    // порог, когда UE <= LoadThresholdUes
    .AddAttribute ("LowLoadThreshold",
                   "RSRQ threshold when load is low",
                   UintegerValue (30),
                   MakeUintegerAccessor (&AdaptiveA2A4RsrqHandoverAlgorithm::m_lowLoadThreshold),
                   MakeUintegerChecker<uint8_t> ())

    // порог, когда UE > LoadThresholdUes
    .AddAttribute ("HighLoadThreshold",
                   "RSRQ threshold when load is high",
                   UintegerValue (20),
                   MakeUintegerAccessor (&AdaptiveA2A4RsrqHandoverAlgorithm::m_highLoadThreshold),
                   MakeUintegerChecker<uint8_t> ());
  return tid;
}

AdaptiveA2A4RsrqHandoverAlgorithm::AdaptiveA2A4RsrqHandoverAlgorithm ()
  : m_loadThreshold (13),
    m_defaultThreshold (30),
    m_lowLoadThreshold (30),
    m_highLoadThreshold (20)
{
}

AdaptiveA2A4RsrqHandoverAlgorithm::~AdaptiveA2A4RsrqHandoverAlgorithm ()
{
}

uint16_t
AdaptiveA2A4RsrqHandoverAlgorithm::GetServingCellThreshold (uint16_t cellId) const
{
  auto it = m_thresholdPerCell.find (cellId);
  if (it != m_thresholdPerCell.end ())
    {
      return it->second;
    }
  return m_defaultThreshold;
}

void
AdaptiveA2A4RsrqHandoverAlgorithm::UpdateThresholds (const NodeContainer& ueNodes)
{
  std::map<uint16_t, uint32_t> ueCountPerCell;

  // Считаем число UE в каждой соте
  for (uint32_t i = 0; i < ueNodes.GetN (); ++i)
    {
      Ptr<NetDevice> nd = ueNodes.Get (i)->GetDevice (0);
      Ptr<LteUeNetDevice> ue = DynamicCast<LteUeNetDevice> (nd);
      if (!ue || !ue->GetRrc ())
        {
          continue;
        }
      uint16_t cid = ue->GetRrc ()->GetCellId ();
      ueCountPerCell[cid]++;
    }

  // Пересчитываем порог для каждой соты
  for (auto& kv : ueCountPerCell)
    {
      uint16_t cellId = kv.first;
      uint32_t count  = kv.second;
      uint8_t  thr    = (count > m_loadThreshold
                         ? m_highLoadThreshold
                         : m_lowLoadThreshold);
      m_thresholdPerCell[cellId] = thr;

      NS_LOG_INFO ("Time=" << Simulator::Now ().GetSeconds ()
                   << "s CellId=" << cellId
                   << " UEs="    << count
                   << " NewThreshold=" << +thr);
    }
}

} // namespace ns3



