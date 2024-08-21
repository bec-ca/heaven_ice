#pragma once

#include <string>

#include "yasf/of_stringable_mixin.hpp"

namespace heaven_ice {

struct ControlKey : public yasf::OfStringableMixin<ControlKey> {
 public:
  enum E {
    Start,
    Right,
    Left,
    Up,
    Down,
    A,
    B,
    C,
  };

  ControlKey(E v) : _v(v) {}

  operator E() const { return _v; }

  const char* to_string() const;

  static ControlKey of_string(const std::string& str);

 private:
  E _v;
};

} // namespace heaven_ice
