#pragma once

#include "register_id.hpp"
#include "size_kind.hpp"
#include "types.hpp"

namespace heaven_ice {

enum class AddrKind {
  Reg,
  Ram,
  ImmByte,
  ImmWord,
  ImmLong,
};

struct Addr {
  AddrKind kind;

  union {
    RegisterId reg;
    ulong_t addr;
    slong_t imm;
  };

  std::string to_string() const;

  ulong_t get_ram_addr() const;

  static Addr make_reg(const RegisterId& reg);
  static Addr ram(ulong_t addr);
  static Addr make_imm_word(sword_t w);
  static Addr make_imm_long(slong_t w);
  static Addr make_imm_byte(sbyte_t w);
  static Addr make_imm(SizeKind size, slong_t w);
};

} // namespace heaven_ice
