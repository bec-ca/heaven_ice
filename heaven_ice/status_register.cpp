#include "status_register.hpp"

#include "bee/format.hpp"
#include "bee/or_error.hpp"

namespace heaven_ice {

using SRV = StatusRegister::StatusRegisterValue;

namespace {

char format_value(SRV v, char letter)
{
  switch (v) {
  case SRV::Clear:
    return tolower(letter);
  case SRV::Set:
    return toupper(letter);
  case SRV::Invalid:
    return '?';
  }
}

SRV of_bool(bool v) { return v ? SRV::Set : SRV::Clear; }

bool to_bool(SRV v)
{
  switch (v) {
  case SRV::Set:
    return true;
  case SRV::Clear:
    return false;
  case SRV::Invalid:
    raise_error("StatusRegister flag is invalid");
  }
}

} // namespace

StatusRegister::StatusRegister() {}

int StatusRegister::to_int() const
{
  int output = _int_priority_mask << 8;
  if (_carry == SRV::Set) output |= 1;
  if (_ov == SRV::Set) output |= 2;
  if (_zero == SRV::Set) output |= 4;
  if (_neg == SRV::Set) output |= 8;
  if (_ext == SRV::Set) output |= 16;
  return output;
}

void StatusRegister::set_from_int(int value)
{
  set_carry((value & 1) != 0);
  set_ov((value & 2) != 0);
  set_zero((value & 4) != 0);
  set_neg((value & 8) != 0);
  set_ext((value & 16) != 0);
  _int_priority_mask = (value >> 8) & 7;
}

std::string StatusRegister::to_string() const
{
  std::string str;
  str += format_value(_ext, 'x');
  str += format_value(_neg, 'n');
  str += format_value(_zero, 'z');
  str += format_value(_ov, 'v');
  str += format_value(_carry, 'c');
  str += ' ';
  str += F(_int_priority_mask);
  return str;
}

void StatusRegister::set_zero(bool value) { _zero = of_bool(value); }

void StatusRegister::set_ext(bool value) { _ext = of_bool(value); }

void StatusRegister::set_neg(bool value) { _neg = of_bool(value); }

void StatusRegister::set_ov(bool value) { _ov = of_bool(value); }

void StatusRegister::set_carry(bool value) { _carry = of_bool(value); }

bool StatusRegister::ext() const { return to_bool(_ext); }

bool StatusRegister::neg() const { return to_bool(_neg); }

bool StatusRegister::zero() const { return to_bool(_zero); }

bool StatusRegister::ov() const { return to_bool(_ov); }

bool StatusRegister::carry() const { return to_bool(_carry); }

bool StatusRegister::check_condition(Condition cond) const
{
  switch (cond) {
  case Condition::True:
    return true;
  case Condition::False:
    return false;
  case Condition::NE:
    return !zero();
  case Condition::EQ:
    return zero();
  case Condition::CC:
    return !carry();
  case Condition::CS:
    return carry();
  case Condition::PL:
    return !neg();
  case Condition::MI:
    return neg();
  case Condition::LS:
    return carry() || zero();
  case Condition::LT:
    return neg() != ov();
  case Condition::HI:
    return !carry() && !zero();
  case Condition::GT:
    return (neg() == ov()) && !zero();
  default:
    raise_error("Condition not implemented: $", cond);
  }
}

void StatusRegister::invalidate_cc()
{
  _ext = SRV::Invalid;
  _neg = SRV::Invalid;
  _zero = SRV::Invalid;
  _ov = SRV::Invalid;
  _carry = SRV::Invalid;
}

} // namespace heaven_ice
