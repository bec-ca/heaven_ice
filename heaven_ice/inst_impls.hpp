#pragma once

#include <limits>
#include <type_traits>

#include "bit_manip.hpp"
#include "globals.hpp"
#include "size_kind.hpp"
#include "types.hpp"

namespace heaven_ice {
namespace details {

template <class T> T minv() { return std::numeric_limits<T>::min(); }
template <class T> T maxv() { return std::numeric_limits<T>::max(); }

template <class T> auto make_signed_v(T v);

template <> inline auto make_signed_v<AddrRegister>(AddrRegister v)
{
  return slong_t(v);
}

template <class T> auto make_signed_v(T v) { return std::make_signed_t<T>(v); }

template <class T> bool is_neg(T v) { return make_signed_v<T>(v) < 0; }

} // namespace details

template <class T> void TST(T v)
{
  G.sr.set_neg(details::is_neg<T>(v));
  G.sr.set_zero(v == 0);
  G.sr.set_ov(false);
  G.sr.set_carry(false);
}

template <class T> T UCC(T v)
{
  TST<T>(v);
  return v;
}

template <class T>
  requires std::is_signed_v<T>
T CMP(T dst, T src)
{
  using U = std::make_unsigned_t<T>;

  T res = dst - src;

  bool ov_max = src < 0 && dst > details::maxv<T>() + src;
  bool ov_min = src > 0 && dst < details::minv<T>() + src;

  G.sr.set_neg(details::is_neg<T>(res));
  G.sr.set_zero(res == 0);
  G.sr.set_ov(ov_max || ov_min);
  G.sr.set_carry(U(dst) < U(src));

  return res;
}

template <class T> void BTST(T dst, int bit)
{
  using U = std::make_unsigned_t<T>;
  G.sr.set_zero(((U(dst) >> bit) & 1) == 0);
}

template <class T>
  requires std::is_unsigned_v<T>
T ROR(T v, int bits)
{
  bits %= 64;
  T res = (v >> bits) | (v << (nbits<T>() - bits));
  if (bits > 0) {
    bool last_bit = ((v << (nbits<T>() - bits)) & 1) == 1;
    G.sr.set_carry(last_bit);
  } else {
    G.sr.set_carry(false);
  }
  G.sr.set_neg(details::is_neg(res));
  G.sr.set_zero(res == 0);
  G.sr.set_ov(false);
  return res;
}

template <class T> T ROR(T v, int bits)
{
  return ROR<std::make_unsigned_t<T>>(v, bits);
}

template <class T>
  requires std::is_unsigned_v<T>
T ROL(T v, int bits)
{
  G.sr.invalidate_cc();
  return (v << bits) | (v >> (nbits<T>() - bits));
}

template <class T> T ROL(T v, int bits)
{
  return ROL<std::make_unsigned_t<T>>(v, bits);
}

template <class T> T SHR(T v, int bits)
{
  G.sr.invalidate_cc();
  T res = v >> bits;
  G.sr.set_zero(res == 0);
  return res;
}

template <class T> T SHL(T v, int bits)
{
  bits %= 64;
  T res = v << bits;
  if (bits > 0) {
    bool last_bit = ((v >> (nbits<T>() - bits)) & 1) == 1;
    G.sr.set_ext(last_bit);
    G.sr.set_carry(last_bit);
  } else {
    G.sr.set_carry(false);
  }
  G.sr.set_neg(details::is_neg<T>(res));
  G.sr.set_zero(res == 0);
  G.sr.set_ov(false);
  return res;
}

template <class T> T ASR(T v, int bits)
{
  return SHR<std::make_signed_t<T>>(v, bits);
}
template <class T> T ASL(T v, int bits)
{
  return SHL<std::make_signed_t<T>>(v, bits);
}
template <class T> T LSR(T v, int bits)
{
  return SHR<std::make_unsigned_t<T>>(v, bits);
}
template <class T> T LSL(T v, int bits)
{
  return SHL<std::make_unsigned_t<T>>(v, bits);
}

inline ulong_t SWAP(ulong_t v) { return (v >> 16) | (v << 16); }

template <class T> T NEG(T v)
{
  T r = -v;
  G.sr.set_ext(r != 0);
  G.sr.set_neg(details::is_neg<T>(r));
  G.sr.set_zero(r == 0);
  G.sr.set_ov(false);
  G.sr.set_carry(r != 0);
  return r;
}

template <class T> T NOT(T v)
{
  G.sr.invalidate_cc();
  return ~v;
}

template <class T> T ANDR(T a, T b) { return a & b; }

template <class T> T AND(T a, T b) { return UCC<T>(a & b); }

template <class T> T ORR(T a, T b) { return a | b; }

template <class T> T OR(T a, T b)
{
  G.sr.invalidate_cc();
  return a | b;
}

template <class T> T EOR(T a, T b)
{
  G.sr.invalidate_cc();
  return a ^ b;
}

template <class T> T ADD(T a, T b)
{
  // TODO: ADDA does not update CC
  G.sr.invalidate_cc();
  T ret = a + b;
  G.sr.set_zero(ret == 0);
  return ret;
}

// TODO: SUBA does not update CC
template <class T> T SUB(T a, T b) { return CMP<T>(a, b); }

template <class T> T MUL(T a, T b)
{
  G.sr.invalidate_cc();
  return a * b;
}

inline ulong_t MULU(uword_t a, uword_t b) { return MUL<ulong_t>(a, b); }

inline slong_t MULS(sword_t a, sword_t b) { return MUL<slong_t>(a, b); }

template <class T> T DIV(T a, T b)
{
  G.sr.invalidate_cc();
  T q = a / b;
  T r = a % b;
  return (q & 0xffff) | (r << 16);
}

inline slong_t DIVS(slong_t a, slong_t b) { return DIV<slong_t>(a, b); }

inline ulong_t DIVU(ulong_t a, ulong_t b) { return DIV<ulong_t>(a, b); }

template <class T> T BCLR(T v, int bit)
{
  BTST<T>(v, bit);
  return v & (~(1 << bit));
}

template <class T> T BSET(T v, int bit)
{
  BTST<T>(v, bit);
  return v | (1 << bit);
}

template <class T> T BCHG(T v, int bit)
{
  BTST<T>(v, bit);
  return v ^ (1 << bit);
}

inline ubyte_t ABCD(ubyte_t a, ubyte_t b)
{
  ubyte_t lo = (a & 0xf) + (b & 0xf);
  ubyte_t hi = (a >> 4) + (b >> 4);
  if (G.sr.ext()) lo++;
  if (lo >= 10) {
    lo -= 10;
    hi += 1;
  }
  bool carry = false;
  if (hi >= 10) {
    carry = true;
    hi -= 10;
  }
  ubyte_t ret = lo | (hi << 4);

  G.sr.set_ext(carry);
  G.sr.set_carry(carry);
  G.sr.set_neg(details::is_neg<ubyte_t>(ret));
  if (ret != 0) { G.sr.set_zero(false); }

  return ret;
}

template <class T, class U, class V> void EXG(U& r1, V& r2)
{
  T v1 = r1.template get<T>();
  T v2 = r2.template get<T>();
  r1.template set<T>(v2);
  r2.template set<T>(v1);
}

template <class T> inline T EXT(prev_size_t<T> v) { return v; }

inline void NOOP() {}

inline void RTE() {}

template <class T> bool NOT_MINUS_ONE(T v) { return v != -1; }

using B = sbyte_t;
using W = sword_t;
using L = slong_t;
using UB = ubyte_t;
using UW = uword_t;
using UL = ulong_t;

} // namespace heaven_ice
