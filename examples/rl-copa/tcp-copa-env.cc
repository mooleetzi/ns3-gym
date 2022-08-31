#include "tcp-copa-env.h"
#include "ns3/tcp-header.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-base.h"
#include <vector>
#include <numeric>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ns3::TcpCopaEnv");
NS_OBJECT_ENSURE_REGISTERED (TcpCopaEnv);

TcpCopaEnv::TcpCopaEnv ()
{
  NS_LOG_FUNCTION (this);
  SetOpenGymInterface (OpenGymInterface::Get ());
}

TcpCopaEnv::~TcpCopaEnv ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TcpCopaEnv::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpCopaEnv")
                          .SetParent<OpenGymEnv> ()
                          .SetGroupName ("OpenGym")
                          .AddConstructor<TcpCopaEnv> ();
  return tid;
}

void
TcpCopaEnv::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
TcpCopaEnv::SetNodeId (uint32_t id)
{
  NS_LOG_FUNCTION (this);
  m_nodeId = id;
}
void
TcpCopaEnv::SetSocketUuid (uint32_t id)
{
  NS_LOG_FUNCTION (this);
  m_socketUuid = id;
}
void
TcpCopaEnv::SetCaPtr (Ptr<TcpCopaRl> ca)
{
  NS_LOG_FUNCTION (this);
  m_ca = ca;
}

std::string
TcpCopaEnv::GetTcpCongStateName (const TcpSocketState::TcpCongState_t state)
{
  std::string stateName = "UNKNOWN";
  switch (state)
    {
    case TcpSocketState::CA_OPEN:
      stateName = "CA_OPEN";
      break;
    case TcpSocketState::CA_DISORDER:
      stateName = "CA_DISORDER";
      break;
    case TcpSocketState::CA_CWR:
      stateName = "CA_CWR";
      break;
    case TcpSocketState::CA_RECOVERY:
      stateName = "CA_RECOVERY";
      break;
    case TcpSocketState::CA_LOSS:
      stateName = "CA_LOSS";
      break;
    case TcpSocketState::CA_LAST_STATE:
      stateName = "CA_LAST_STATE";
      break;
    default:
      stateName = "UNKNOWN";
      break;
    }
  return stateName;
}

std::string
TcpCopaEnv::GetTcpCAEventName (const TcpSocketState::TcpCAEvent_t event)
{
  std::string eventName = "UNKNOWN";
  switch (event)
    {
    case TcpSocketState::CA_EVENT_TX_START:
      eventName = "CA_EVENT_TX_START";
      break;
    case TcpSocketState::CA_EVENT_CWND_RESTART:
      eventName = "CA_EVENT_CWND_RESTART";
      break;
    case TcpSocketState::CA_EVENT_COMPLETE_CWR:
      eventName = "CA_EVENT_COMPLETE_CWR";
      break;
    case TcpSocketState::CA_EVENT_LOSS:
      eventName = "CA_EVENT_LOSS";
      break;
    case TcpSocketState::CA_EVENT_ECN_NO_CE:
      eventName = "CA_EVENT_ECN_NO_CE";
      break;
    case TcpSocketState::CA_EVENT_ECN_IS_CE:
      eventName = "CA_EVENT_ECN_IS_CE";
      break;
    case TcpSocketState::CA_EVENT_DELAYED_ACK:
      eventName = "CA_EVENT_DELAYED_ACK";
      break;
    case TcpSocketState::CA_EVENT_NON_DELAYED_ACK:
      eventName = "CA_EVENT_NON_DELAYED_ACK";
      break;
    default:
      eventName = "UNKNOWN";
      break;
    }
  return eventName;
}

/*
Define action space
*/
Ptr<OpenGymSpace>
TcpCopaEnv::GetActionSpace ()
{
  // new_ssThresh
  // new_cWnd
  // new_velocity
  uint32_t parameterNum1 = 2;
  float low1 = 0.0;
  float high1 = 65535;
  std::vector<uint32_t> shape1 = {
      parameterNum1,
  };
  std::string dtype1 = TypeNameGet<uint32_t> ();

  Ptr<OpenGymBoxSpace> box1 = CreateObject<OpenGymBoxSpace> (low1, high1, shape1, dtype1);
  NS_LOG_INFO ("MyGetActionSpaceBox1: " << box1);
  uint32_t parameterNum2 = 1;
  uint32_t low2 = 1;
  uint32_t high2 = 64;
  std::vector<uint32_t> shape2 = {
      parameterNum2,
  };
  std::string dtype2 = TypeNameGet<uint32_t> ();
  Ptr<OpenGymBoxSpace> box2 = CreateObject<OpenGymBoxSpace> (low2, high2, shape2, dtype2);
  NS_LOG_INFO ("MyGetActionSpaceBox2: " << box2);
  Ptr<OpenGymDictSpace> space = CreateObject<OpenGymDictSpace> ();
  space->Add ("base", box1);
  space->Add ("vel", box2);
  return space;
}

/*
Define game over condition
*/
bool
TcpCopaEnv::GetGameOver ()
{
  m_isGameOver = false;
  bool test = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  if (stepCounter == 1000 && test)
    { //TODO check here
      m_isGameOver = true;
    }
  NS_LOG_INFO ("MyGetGameOver: " << m_isGameOver);
  return m_isGameOver;
}

/*
Define reward function
*/
float
TcpCopaEnv::GetReward ()
{
  NS_LOG_INFO ("MyGetReward: " << m_envReward);
  return m_envReward; //TODO check here
}
/*
Define extra info. Optional
*/
std::string
TcpCopaEnv::GetExtraInfo ()
{
  NS_LOG_INFO ("MyGetExtraInfo: " << m_info);
  return m_info;
}

