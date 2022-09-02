#ifndef TCP_COPA_RL_H
#define TCP_COPA_RL_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/opengym-module.h"
#include "ns3/tcp-socket-base.h"

namespace ns3 {
class TcpNewReno;
class TcpSocketBase;
class Time;
class TcpCopaEnv;

class TcpSocketCopaDerived : public TcpSocketBase
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId () const;

  TcpSocketCopaDerived (void);
  virtual ~TcpSocketCopaDerived (void);

  Ptr<TcpCongestionOps> GetCongestionControlAlgorithm ();
};

class TcpCopaRl : public TcpNewReno
{
public:
  static TypeId GetTypeId (void);

  TcpCopaRl (void);

  TcpCopaRl (const TcpCopaRl &sock);

  virtual ~TcpCopaRl (void);

  std::string GetName () const;

  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt);

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  Ptr<OpenGymDataContainer> GetObservation (void);

  void Update_Velocity_Parameter (double now, double rtt, double win);
  float  Step (Ptr<OpenGymDataContainer> action);
  Ptr<TcpCongestionOps> Fork ();

protected:
  static uint64_t GenerateUuid ();
  void ConnectSocketCallbacks ();

  // OpenGymEnv interface
  Ptr<TcpSocketBase> m_tcpSocket;
  Ptr<TcpCopaEnv> m_tcpGymEnv;

private:
  // Copa parameters

  double m_sending_rate{0.0};
  double m_expected_sending_rate{0.0};
  double m_last_expected_sending_rate{0.0};

  double m_min_rtt{1000.0};
  double m_probe_min_rtt_len{10.0}; // minRTT 在 10秒内的最小值
  double m_probe_rtt_timestamp{-1.0};

  double m_rtt_standing{1000.0};
  double m_rtt_standing_len{0.0};
  double m_probe_rtt_standing_timestamp{-1.0};

  double m_delay{0.0};
  double m_delta{0.5};
  // change to rl_velocity
  double m_velocity_parameter{1.0};

  double m_last_win{-1.0};
  double m_last_time{-1.0};
  double m_last_win_len{0.0};

  // Rl
  void CreateGymEnv ();
  // OpenGymEnv env
  float m_reward{0};
  float m_new_reward{0};
};

} // namespace ns3
#endif /* TCP_COPA_RL_H */
