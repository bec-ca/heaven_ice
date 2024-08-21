#pragma once

#include <array>
#include <memory>
#include <string>

#include "addr.hpp"
#include "addr_mode.hpp"
#include "condition.hpp"
#include "size_kind.hpp"
#include "types.hpp"

namespace heaven_ice {

struct Machine {
  Machine(bool verbose);

  slong_t read_register(SizeKind size, RegisterId id) const;

  void write_register(SizeKind size, RegisterId id, slong_t value);

  Addr read_address(SizeKind size, const AddrMode& am);

  slong_t read_value(SizeKind size, const Addr& addr, int inc_addr = 0);

  void write_value(
    SizeKind size, const Addr& addr, slong_t value, int inc_addr = 0);

  slong_t read_value(SizeKind size, const AddrMode& am, int inc_addr = 0);

  void write_value(
    SizeKind size, const AddrMode& am, slong_t value, int inc_addr = 0);

  void jump(ulong_t addr);

  void push(SizeKind size, slong_t addr);

  slong_t pop(SizeKind size);

  void set_sp(ulong_t v);

  ulong_t sp() const;

  void print_registers() const;

 private:
  slong_t read_register_quiet(SizeKind size, RegisterId id) const;

  void write_register_quiet(SizeKind size, RegisterId id, slong_t value);

  bool _verbose;
};

} // namespace heaven_ice
