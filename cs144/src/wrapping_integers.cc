#include "wrapping_integers.hh"
#include <cstdint>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { static_cast<uint32_t>( n /*&(UINT32_MAX)*/ ) + zero_point.raw_value_ };
}

static uint64_t ___abs___( uint64_t a, uint64_t b )
{ // abs(a-b)
  if ( a > b )
    return a - b;
  return b - a;
}

static uint64_t closet( uint64_t x, uint64_t y, uint64_t z, uint64_t target )
{
  auto xl = ___abs___( x, target );
  auto yl = ___abs___( y, target );
  auto zl = ___abs___( z, target );
  if ( xl <= yl && xl <= zl )
    return x;
  if ( yl <= xl && yl <= zl )
    return y;
  return z;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{ // find the num k*(1UL<<32)+dis which is closet to checkpoint
  auto dis = raw_value_ - zero_point.raw_value_;
  auto _check = checkpoint & 0xffff'ffff'0000'0000;

  auto res1 = _check - ( 1UL << 32 ) + dis;
  auto res2 = _check + dis;
  auto res3 = _check + ( 1UL << 32 ) + dis;

  return closet( res1, res2, res3, checkpoint );
}
