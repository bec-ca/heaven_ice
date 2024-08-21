#include "addr_mode.hpp"

#include "magic_constants.hpp"

#include "bee/format.hpp"
#include "bee/hex.hpp"

namespace heaven_ice {
namespace {

template <class T>
std::string addr_to_string(T addr, const std::map<ulong_t, std::string>& labels)
{
  if (auto it = labels.find(addr); it != labels.end()) {
    return it->second;
  } else {
    return F("($)", simple_hex(addr));
  }
}

} // namespace

std::string AddrMode::to_string() const
{
  switch (kind) {
  case AddrModeKind::ImmByte:
    return F("#$", simple_hex<sbyte_t>(imm));
  case AddrModeKind::ImmWord:
    return F("#$", simple_hex<sword_t>(imm));
  case AddrModeKind::ImmLong:
    return F("#$", simple_hex<slong_t>(imm));
  case AddrModeKind::ImmAddrWord:
    return F("($)", simple_hex<uword_t>(imm));
  case AddrModeKind::ImmAddrLong:
    return F("($)", simple_hex<ulong_t>(imm));
  case AddrModeKind::Reg:
    return F(reg);
  case AddrModeKind::AReg:
    return F("($)", reg);
  case AddrModeKind::PostInc:
    return F("($)+", reg);
  case AddrModeKind::PreDec:
    return F("-($)", reg);
  case AddrModeKind::ALongDisp:
    return F(
      "($.$){}", reg, idx_size.to_string(), signed_simple_hex<slong_t>(imm));
  case AddrModeKind::AXByteDisp:
    return F(
      "($,$.$){}",
      reg,
      reg2,
      idx_size.to_string(),
      signed_simple_hex<sbyte_t>(imm));
  }
}

std::string AddrMode::to_string_with_labels(
  const std::map<ulong_t, std::string>& labels) const
{
  switch (kind) {
  case AddrModeKind::ImmAddrWord:
    return addr_to_string<uword_t>(imm, labels);
  case AddrModeKind::ImmAddrLong:
    return addr_to_string<ulong_t>(imm, labels);
  default:
    return to_string();
  }
}

AddrMode AddrMode::make_reg(RegisterId reg)
{
  return {.kind = AddrModeKind::Reg, .reg = reg};
}

AddrMode AddrMode::make_imm_byte(sbyte_t value)
{
  return {.kind = AddrModeKind::ImmByte, .imm = value};
}

AddrMode AddrMode::make_imm_word(sword_t value)
{
  return {.kind = AddrModeKind::ImmWord, .imm = value};
}

AddrMode AddrMode::make_imm_long(slong_t value)
{
  return {.kind = AddrModeKind::ImmLong, .imm = value};
}

AddrMode AddrMode::make_imm(SizeKind size, slong_t value)
{
  switch (size) {
  case SizeKind::Byte:
    return make_imm_byte(value);
  case SizeKind::Word:
    return make_imm_word(value);
  case SizeKind::Long:
    return make_imm_long(value);
  }
}

AddrMode AddrMode::make_imm_addr(ulong_t addr)
{
  return {.kind = AddrModeKind::ImmAddrLong, .imm = slong_t(addr)};
}

std::optional<ulong_t> AddrMode::const_opt() const
{
  switch (kind) {
  case AddrModeKind::ImmByte:
  case AddrModeKind::ImmWord:
  case AddrModeKind::ImmLong:
    return imm;
  case AddrModeKind::ImmAddrWord:
  case AddrModeKind::ImmAddrLong:
  case AddrModeKind::Reg:
  case AddrModeKind::AReg:
  case AddrModeKind::PostInc:
  case AddrModeKind::PreDec:
  case AddrModeKind::ALongDisp:
  case AddrModeKind::AXByteDisp:
    return std::nullopt;
  }
}

std::optional<ulong_t> AddrMode::addr_opt() const
{
  switch (kind) {
  case AddrModeKind::ImmAddrWord:
  case AddrModeKind::ImmAddrLong:
    return imm;
  case AddrModeKind::ImmByte:
  case AddrModeKind::ImmWord:
  case AddrModeKind::ImmLong:
  case AddrModeKind::Reg:
  case AddrModeKind::AReg:
  case AddrModeKind::PostInc:
  case AddrModeKind::PreDec:
  case AddrModeKind::ALongDisp:
  case AddrModeKind::AXByteDisp:
    return std::nullopt;
  }
}

bool AddrMode::is_addr_reg() const
{
  switch (kind) {
  case AddrModeKind::Reg:
    return reg.is_addr();
  default:
    return false;
  }
}

bool AddrMode::is_inc_or_dec() const
{
  switch (kind) {
  case AddrModeKind::PostInc:
  case AddrModeKind::PreDec:
    return true;
  default:
    return false;
  }
}

} // namespace heaven_ice
