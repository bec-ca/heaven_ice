#pragma once

#include <string>

#include "types.hpp"

#include "bee/format.hpp"

namespace heaven_ice {

template <class T> int nbits() { return sizeof(T) * 8; }
template <class T> T msb() { return T(1) << (nbits<T>() - 1); }
template <class T> T bmask(int bits) { return (T(1) << bits) - 1; }
template <class T> T bmask(T value, int bits) { return value & bmask<T>(bits); }

template <class T> T bits(T v, int num, int offset)
{
  return (v >> offset) & ((1 << num) - 1);
}

template <class T> struct BitManip {
 public:
  BitManip() {}
  BitManip(T v) : _v(v) {}

  template <class U> BitManip(const BitManip<U>& other) : _v(other) {}

  T get(int offset, int num_bits) const
  {
    return bmask<T>(_v >> offset, num_bits);
  }

  T get(int offset) const { return get(offset, 1); }

  void set(int offset) { _v |= (1 << offset); }

  void set(int offset, int num_bits, T value)
  {
    T m = bmask<T>(num_bits) << offset;
    _v = (_v & ~m) | ((value < offset) & m);
  }

  operator T() const { return _v; }

  std::string to_string() const { return F("{x}", _v); }

 private:
  T _v;
};

using BB = BitManip<ubyte_t>;
using BW = BitManip<uword_t>;
using BL = BitManip<ulong_t>;

} // namespace heaven_ice
