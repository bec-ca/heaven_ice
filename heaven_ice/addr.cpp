#include "addr.hpp"

#include "bee/format.hpp"
#include "bee/or_error.hpp"

namespace heaven_ice {

std::string Addr::to_string() const
{
  switch (kind) {
  case AddrKind::Reg:
    return F(reg);
  case AddrKind::Ram:
    return F("(#{06x})", addr);
  case AddrKind::ImmByte:
    return F("#{02x}", imm);
  case AddrKind::ImmWord:
    return F("#{04x}", imm);
  case AddrKind::ImmLong:
    return F("#{06x}", imm);
  }
}

ulong_t Addr::get_ram_addr() const
{
  switch (kind) {
  case AddrKind::Ram:
    return addr;
  case AddrKind::Reg:
  case AddrKind::ImmByte:
  case AddrKind::ImmWord:
  case AddrKind::ImmLong:
    raise_error("Not a ram address: $", *this);
  }
}

Addr Addr::make_reg(const RegisterId& reg)
{
  return {.kind = AddrKind::Reg, .reg = reg};
}

Addr Addr::ram(ulong_t addr) { return {.kind = AddrKind::Ram, .addr = addr}; }

Addr Addr::make_imm_word(sword_t w)
{
  return {.kind = AddrKind::ImmWord, .imm = w};
}

Addr Addr::make_imm_long(slong_t w)
{
  return {.kind = AddrKind::ImmLong, .imm = w};
}

Addr Addr::make_imm_byte(sbyte_t w)
{
  return {.kind = AddrKind::ImmByte, .imm = w};
}

Addr Addr::make_imm(SizeKind size, slong_t w)
{
  switch (size) {
  case SizeKind::Byte:
    return make_imm_byte(w);
  case SizeKind::Word:
    return make_imm_word(w);
  case SizeKind::Long:
    return make_imm_long(w);
  }
}

} // namespace heaven_ice