/*
Execute received actions
*/
bool
TcpCopaEnv::ExecuteActions (Ptr<OpenGymDataContainer> action)
{

  Ptr<OpenGymDictContainer> dict = DynamicCast<OpenGymDictContainer> (action);
  Ptr<OpenGymBoxContainer<uint32_t>> base =
      DynamicCast<OpenGymBoxContainer<uint32_t>> (dict->Get ("base"));
  Ptr<OpenGymBoxContainer<uint32_t>> vel =
      DynamicCast<OpenGymBoxContainer<uint32_t>> (dict->Get ("vel"));
  m_new_ssThresh = base->GetValue (0);
  m_new_cWnd = base->GetValue (1);
  m_new_velocity_parameter = vel->GetValue (0);
  m_envReward = m_ca->Step (action);
  // Config::Set("/NodeList"+std::to_string(m_nodeId)+"/ApplicationList/"+std::to_string(m_appId)+"/Socket/SetSsThresh", UintegerValue(m_new_ssThresh));
  NS_LOG_INFO ("MyExecuteActions: " << action);
  return true;
}

/*
Define observation space
*/
Ptr<OpenGymSpace>
TcpCopaEnv::GetObservationSpace ()
{
  // socket unique ID
  // tcp env type: event-based = 0 / time-based = 1
  // sim time in us
  // node ID
  // ssThresh
  // cWnd
  // segmentSize
  // segmentsAcked
  // bytesInFlight
  // rtt in us
  // min rtt in us
  // called func
  // congetsion algorithm (CA) state
  // CA event
  // ECN state
  uint32_t parameterNum = 15;
  float low = 0.0;
  float high = 1000000000.0;
  std::vector<uint32_t> shape = {
      parameterNum,
  };
  std::string dtype = TypeNameGet<uint64_t> ();

  Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  NS_LOG_INFO ("MyGetObservationSpace: " << box);
  return box;
}

/*
Collect observations
*/
Ptr<OpenGymDataContainer>
TcpCopaEnv::GetObservation ()
{
  uint32_t parameterNum = 15;
  std::vector<uint32_t> shape = {
      parameterNum,
  };

  Ptr<OpenGymBoxContainer<uint64_t>> box = CreateObject<OpenGymBoxContainer<uint64_t>> (shape);

  box->AddValue (m_socketUuid);
  box->AddValue (0);
  box->AddValue (Simulator::Now ().GetMicroSeconds ());
  box->AddValue (m_nodeId);
  box->AddValue (m_tcb->m_ssThresh);
  box->AddValue (m_tcb->m_cWnd);
  box->AddValue (m_tcb->m_segmentSize);
  box->AddValue (m_segmentsAcked);
  box->AddValue (m_bytesInFlight);
  box->AddValue (m_rtt.GetMicroSeconds ());
  box->AddValue (m_tcb->m_minRtt.GetMicroSeconds ());
  box->AddValue (m_calledFunc);
  box->AddValue (m_tcb->m_congState);
  box->AddValue (m_event);
  box->AddValue (m_tcb->m_ecnState);

  // Print data
  NS_LOG_INFO ("MyGetObservation: " << box);
  return box;
}

void
TcpCopaEnv::TxPktTrace (Ptr<const Packet>, const TcpHeader &, Ptr<const TcpSocketBase>)
{
  NS_LOG_FUNCTION (this);
}

void
TcpCopaEnv::RxPktTrace (Ptr<const Packet>, const TcpHeader &, Ptr<const TcpSocketBase>)
{
  NS_LOG_FUNCTION (this);
}

uint32_t
TcpCopaEnv::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this);
  // pkt was lost, so penalty
  // m_envReward = m_penalty;

  NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId
                                 << " GetSsThresh, BytesInFlight: " << bytesInFlight);
  m_calledFunc = CalledFunc_t::GET_SS_THRESH;
  m_info = "GetSsThresh";
  m_tcb = tcb;
  m_bytesInFlight = bytesInFlight;
  Notify ();
  return m_new_ssThresh;
}

void
TcpCopaEnv::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this);
  // pkt was acked, so reward
  // m_envReward = m_reward;

  NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId
                                 << " IncreaseWindow, SegmentsAcked: " << segmentsAcked);
  m_calledFunc = CalledFunc_t::INCREASE_WINDOW;
  m_info = "IncreaseWindow";
  m_tcb = tcb;
  m_segmentsAcked = segmentsAcked;
  Notify ();
}

void
TcpCopaEnv::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId << " PktsAcked, SegmentsAcked: "
                                 << segmentsAcked << " Rtt: " << rtt);
  m_calledFunc = CalledFunc_t::PKTS_ACKED;
  m_info = "PktsAcked";
  m_tcb = tcb;
  m_segmentsAcked = segmentsAcked;
  m_rtt = rtt;
}

void
TcpCopaEnv::CongestionStateSet (Ptr<TcpSocketState> tcb,
                                const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this);
  std::string stateName = GetTcpCongStateName (newState);
  NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId << " CongestionStateSet: " << newState
                                 << " " << stateName);

  m_calledFunc = CalledFunc_t::CONGESTION_STATE_SET;
  m_info = "CongestionStateSet";
  m_tcb = tcb;
  m_newState = newState;
}
void
TcpCopaEnv::CwndEvent (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
{
  NS_LOG_FUNCTION (this);
  std::string eventName = GetTcpCAEventName (event);
  NS_LOG_INFO (Simulator::Now () << " Node: " << m_nodeId << " CwndEvent: " << event << " "
                                 << eventName);

  m_calledFunc = CalledFunc_t::CWND_EVENT;
  m_info = "CwndEvent";
  m_tcb = tcb;
  m_event = event;
}

} // namespace ns3