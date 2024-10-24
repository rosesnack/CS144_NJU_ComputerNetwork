#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if (message.RST) reader().set_error();
  if (!SYN_) {
    if (!message.SYN) return;  //忽略没有SYN时到来的信息
    SYN_ = true;  
    zero_point_ = message.seqno;
  }
  uint64_t stream_index = message.seqno.unwrap(zero_point_, checkpoint);
  if (!message.SYN) stream_index--;  //stream_index不包括SYN
  checkpoint = stream_index;  //更新checkpoint
  reassembler_.insert(stream_index, message.payload, message.FIN);
  if (message.FIN) FIN_ = true;
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage msg;
  uint64_t end_index = writer().bytes_pushed() + 1;  //下一个想要的absolute seqno（从stream index转换要+1)
  if (FIN_ && reassembler_.bytes_pending() == 0) end_index++; //考虑FIN占的序号
  if (SYN_) msg.ackno = Wrap32::wrap(end_index, zero_point_); //转换为ackno

  uint64_t window_size = writer().available_capacity();
  if (window_size > 65535) window_size = 65535;  //window_size最大为65535
  msg.window_size = window_size;

  if (writer().has_error()) msg.RST = true;

  return msg;
}
