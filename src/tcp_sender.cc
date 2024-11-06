#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return pq.size();
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  TCPSenderMessage mss;
  uint64_t len = min(window_size_, (uint16_t)TCPConfig::MAX_PAYLOAD_SIZE);
  string str;
  read(input_.reader(), len, str);
  mss.seqno = sn_;
  if (sn_ == isn_) mss.SYN = true;
  sn_ = sn_ + mss.sequence_length();
  mss.payload = str;

  if (!timer_) timer_ = true;
  pq.push(mss);
  transmit(mss);
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage mss;
  mss.seqno = sn_;
  return mss;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  while (!pq.empty() && pq.top().seqno < msg.ackno) {
    pq.pop();
    RTO_ = initial_RTO_ms_;
    time_passed_ = 0;
    if (pq.empty())
      timer_ = false;
    consecutive_retransmissions_ = 0;  
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if (timer_) {
    time_passed_ += ms_since_last_tick;
    if (time_passed_ > RTO_) {
      TCPSenderMessage mss= pq.top();
      transmit(mss);
      if (window_size_ != 0) {
        consecutive_retransmissions_++;
        RTO_ *= 2;
      }
      time_passed_ = 0;
    }
  }
}
