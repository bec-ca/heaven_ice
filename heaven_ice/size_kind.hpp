#pragma once

#include <compare>
#include <string>
#include <type_traits>

#include "types.hpp"

#include "bee/concepts.hpp"
#include "bee/or_error.hpp"

namespace heaven_ice {

struct SizeKind {
 public:
  enum Value {
    Byte = 0,
    Word = 1,
    Long = 2,
  };

  SizeKind() {}
  SizeKind(Value value) : _e(value) {}

  operator Value() const { return _e; }
  auto operator<=>(const SizeKind& other) const = default;

  int num_bytes() const;
  int num_bits() const;

  static SizeKind b() { return Byte; }
  static SizeKind w() { return Word; }
  static SizeKind l() { return Long; }

  static bee::OrError<SizeKind> decode3(int code);
  static bee::OrError<SizeKind> decode_move(int code);
  static bee::OrError<SizeKind> decode2(int code);

  const char* to_string() const;

  std::string format(slong_t value) const;

  ulong_t msb_mask() const;

  SizeKind previous() const;

  slong_t trim(slong_t value) const;

  ulong_t mask() const;

  slong_t max_value() const;
  slong_t min_value() const;

  const char* stype_name() const;
  const char* utype_name() const;

  inline bool operator<(const SizeKind& other) const { return _e < other._e; }

  template <class T> static SizeKind of_type()
  {
    using U = std::make_signed_t<std::decay_t<T>>;
    if constexpr (std::is_same_v<U, sbyte_t>) {
      return Byte;
    } else if constexpr (std::is_same_v<U, sword_t>) {
      return Word;
    } else if constexpr (std::is_same_v<U, slong_t>) {
      return Long;
    } else {
      static_assert(bee::always_false<T>, "Unsupported type");
    }
  }

 private:
  Value _e;
};

template <class T> struct prev_size;

template <> struct prev_size<slong_t> {
  using type = sword_t;
};
template <> struct prev_size<sword_t> {
  using type = sbyte_t;
};
template <> struct prev_size<sbyte_t> {
  using type = sbyte_t;
};

template <class T> using prev_size_t = prev_size<T>::type;

} // namespace heaven_ice
