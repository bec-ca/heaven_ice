#include "machine.hpp"

#include "globals.hpp"

#include "bee/format.hpp"
#include "bee/print.hpp"

namespace heaven_ice {

Machine::Machine(bool verbose) : _verbose(verbose) {}

slong_t Machine::read_register(SizeKind size, RegisterId id) const
{
  auto ret = read_register_quiet(size, id);
  if (_verbose) P("$.$ -> #$", id, size, size.format(ret));
  return ret;
}

void Machine::write_register(SizeKind size, RegisterId id, slong_t value)
{
  if (_verbose) P("$.$ <- #$", id, size, size.format(value));
  write_register_quiet(size, id, value);
}

Addr Machine::read_address(SizeKind size, const AddrMode& am)
{
  switch (am.kind) {
  case AddrModeKind::ImmByte:
    return {.kind = AddrKind::ImmByte, .imm = sbyte_t(am.imm)};
  case AddrModeKind::ImmWord:
    return {.kind = AddrKind::ImmWord, .imm = sword_t(am.imm)};
  case AddrModeKind::ImmLong:
    return {.kind = AddrKind::ImmLong, .imm = slong_t(am.imm)};
  case AddrModeKind::Reg:
    return Addr::make_reg(am.reg);
  case AddrModeKind::AReg:
    return Addr::ram(read_register(SizeKind::Long, am.reg));
  case AddrModeKind::ImmAddrWord:
    return Addr::ram(uword_t(am.imm));
  case AddrModeKind::ImmAddrLong:
    return Addr::ram(ulong_t(am.imm));
  case AddrModeKind::PostInc: {
    ulong_t addr = read_register(SizeKind::Long, am.reg);
    write_register(SizeKind::Long, am.reg, addr + size.num_bytes());
    return Addr::ram(addr);
  }
  case AddrModeKind::PreDec: {
    ulong_t addr = read_register(SizeKind::Long, am.reg) - size.num_bytes();
    write_register(SizeKind::Long, am.reg, addr);
    return Addr::ram(addr);
  }
  case AddrModeKind::ALongDisp:
    return Addr::ram(read_register(am.idx_size, am.reg) + slong_t(am.imm));
  case AddrModeKind::AXByteDisp:
    return Addr::ram(
      read_register(SizeKind::Long, am.reg) +
      read_register(am.idx_size, am.reg2) + sbyte_t(am.imm));
  };
}

slong_t Machine::read_value(SizeKind size, const Addr& addr, int inc_addr)
{
  switch (addr.kind) {
  case AddrKind::ImmByte:
    return sbyte_t(addr.imm);
  case AddrKind::ImmWord:
    return sword_t(addr.imm);
  case AddrKind::ImmLong:
    return slong_t(addr.imm);
  case AddrKind::Ram:
    return G.io->read_signed(size, addr.addr + inc_addr);
  case AddrKind::Reg:
    return read_register(size, addr.reg);
  }
}

void Machine::write_value(
  SizeKind size, const Addr& addr, slong_t value, int inc_addr)
{
  switch (addr.kind) {
  case AddrKind::ImmByte:
  case AddrKind::ImmWord:
  case AddrKind::ImmLong:
    raise_error("Not supported");
  case AddrKind::Ram: {
    G.io->write_signed(size, addr.addr + inc_addr, value);
  } break;
  case AddrKind::Reg: {
    write_register(size, addr.reg, value);
  } break;
  }
}

slong_t Machine::read_value(SizeKind size, const AddrMode& am, int inc_addr)
{
  auto addr = read_address(size, am);
  return read_value(size, addr, inc_addr);
}

void Machine::write_value(
  SizeKind size, const AddrMode& am, slong_t value, int inc_addr)
{
  auto addr = read_address(size, am);
  write_value(size, addr, value, inc_addr);
}

void Machine::push(SizeKind size, slong_t value)
{
  auto spv = sp() - size.num_bytes();
  set_sp(spv);
  write_value(size, Addr::ram(spv), value);
}

slong_t Machine::pop(SizeKind size)
{
  auto spv = sp();
  slong_t ret = read_value(size, Addr::ram(spv));
  set_sp(spv + size.num_bytes());
  return ret;
}

void Machine::set_sp(ulong_t v)
{
  write_register(SizeKind::Long, RegisterId::usp(), v);
}

ulong_t Machine::sp() const
{
  return read_register(SizeKind::Long, RegisterId::usp());
}

void Machine::print_registers() const
{
  for (int i = 0; i < 8; i++) {
    auto reg = RegisterId::data(i);
    auto value = read_register_quiet(SizeKind::Long, reg);
    if (_verbose) P("$={x}", reg, value);
  }
  for (int i = 0; i < 8; i++) {
    auto reg = RegisterId::addr(i);
    auto value = read_register_quiet(SizeKind::Long, reg);
    if (_verbose) P("$={x}", reg, value);
  }
}

slong_t Machine::read_register_quiet(SizeKind size, RegisterId id) const
{
  switch (id.kind) {
  case RegisterKind::Addr:
    return G.a.at(id.reg_id).get_s(size);
  case RegisterKind::Data:
    return G.d.at(id.reg_id).get_s(size);
  case RegisterKind::SR:
    return G.sr.to_int();
  }
}

void Machine::write_register_quiet(SizeKind size, RegisterId id, slong_t value)
{
  switch (id.kind) {
  case RegisterKind::Addr: {
    G.a.at(id.reg_id).set_s(size, value);
  } break;
  case RegisterKind::Data: {
    G.d.at(id.reg_id).set_s(size, value);
  } break;
  case RegisterKind::SR: {
    G.sr.set_from_int(value);
  } break;
  }
}

} // namespace heaven_ice
