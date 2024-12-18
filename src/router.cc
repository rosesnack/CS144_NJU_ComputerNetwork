#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  routing_table_.emplace_back(route_prefix, prefix_length, next_hop, interface_num);
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for (auto& itf : _interfaces) {
    while (!itf->datagrams_received().empty()) {
      auto dgram = itf->datagrams_received().front();
      if (dgram.header.ttl > 1) {
        --dgram.header.ttl;
        dgram.header.compute_checksum();
        uint32_t dst_ip = dgram.header.dst;

        auto target = routing_table_.end();
        int max_length = -1;
        for (auto it = routing_table_.begin(); it != routing_table_.end(); ++it) {
          uint32_t a1 = 0;
          uint32_t a2 = 0;
          if (it->prefix_length != 0) {
            uint8_t shiftBits = 32 - it->prefix_length; 
            a1 = dst_ip >> shiftBits;           
            a2 = it->route_prefix >> shiftBits;
          }
          if (a1 == a2 && it->prefix_length > max_length) {
            max_length = it->prefix_length;
            target = it;
          }
        }
        if (target != routing_table_.end()) 
          interface(target->interface_num)->send_datagram(dgram, target->next_hop.value_or(Address::from_ipv4_numeric(dst_ip)));
      }
      itf->datagrams_received().pop();
    }
  }
}