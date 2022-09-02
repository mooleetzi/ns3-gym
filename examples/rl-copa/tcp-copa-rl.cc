#include "tcp-copa-rl.h"
#include "tcp-copa-env.h"
#include "ns3/tcp-header.h"
#include "ns3/object.h"
#include "ns3/node-list.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-l4-protocol.h"

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (TcpSocketCopaDerived);
TypeId
TcpSocketCopaDerived::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketCopaDerived")
                          .SetParent<TcpSocketBase> ()
                          .SetGroupName ("Internet")
                          .AddConstructor<TcpSocketCopaDerived> ();
  return tid;
}
TypeId
TcpSocketCopaDerived::GetInstanceTypeId () const
{
  return TcpSocketCopaDerived::GetTypeId ();
}
TcpSocketCopaDerived::TcpSocketCopaDerived (void)
{
}
Ptr<TcpCongestionOps>
TcpSocketCopaDerived::GetCongestionControlAlgorithm ()
{
  return m_congestionControl;
}
TcpSocketCopaDerived::~TcpSocketCopaDerived (void)
{
}

NS_LOG_COMPONENT_DEFINE ("ns3::TcpCopaRl");
NS_OBJECT_ENSURE_REGISTERED (TcpCopaRl);

TypeId
TcpCopaRl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpCopaRl")
                          .SetParent<TcpNewReno> ()
                          .SetGroupName ("Internet")
                          .AddConstructor<TcpCopaRl> ()
                          .AddAttribute ("velocity", "The velocity", UintegerValue (1),
                                         MakeUintegerAccessor (&TcpCopaRl::m_velocity_parameter),
                                         MakeUintegerChecker<uint32_t> ());
  return tid;
}

TcpCopaRl::TcpCopaRl (void) : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
  m_tcpSocket = 0;
  m_tcpGymEnv = 0;
}

TcpCopaRl::TcpCopaRl (const TcpCopaRl &sock) : TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
  m_tcpSocket = 0;
  m_tcpGymEnv = 0;
}

TcpCopaRl::~TcpCopaRl (void)
{
  m_tcpSocket = 0;
  m_tcpGymEnv = 0;
}

uint64_t
TcpCopaRl::GenerateUuid ()
{
  static uint64_t uuid = 0;
  uuid++;
  return uuid;
}

void
TcpCopaRl::ConnectSocketCallbacks ()
{
  NS_LOG_FUNCTION (this);

  bool foundSocket = false;
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<TcpL4Protocol> tcp = node->GetObject<TcpL4Protocol> ();

      ObjectVectorValue socketVec;
      tcp->GetAttribute ("SocketList", socketVec);
      NS_LOG_DEBUG ("Node: " << node->GetId () << " TCP socket num: " << socketVec.GetN ());

      uint32_t sockNum = socketVec.GetN ();
      for (uint32_t j = 0; j < sockNum; j++)
        {
          Ptr<Object> sockObj = socketVec.Get (j);
          Ptr<TcpSocketBase> tcpSocket = DynamicCast<TcpSocketBase> (sockObj);
          NS_LOG_DEBUG ("Node: " << node->GetId () << " TCP Socket: " << tcpSocket);
          if (!tcpSocket)
            {
              continue;
            }

          Ptr<TcpSocketCopaDerived> dtcpSocket = StaticCast<TcpSocketCopaDerived> (tcpSocket);
          Ptr<TcpCongestionOps> ca = dtcpSocket->GetCongestionControlAlgorithm ();
          NS_LOG_DEBUG ("CA name: " << ca->GetName ());
          Ptr<TcpCopaRl> rlCa = DynamicCast<TcpCopaRl> (ca);
          if (rlCa == this)
            {
              NS_LOG_DEBUG ("Found TcpRl CA!");
              foundSocket = true;
              m_tcpSocket = tcpSocket;
              break;
            }
        }

      if (foundSocket)
        {
          break;
        }
    }

  NS_ASSERT_MSG (m_tcpSocket, "TCP socket was not found.");

  if (m_tcpSocket)
    {
      NS_LOG_DEBUG ("Found TCP Socket: " << m_tcpSocket);
      m_tcpSocket->TraceConnectWithoutContext ("Tx",
                                               MakeCallback (&TcpCopaEnv::TxPktTrace, m_tcpGymEnv));
      m_tcpSocket->TraceConnectWithoutContext ("Rx",
                                               MakeCallback (&TcpCopaEnv::RxPktTrace, m_tcpGymEnv));
      NS_LOG_DEBUG ("Connect socket callbacks " << m_tcpSocket->GetNode ()->GetId ());
      m_tcpGymEnv->SetNodeId (m_tcpSocket->GetNode ()->GetId ());
    }
}

