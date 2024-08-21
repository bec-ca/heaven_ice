#pragma once

#include <string>

#include "condition.hpp"

namespace heaven_ice {

struct StatusRegister {
  enum class StatusRegisterValue { Set, Clear, Invalid };

  StatusRegister();

  int to_int() const;

  void set_from_int(int value);

  std::string to_string() const;

  void set_zero(bool value);
  void set_ext(bool value);
  void set_neg(bool value);
  void set_ov(bool value);
  void set_carry(bool value);

  bool ext() const;
  bool neg() const;
  bool zero() const;
  bool ov() const;
  bool carry() const;

  void invalidate_cc();

  int int_priority_mask() const { return _int_priority_mask; }

  auto operator<=>(const StatusRegister& other) const = default;

  bool check_condition(Condition cond) const;

 private:
  void _assert_cc_valid() const;

  StatusRegisterValue _ext = StatusRegisterValue::Clear;
  StatusRegisterValue _neg = StatusRegisterValue::Clear;
  StatusRegisterValue _zero = StatusRegisterValue::Clear;
  StatusRegisterValue _ov = StatusRegisterValue::Clear;
  StatusRegisterValue _carry = StatusRegisterValue::Clear;

  int _int_priority_mask = 0;
};

} // namespace heaven_ice
