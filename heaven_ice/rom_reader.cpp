#include "rom_reader.hpp"

#include "bit_manip.hpp"
#include "register_id.hpp"

namespace heaven_ice {

RomReader::RomReader(const IOIntf::ptr& rom) : _rom(rom) {}

ulong_t RomReader::pc() const { return _pc; }
ulong_t RomReader::opcode_pc() const { return _opcode_pc; }

sbyte_t RomReader::read_sbyte_pc()
{
  auto ret = _rom->w(_pc);
  _pc += 2;
  return ret;
}

sword_t RomReader::read_sword_pc()
{
  auto ret = _rom->w(_pc);
  _pc += 2;
  return ret;
}
slong_t RomReader::read_slong_pc()
{
  auto ret = _rom->l(_pc);
  _pc += 4;
  return ret;
}

uword_t RomReader::read_next_opcode()
{
  uword_t ret = read_sword_pc();
  _opcode_pc = _pc;
  return ret;
}

slong_t RomReader::read_pc(SizeKind size)
{
  switch (size) {
  case SizeKind::Byte:
    return read_sbyte_pc();
  default:
    auto ret = read_signed(size, _pc);
    _pc += size.num_bytes();
    return ret;
  }
}

slong_t RomReader::read_signed(SizeKind size, ulong_t addr)
{
  return _rom->read_signed(size, addr);
}

bee::OrError<AddrMode> RomReader::decode_address_mode(
  SizeKind size, uword_t code)
{
  auto mode = (code >> 3) & 0x7;
  auto reg = code & 0x7;
  switch (mode) {
  case 0:
    return AddrMode::make_reg(RegisterId::data(reg));
  case 1:
    return AddrMode::make_reg(RegisterId::addr(reg));
  case 2:
    return AddrMode{.kind = AddrModeKind::AReg, .reg = RegisterId::addr(reg)};
  case 3:
    return AddrMode{
      .kind = AddrModeKind::PostInc, .reg = RegisterId::addr(reg)};
  case 4:
    return AddrMode{.kind = AddrModeKind::PreDec, .reg = RegisterId::addr(reg)};
  case 5: {
    auto im = read_sword_pc();
    return AddrMode{
      .kind = AddrModeKind::ALongDisp,
      .imm = im,
      .reg = RegisterId::addr(reg),
      .idx_size = SizeKind::l(),
    };
  }
  case 6: {
    BW w = read_sword_pc();
    sbyte_t disp = w.get(0, 8);
    bail(idx_size, SizeKind::decode2(w.get(11, 1)));
    int reg_id = w.get(12, 3);
    auto xreg =
      w.get(15, 1) == 0 ? RegisterId::data(reg_id) : RegisterId::addr(reg_id);
    return AddrMode{
      .kind = AddrModeKind::AXByteDisp,
      .imm = disp,
      .reg = RegisterId::addr(reg),
      .reg2 = xreg,
      .idx_size = idx_size,
    };
  }
  case 7:
    switch (reg) {
    case 0:
      return AddrMode{
        .kind = AddrModeKind::ImmAddrWord, .imm = read_sword_pc()};
    case 1:
      return AddrMode::make_imm_addr(read_slong_pc());
    case 2:
      return AddrMode::make_imm_addr(_opcode_pc + read_sword_pc());
    case 3: {
      BW w = read_sword_pc();
      sbyte_t disp = w.get(0, 8);
      bail(idx_size, SizeKind::decode2(w.get(11, 1)));
      int reg_id = w.get(12, 3);
      auto xreg =
        w.get(15, 1) == 0 ? RegisterId::data(reg_id) : RegisterId::addr(reg_id);
      return AddrMode{
        .kind = AddrModeKind::ALongDisp,
        .imm = slong_t(_opcode_pc + disp),
        .reg = xreg,
        .idx_size = idx_size,
      };
    }
    case 4:
      switch (size) {
      case SizeKind::Byte:
        return AddrMode{.kind = AddrModeKind::ImmByte, .imm = read_sbyte_pc()};
      case SizeKind::Word:
        return AddrMode::make_imm_word(read_sword_pc());
      case SizeKind::Long:
        return AddrMode{.kind = AddrModeKind::ImmLong, .imm = read_slong_pc()};
      }
    }
  }
  return EF(
    "Unsupported instruction mode*reg combination, mode:$ reg:$ ", mode, reg);
}

void RomReader::set_pc(ulong_t pc) { _pc = pc; }

} // namespace heaven_ice
