#pragma once

#include <compare>

namespace heaven_ice {

struct Dir {
 public:
  enum E {
    Right,
    Left,
  };

  constexpr Dir(E e) : _e(e) {}
  constexpr operator E() const { return _e; }
  constexpr auto operator<=>(const Dir& other) const = default;

  bool operator==(E e) const { return e == _e; }

  const char* to_string() const;

  static Dir of_code(int code);

 private:
  E _e;
};

} // namespace heaven_ice
