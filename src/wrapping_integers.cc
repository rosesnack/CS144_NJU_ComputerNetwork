#include "wrapping_integers.hh"
#include <cmath> 
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ) //n: absolute sequence number
{
  return Wrap32(zero_point + n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t a = checkpoint / 4294967296;
  uint64_t res = raw_value_ - zero_point.raw_value_ + a * 4294967296;
  if (res > checkpoint) {
    if (res >= 4294967296 && res - 2147483648 > checkpoint)
      res -= 4294967296;
  }
  if (res < checkpoint && res + 2147483648 < checkpoint)
    res += 4294967296;
  return res;
}