std::string
TcpCopaRl::GetName () const
{
  return "TcpCopaRl";
}

void
TcpCopaRl::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (!m_tcpGymEnv)
    {
      CreateGymEnv ();
    }

  if (m_tcpGymEnv)
    {
      m_tcpGymEnv->IncreaseWindow (tcb, segmentsAcked);
    }
}

void
TcpCopaRl::Update_Velocity_Parameter (double now, double rtt, double win)
{
  if (m_last_time == -1)
    {
      m_last_time = now;
      m_last_win = win;
      m_last_win_len = 3 * rtt;
      // m_velocity_parameter = 1.0;
    }
  else if (now >= (m_last_win_len + m_last_time))
    {
      // m_velocity_parameter = (m_last_win <= win) ? m_velocity_parameter * 2 : 1.0;

      m_last_time = now;
      m_last_win = win;
      m_last_win_len = 3 * rtt;
    }

  // m_velocity_parameter = std::min (m_velocity_parameter, 32.0);
}

void
TcpCopaRl::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
  if (!m_tcpGymEnv)
    {
      CreateGymEnv ();
    }
  double rtt_dec = 0; //for reward fn
  double cwnd_dec = 0; //for reward fn
  if (tcb->m_lastRtt.Get () != Time (0))
    {
      rtt_dec = rtt.GetSeconds () - tcb->m_lastRtt.Get ().GetSeconds ();
    }

  Time now = Simulator::Now ();
  // 更新 最小RTT
  if ((m_probe_rtt_timestamp == -1) ||
      (now.GetSeconds () >= (m_probe_rtt_timestamp + m_probe_min_rtt_len)) ||
      (rtt.GetSeconds () <= m_min_rtt &&
       (now.GetSeconds () < (m_probe_rtt_timestamp + m_probe_min_rtt_len))))
    {
      m_min_rtt = rtt.GetSeconds ();
      m_probe_rtt_timestamp = Simulator::Now ().GetSeconds ();
    }
  // 更新 rtt_standing
  if ((m_probe_rtt_standing_timestamp == -1) ||
      (now.GetSeconds () >= (m_probe_rtt_standing_timestamp + m_rtt_standing_len)) ||
      (rtt.GetSeconds () <= m_rtt_standing &&
       (now.GetSeconds () < (m_probe_rtt_standing_timestamp + m_rtt_standing_len))))
    {
      m_rtt_standing = rtt.GetSeconds ();
      m_rtt_standing_len = 0.5 * rtt.GetSeconds ();
      m_probe_rtt_standing_timestamp = Simulator::Now ().GetSeconds ();
    }
  // RTTstanding - minRTT
  m_delay = m_rtt_standing - m_min_rtt;
  m_delay = std::max (0.0, m_delay);

  m_sending_rate = (double) tcb->m_cWnd / m_rtt_standing;

  if (m_delay)
    {
      m_expected_sending_rate = 1.0 / (m_delta * m_delay) * 1500;
      m_last_expected_sending_rate = m_expected_sending_rate;
    }
  else
    {
      m_expected_sending_rate = m_last_expected_sending_rate;
    }

  Update_Velocity_Parameter (Simulator::Now ().GetSeconds (), rtt.GetSeconds (),
                             (double) tcb->m_cWnd);

  // 调整拥塞窗口 在 一个RTT 改变 v / delta 的 分组
  if (m_sending_rate <= m_expected_sending_rate)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize *
                                          m_velocity_parameter / m_delta) /
                     tcb->m_cWnd.Get ();
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);
      cwnd_dec = -adder;
    }
  else
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize *
                                          m_velocity_parameter / m_delta) /
                     tcb->m_cWnd.Get ();
      adder = std::max (1.0, adder);
      if (tcb->m_cWnd <= static_cast<uint32_t> (10 * tcb->m_segmentSize))
        {
          tcb->m_cWnd = static_cast<uint32_t> (10 * tcb->m_segmentSize);
          cwnd_dec = 0;
        }
      else
        {
          tcb->m_cWnd -= static_cast<uint32_t> (adder);
          cwnd_dec = adder;
        }
    }

  std::cout << "Time\t" << Simulator::Now ().GetSeconds () << "\tCongestionWindow\t" << tcb->m_cWnd
            << "\trtt\t" << rtt.GetSeconds () << "\tsengdingRate\t" << m_sending_rate
            << "\texpectedrate\t" << m_expected_sending_rate << "\tdelay\t" << m_delay
            << "\trttstarnding\t" << m_rtt_standing << "\tminrttt" << m_min_rtt << "\tparam\t"
            << m_velocity_parameter << std::endl;
  m_new_reward += 0.1 * rtt_dec - 10 * cwnd_dec + 0.01 * m_sending_rate;
  if (m_tcpGymEnv)
    {
      m_tcpGymEnv->PktsAcked (tcb, segmentsAcked, rtt);
    }
}
float
TcpCopaRl::Step (Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymDictContainer> dict = DynamicCast<OpenGymDictContainer> (action);
  Ptr<OpenGymBoxContainer<uint32_t>> base =
      DynamicCast<OpenGymBoxContainer<uint32_t>> (dict->Get ("base"));
  Ptr<OpenGymBoxContainer<uint32_t>> vel =
      DynamicCast<OpenGymBoxContainer<uint32_t>> (dict->Get ("vel"));
  m_velocity_parameter = vel->GetValue (0);
  m_reward = m_new_reward;
  m_new_reward = 0;
  if (m_velocity_parameter < 1) //
    {
      m_new_reward -= 10;
      m_velocity_parameter = 1.0;
    }
  else if (m_velocity_parameter > 64)
    {
      m_new_reward -= 10;
      m_velocity_parameter = 64.0;
    }
  return m_reward;
}

