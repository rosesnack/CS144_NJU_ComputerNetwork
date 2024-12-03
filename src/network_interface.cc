#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
  , arp_cache_()
  , queued_datagrams_()
  , pending_arp_requests_()
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // 如果已经缓存了目标IP的以太网地址，直接发送
  auto it = arp_cache_.find(next_hop.ipv4_numeric());
  if (it != arp_cache_.end()) {
    EthernetFrame frame;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.header.src = ethernet_address_;
    frame.header.dst = it->second.first;
    frame.payload = serialize(dgram);
    transmit(frame);  
  } else {
    // 如果没有缓存目标地址，广播ARP请求
    if (pending_arp_requests_.find(next_hop.ipv4_numeric()) == pending_arp_requests_.end()) {
      pending_arp_requests_.insert({next_hop.ipv4_numeric(), 0});
      ARPMessage arpmsg;
      arpmsg.opcode = ARPMessage::OPCODE_REQUEST;
      arpmsg.sender_ip_address = ip_address_.ipv4_numeric();
      arpmsg.sender_ethernet_address = ethernet_address_;
      arpmsg.target_ip_address = next_hop.ipv4_numeric();

      EthernetFrame frame {{ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP},
                             serialize(arpmsg)};
      transmit(frame);
    }
    // 将数据报缓存，等待ARP响应
    queued_datagrams_[next_hop.ipv4_numeric()].push_back(dgram);
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if (frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST)
    return;

  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram datagram;
    if (parse(datagram, frame.payload)) {
      datagrams_received_.push(datagram);
    }
  }
  else if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp_msg;
    if (parse(arp_msg, frame.payload)) {
      //remember the mapping between the sender's IP address and Ethernet address for 30 seconds
      uint32_t sender_ip_address = arp_msg.sender_ip_address;
      EthernetAddress sender_ethernet_address = arp_msg.sender_ethernet_address;
      arp_cache_[sender_ip_address] = {sender_ethernet_address, 0};

      for (auto it = queued_datagrams_.begin(); it != queued_datagrams_.end(); ) {
        if (it->first == sender_ip_address) {
          for (auto&y : it->second) {
            EthernetFrame frame2;
            frame2.header.type = EthernetHeader::TYPE_IPv4;
            frame2.header.src = ethernet_address_;
            frame2.header.dst = sender_ethernet_address;
            frame2.payload = serialize(y);
            transmit(frame2); 
          }
        }
        it = queued_datagrams_.erase(it);
      }

      if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST 
      && arp_msg.target_ip_address == ip_address_.ipv4_numeric()) {
          ARPMessage reply_msg;
          reply_msg.opcode = ARPMessage::OPCODE_REPLY;
          reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
          reply_msg.sender_ethernet_address = ethernet_address_;
          reply_msg.target_ip_address = arp_msg.sender_ip_address;
          reply_msg.target_ethernet_address = arp_msg.sender_ethernet_address;

          EthernetFrame reply_frame {
            {arp_msg.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP},
            serialize(reply_msg)};
          transmit(reply_frame);
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for (auto it = pending_arp_requests_.begin(); it != pending_arp_requests_.end(); ) {
    it->second += ms_since_last_tick;
    if (it->second >= 5000) {
        it = pending_arp_requests_.erase(it);
    } else {
        ++it;
    }
  }
  for (auto it = arp_cache_.begin(); it != arp_cache_.end(); ) {
    it->second.second += ms_since_last_tick;
    if (it->second.second >= 30000) {
        it = arp_cache_.erase(it);
    } else {
        ++it;
    }
  }
}
