#pragma once

#include <memory>
#include <string>

#include "addr_mode.hpp"
#include "io_intf.hpp"
#include "size_kind.hpp"
#include "types.hpp"

#include "bee/or_error.hpp"

namespace heaven_ice {

struct RomReader {
 public:
  using ptr = std::unique_ptr<RomReader>;

  RomReader(const IOIntf::ptr& rom);
  ulong_t pc() const;
  ulong_t opcode_pc() const;

  sbyte_t read_sbyte_pc();
  sword_t read_sword_pc();
  slong_t read_slong_pc();

  uword_t read_next_opcode();

  slong_t read_pc(SizeKind size);
  slong_t read_signed(SizeKind size, ulong_t addr);

  bee::OrError<AddrMode> decode_address_mode(SizeKind size, uword_t code);
  void set_pc(ulong_t pc);

  ulong_t size() const;

 private:
  IOIntf::ptr _rom;
  ulong_t _pc = 0x200;
  ulong_t _opcode_pc;
};

} // namespace heaven_ice
