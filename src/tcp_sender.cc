#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return sequence_numbers_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  TCPSenderMessage mss;
  uint64_t avail_window_size = 1, len;
  string str;
  if (window_size_ == 0) {
    len = 1;
    if (sequence_numbers_in_flight_ > 0)
      return;
  }
  else {
    avail_window_size = window_size_ - sequence_numbers_in_flight_;
    if (window_size_ < sequence_numbers_in_flight_) avail_window_size = 0;
    len = min(avail_window_size, TCPConfig::MAX_PAYLOAD_SIZE);
    if (len == 0) return; //window is full
  }

  read(input_.reader(), len, str);
  mss.payload = str;
  mss.seqno = sn_;
  if (input_.has_error()) mss.RST = true;
  if (!SYN_) SYN_ = mss.SYN = true;
  if (input_.reader().is_finished() && !FIN_ && str.length() != avail_window_size) FIN_ = mss.FIN = true;
  sn_ = sn_ + mss.sequence_length();

  if (mss.sequence_length() != 0) {
    pq.push(mss);
    sequence_numbers_in_flight_ += mss.sequence_length();
    transmit(mss);
    if (!timer_) timer_ = true;
    push(transmit); //直到没有要传输的数据为止
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage mss;
  mss.seqno = sn_;
  if (input_.has_error()) mss.RST = true;
  return mss;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  window_size_ = msg.window_size;
  if (sn_ < msg.ackno) return;  //Impossible ackno (beyond next seqno) is ignored
  if (msg.RST)  input_.set_error();
  while (!pq.empty() && pq.top().seqno + (pq.top().sequence_length() - 1) < msg.ackno) {
    sequence_numbers_in_flight_ -= pq.top().sequence_length();
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
    if (time_passed_ >= RTO_) {
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
