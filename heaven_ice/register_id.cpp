#include "register_id.hpp"

#include "bee/format.hpp"
#include "bee/or_error.hpp"

namespace heaven_ice {

const char* RegisterId::register_kind_prefix(RegisterKind kind)
{
  switch (kind) {
  case RegisterKind::Data:
    return "D";
  case RegisterKind::Addr:
    return "A";
  case RegisterKind::SR:
    return "SR";
  }
}

std::string RegisterId::to_string() const
{
  switch (kind) {
  case RegisterKind::Addr: {
    if (reg_id == 7) { return "USP"; }
  } break;
  case RegisterKind::SR:
    return "SR";
  case RegisterKind::Data:
    break;
  }
  return F("{}{}", register_kind_prefix(kind), reg_id);
}

void RegisterId::check_id(int reg_id)
{
  if (reg_id < 0 || reg_id >= 8) {
    raise_error("Invalid register number: $", reg_id);
  }
}

RegisterId RegisterId::data(int reg_id)
{
  check_id(reg_id);
  return {RegisterKind::Data, reg_id};
}

RegisterId RegisterId::addr(int reg_id)
{
  check_id(reg_id);
  return {RegisterKind::Addr, reg_id};
}

RegisterId RegisterId::usp() { return addr(7); }
RegisterId RegisterId::sr() { return {RegisterKind::SR, 0}; }

bool RegisterId::is_addr() const { return kind == RegisterKind::Addr; }

bool RegisterId::operator==(const RegisterId& rhs) const = default;

} // namespace heaven_ice
