#ifndef ADAPTIVE_A2A4_RSRQ_HANDOVER_ALGORITHM_H
#define ADAPTIVE_A2A4_RSRQ_HANDOVER_ALGORITHM_H

#include "ns3/a2-a4-rsrq-handover-algorithm.h"
#include "ns3/node-container.h"
#include "ns3/log.h"
#include <map>

namespace ns3 {

class AdaptiveA2A4RsrqHandoverAlgorithm : public A2A4RsrqHandoverAlgorithm
{
public:
  static TypeId GetTypeId (void);
  AdaptiveA2A4RsrqHandoverAlgorithm ();
  virtual ~AdaptiveA2A4RsrqHandoverAlgorithm ();

  // Переопределяем базовый метод для выдачи порога
  virtual uint16_t GetServingCellThreshold (uint16_t cellId) const;

  // Вызывается извне по расписанию для пересчёта порогов
  void UpdateThresholds (const NodeContainer& ueNodes);

private:
  uint32_t m_loadThreshold;        // число UE, при котором порог меняется
  uint8_t  m_defaultThreshold;     // используется, если для соты нет записи
  uint8_t  m_lowLoadThreshold;     // порог при числе UE <= m_loadThreshold
  uint8_t  m_highLoadThreshold;    // порог при числе UE > m_loadThreshold

  std::map<uint16_t, uint8_t> m_thresholdPerCell;
};

} // namespace ns3

#endif /* ADAPTIVE_A2A4_RSRQ_HANDOVER_ALGORITHM_H */



