#ifndef TCP_COPA_ENV_H
#define TCP_COPA_ENV_H

#include "ns3/opengym-module.h"
#include "tcp-copa-rl.h"
#include "ns3/tcp-socket-base.h"
#include <vector>

namespace ns3 {

class Packet;
class TcpHeader;
class TcpSocketBase;
class Time;
class TcpCopaRl;

class TcpCopaEnv : public OpenGymEnv
{
public:
  TcpCopaEnv ();
  virtual ~TcpCopaEnv ();
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  void SetNodeId (uint32_t id);
  void SetSocketUuid (uint32_t id);

  std::string GetTcpCongStateName (const TcpSocketState::TcpCongState_t state);
  std::string GetTcpCAEventName (const TcpSocketState::TcpCAEvent_t event);

  // OpenGym interface
  virtual Ptr<OpenGymSpace> GetActionSpace ();
  virtual bool GetGameOver ();
  virtual float GetReward ();
  virtual std::string GetExtraInfo ();
  virtual bool ExecuteActions (Ptr<OpenGymDataContainer> action);

  virtual Ptr<OpenGymSpace> GetObservationSpace ();
  virtual Ptr<OpenGymDataContainer> GetObservation ();

  // trace packets, e.g. for calculating inter tx/rx time
  virtual void TxPktTrace (Ptr<const Packet>, const TcpHeader &, Ptr<const TcpSocketBase>);
  virtual void RxPktTrace (Ptr<const Packet>, const TcpHeader &, Ptr<const TcpSocketBase>);

  // TCP congestion control interface
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  // optional functions used to collect obs
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt);
  virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);
  virtual void CwndEvent (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event);

  virtual void SetCaPtr (Ptr<TcpCopaRl> ca);

  typedef enum {
    GET_SS_THRESH = 0,
    INCREASE_WINDOW,
    PKTS_ACKED,
    CONGESTION_STATE_SET,
    CWND_EVENT,
  } CalledFunc_t;

protected:
  uint32_t m_nodeId;
  uint32_t m_socketUuid;

  // state
  // obs has to be implemented in child class

  // game over
  bool m_isGameOver;

  // extra info
  std::string m_info;

  // actions
  uint32_t m_new_ssThresh;
  uint32_t m_new_cWnd;
  uint32_t m_new_velocity_parameter;

private:
  // state
  CalledFunc_t m_calledFunc;
  Ptr<const TcpSocketState> m_tcb;
  uint32_t m_bytesInFlight;
  uint32_t m_segmentsAcked;
  Time m_rtt;
  TcpSocketState::TcpCongState_t m_newState;
  TcpSocketState::TcpCAEvent_t m_event;
  uint32_t m_velocity_parameter;

  // reward
  float m_envReward;
  Ptr<TcpCopaRl> m_ca;
};

} // namespace ns3
#endif /* TCP_COPA_ENV_H */