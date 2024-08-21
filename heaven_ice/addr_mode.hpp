#pragma once

#include <map>
#include <optional>
#include <string>

#include "register_id.hpp"
#include "size_kind.hpp"
#include "types.hpp"

namespace heaven_ice {

enum class AddrModeKind {
  ImmByte,
  ImmWord,
  ImmLong,
  ImmAddrWord,
  ImmAddrLong,
  Reg,
  AReg,
  PostInc,
  PreDec,
  ALongDisp,
  AXByteDisp,
};

struct AddrMode {
  AddrModeKind kind;

  slong_t imm{};

  RegisterId reg{};
  RegisterId reg2{};

  SizeKind idx_size{};

  std::string to_string() const;
  std::string to_string_with_labels(
    const std::map<ulong_t, std::string>& labels) const;

  static AddrMode make_reg(RegisterId reg);
  static AddrMode make_imm_byte(sbyte_t value);
  static AddrMode make_imm_word(sword_t value);
  static AddrMode make_imm_long(slong_t value);
  static AddrMode make_imm(SizeKind size, slong_t value);
  static AddrMode make_imm_addr(ulong_t value);

  std::optional<ulong_t> const_opt() const;
  std::optional<ulong_t> addr_opt() const;

  bool is_addr_reg() const;

  bool is_inc_or_dec() const;
};

} // namespace heaven_ice
