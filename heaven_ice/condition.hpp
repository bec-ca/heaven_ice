#pragma once

#include <compare>

namespace heaven_ice {

struct Condition {
 public:
  enum Value {
    True,  // Always true
    False, // Always false
    HI,    // Higher
    LS,    // Lower or same
    CC,    // Carry clear
    CS,    // Carry set
    NE,    // Not equal
    EQ,    // Equal
    VC,    // Overflow clear
    VS,    // Overflow set
    PL,    // Plus
    MI,    // Minus
    GE,    // Greater or equal
    LT,    // Less than
    GT,    // Greater than
    LE,    // Less or equal
  };

  Condition() {}
  Condition(Value v) : _value(v) {}

  static Condition of_code(int code);

  operator Value() const { return _value; }

  auto operator<=>(const Condition& other) const = default;
  auto operator==(const Value v) const { return v == _value; }

  const char* to_string() const;

 private:
  Value _value;
};

} // namespace heaven_ice
