#include "instruction.hpp"

namespace heaven_ice {

std::string Instruction::to_string() const
{
  std::string out = name.to_string();
  if (size.has_value()) { out += F(".$", *size); }
  if (cond.has_value()) { out += F(" cond:$", *cond); }
  if (dst.has_value()) { out += F(" dst:$", *dst); }
  if (src.has_value()) { out += F(" src:$", *src); }
  if (register_list.has_value()) { out += F(" regs:$", *register_list); }
  return out;
}

bool Instruction::is_unconditional_jump() const
{
  switch (name) {
  case InstEnum::Bcc:
    return *cond == Condition::True;
  case InstEnum::RTS:
  case InstEnum::RTE:
  case InstEnum::JMP:
    return true;
  default:
    return false;
  }
}

bool Instruction::is_conditional_jump() const
{
  switch (name) {
  case InstEnum::Bcc:
  case InstEnum::DBcc:
    return !is_unconditional_jump();
  default:
    return false;
  }
}

std::optional<ulong_t> Instruction::jump_addr() const
{
  switch (name) {
  case InstEnum::Bcc:
  case InstEnum::DBcc:
  case InstEnum::BSR:
  case InstEnum::JSR:
  case InstEnum::JMP:
    return src->addr_opt();
  default:
    return std::nullopt;
  }
}

bool Instruction::is_fn_call() const
{
  switch (name) {
  case InstEnum::BSR:
  case InstEnum::JSR:
    return true;
  default:
    return false;
  }
}

} // namespace heaven_ice