Ptr<OpenGymDataContainer>
TcpCopaRl::GetObservation ()
{

  // double m_sending_rate{0.0};
  // double m_expected_sending_rate{0.0};
  // double m_last_expected_sending_rate{0.0};

  // double m_min_rtt{1000.0};
  // double m_probe_min_rtt_len{10.0}; // minRTT 在 10秒内的最小值
  // double m_probe_rtt_timestamp{-1.0};

  // double m_rtt_standing{1000.0};
  // double m_rtt_standing_len{0.0};
  // double m_probe_rtt_standing_timestamp{-1.0};

  // double m_delay{0.0};
  // double m_delta{0.5};
  // // change to rl_velocity
  // double m_velocity_parameter{1.0};

  // double m_last_win{-1.0};
  // double m_last_time{-1.0};
  // double m_last_win_len{0.0};
  uint32_t parameterNum = 15;
  std::vector<uint32_t> shape = {
      parameterNum,
  };

  Ptr<OpenGymBoxContainer<double_t>> box = CreateObject<OpenGymBoxContainer<double_t>> (shape);
  box->AddValue (m_sending_rate);
  box->AddValue (m_expected_sending_rate);
  box->AddValue (m_last_expected_sending_rate);
  box->AddValue (m_min_rtt);
  box->AddValue (m_probe_min_rtt_len);
  box->AddValue (m_probe_rtt_timestamp);
  box->AddValue (m_rtt_standing);
  box->AddValue (m_rtt_standing_len);
  box->AddValue (m_probe_rtt_standing_timestamp);
  box->AddValue (m_delay);
  box->AddValue (m_delta);
  box->AddValue (m_velocity_parameter);
  box->AddValue (m_last_win);
  box->AddValue (m_last_time);
  box->AddValue (m_last_win_len);
  return box;

  // Print data
  NS_LOG_INFO ("MyGetObservation: " << box);
  return box;
}

Ptr<TcpCongestionOps>
TcpCopaRl::Fork ()
{
  return CopyObject<TcpCopaRl> (this);
}

void
TcpCopaRl::CreateGymEnv ()
{
  NS_LOG_FUNCTION (this);
  Ptr<TcpCopaEnv> env = CreateObject<TcpCopaEnv> ();
  env->SetSocketUuid (TcpCopaRl::GenerateUuid ());
  env->SetCaPtr (Ptr<TcpCopaRl> (this));
  m_tcpGymEnv = env;

  ConnectSocketCallbacks ();
}

} // namespace ns3