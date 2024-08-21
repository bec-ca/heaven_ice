#include "disasm.hpp"

#include <limits>
#include <map>
#include <set>

#include "addr.hpp"
#include "addr_mode.hpp"
#include "binary.hpp"
#include "condition.hpp"
#include "dir.hpp"
#include "hex_view.hpp"
#include "inst_enum.hpp"
#include "instruction.hpp"
#include "memory.hpp"
#include "opcode_decoder.hpp"
#include "register_id.hpp"
#include "register_list.hpp"
#include "rom_reader.hpp"

#include "bee/format.hpp"
#include "bee/print.hpp"
#include "bee/string_util.hpp"

namespace heaven_ice {
namespace {

struct DisasmImpl final : public Disasm {
 public:
  static bee::OrError<ptr> create(const IOIntf::ptr& bud)
  {
    bail(
      opcode_decoder,
      OpcodeDecoder::create_from_file(
        bee::FilePath("heaven_ice/instructions.txt")));
    auto reader = std::make_unique<RomReader>(bud);

    auto ret = std::make_shared<DisasmImpl>();

    ret->_opcode_decoder = std::move(opcode_decoder);
    ret->_reader = std::move(reader);

    return ret;
  }

  virtual bee::OrError<Instruction> disasm_one(ulong_t addr) override
  {
    bail(
      inst,
      parse_one_internal(addr),
      "Failed to parse instruction at {x}",
      addr);
    inst.pc = addr;
    inst.bytes = _reader->pc() - addr;
    return inst;
  }

 private:
  bee::OrError<Instruction> parse_one_internal(ulong_t addr)
  {
    _reader->set_pc(addr);
    uword_t opcode = _reader->read_next_opcode();
    bail(inst, _opcode_decoder->decode(opcode));
    switch (inst.name) {
    case InstEnum::ANDI:
    case InstEnum::ORI:
    case InstEnum::EORI:
    case InstEnum::ADDI:
    case InstEnum::SUBI: {
      SizeKind size = inst.size();
      auto src = AddrMode::make_imm(size, _reader->read_pc(size));
      bail(dst, _reader->decode_address_mode(size, inst.ea1()));
      return Instruction{
        .name = inst.name, .size = size, .src = src, .dst = dst};

    } break;
    case InstEnum::TST: {
      SizeKind size = inst.size();
      bail(src, _reader->decode_address_mode(size, inst.ea1()));
      return Instruction{.name = inst.name, .size = size, .src = src};
    } break;
    case InstEnum::BCHG:
    case InstEnum::BCLR:
    case InstEnum::BSET:
    case InstEnum::BTST: {
      auto bit_addr = inst.has_reg1()
                      ? AddrMode::make_reg(inst.reg1())
                      : AddrMode::make_imm_word(_reader->read_sword_pc());

      // Size kind passed in is not important because only data addressing
      // modes are allowed.
      bail(dst, _reader->decode_address_mode(SizeKind::Word, inst.ea1()));
      auto size = [&]() -> SizeKind {
        switch (dst.kind) {
        case AddrModeKind::Reg:
          return SizeKind::Long;
        case AddrModeKind::AReg:
        case AddrModeKind::PostInc:
        case AddrModeKind::PreDec:
        case AddrModeKind::ImmAddrWord:
        case AddrModeKind::ImmAddrLong:
        case AddrModeKind::ALongDisp:
        case AddrModeKind::AXByteDisp:
          return SizeKind::Byte;
        case AddrModeKind::ImmByte:
        case AddrModeKind::ImmWord:
        case AddrModeKind::ImmLong:
          raise_error("Not allowed");
        }
      }();

      return Instruction{
        .name = inst.name, .size = size, .src = bit_addr, .dst = dst};
    } break;
    case InstEnum::NEG:
    case InstEnum::NOT:
    case InstEnum::CLR: {
      SizeKind size = inst.size();
      bail(dst, _reader->decode_address_mode(size, inst.ea1()));
      return Instruction{.name = inst.name, .size = size, .dst = dst};
    } break;
    case InstEnum::Bcc: {
      auto cond = inst.cond();
      slong_t disp = inst.disp();
      if (disp == 0) { disp = _reader->read_sword_pc(); }
      return Instruction{
        .name = inst.name,
        .cond = cond,
        .src = AddrMode::make_imm_addr(_reader->opcode_pc() + disp)};
    } break;
    case InstEnum::LEA: {
      auto out_reg = inst.reg1();
      SizeKind size = SizeKind::Long;

      bail(src, _reader->decode_address_mode(size, inst.ea1()));
      return Instruction{
        .name = inst.name,
        .size = size,
        .src = src,
        .dst = AddrMode::make_reg(out_reg)};
    } break;
    case InstEnum::MOVEQ: {
      auto reg = inst.reg1();
      return Instruction{
        .name = inst.name,
        .size = SizeKind::Long,
        .src = AddrMode::make_imm_byte(inst.data()),
        .dst = AddrMode::make_reg(reg),
      };
    } break;
    case InstEnum::MOVEM: {
      SizeKind size = inst.size();
      auto mem_dr = inst.dir();
      uword_t register_mask = _reader->read_pc(SizeKind::Word);
      bail(am, _reader->decode_address_mode(size, inst.ea1()));
      bool reverse = am.kind == AddrModeKind::PreDec;
      RegisterList register_list{.reverse = reverse, .mask = register_mask};
      std::optional<AddrMode> src;
      std::optional<AddrMode> dst;
      if (mem_dr == 1) {
        src = am;
      } else {
        dst = am;
      }
      return Instruction{
        .name = inst.name,
        .size = size,
        .src = src,
        .dst = dst,
        .register_list = register_list};
    } break;
    case InstEnum::DBcc: {
      auto cond = inst.cond();
      auto size = SizeKind::Word;
      auto dst = AddrMode::make_reg(inst.reg1());
      auto src = AddrMode::make_imm_addr(
        _reader->opcode_pc() + _reader->read_sword_pc());
      return Instruction{
        .name = inst.name, .size = size, .cond = cond, .src = src, .dst = dst};
    } break;
    case InstEnum::AND:
    case InstEnum::OR:
    case InstEnum::EOR:
    case InstEnum::SUB:
    case InstEnum::SUBA:
    case InstEnum::SUBQ:
    case InstEnum::ADD:
    case InstEnum::ADDQ:
    case InstEnum::ADDA: {
      SizeKind size = inst.size();
      bail(src_am, _reader->decode_address_mode(size, inst.ea1()));
      bool is_quick = inst.name.is_quick();
      auto dir = is_quick ? 1 : inst.has_dir() ? inst.dir() : 0;

      auto dst_am = inst.has_data() ? AddrMode::make_imm_word(inst.data())
                                    : AddrMode::make_reg(inst.reg1());

      if (dir == 1) { std::swap(dst_am, src_am); }

      return Instruction{
        .name = inst.name, .size = size, .src = src_am, .dst = dst_am};
    } break;
    case InstEnum::MOVE: {
      SizeKind size = inst.size();
      bail(src_am, _reader->decode_address_mode(size, inst.ea1()));
      bail(dst_am, _reader->decode_address_mode(size, inst.ea2()));
      return Instruction{
        .name = inst.name, .size = size, .src = src_am, .dst = dst_am};
    } break;
    case InstEnum::ANDI_to_SR:
    case InstEnum::ORI_to_SR: {
      auto src = AddrMode::make_imm_word(_reader->read_sword_pc());
      auto dst = AddrMode::make_reg(RegisterId::sr());
      return Instruction{
        .name = inst.name, .size = SizeKind::Word, .src = src, .dst = dst};
    } break;
    case InstEnum::MOVE_to_SR: {
      SizeKind size = SizeKind::Word;
      bail(src, _reader->decode_address_mode(size, inst.ea1()));
      return Instruction{
        .name = inst.name,
        .size = size,
        .src = src,
        .dst = AddrMode::make_reg(RegisterId::sr())};
    } break;
    case InstEnum::MOVE_from_SR: {
      SizeKind size = SizeKind::Word;
      bail(dst, _reader->decode_address_mode(size, inst.ea1()));
      return Instruction{
        .name = inst.name,
        .size = size,
        .src = AddrMode::make_reg(RegisterId::sr()),
        .dst = dst,
      };
    } break;
    case InstEnum::MOVE_USP: {
      auto dr = inst.dir();
      auto src = AddrMode::make_reg(inst.reg1());
      auto dst = AddrMode::make_reg(RegisterId::usp());
      if (dr == 1) { std::swap(src, dst); }

      return Instruction{
        .name = inst.name, .size = SizeKind::Long, .src = src, .dst = dst};
    } break;
    case InstEnum::BSR: {
      sword_t disp = inst.disp();
      if (disp == 0) { disp = _reader->read_sword_pc(); }
      auto src = AddrMode::make_imm_addr(_reader->opcode_pc() + disp);
      return Instruction{.name = inst.name, .src = src};
    } break;
    case InstEnum::ROR:
    case InstEnum::ROL:
    case InstEnum::ASR:
    case InstEnum::ASL:
    case InstEnum::LSR:
    case InstEnum::LSL: {
      auto size = inst.has_size() ? inst.size() : SizeKind(SizeKind::Word);
      auto bits_src = [&]() -> AddrMode {
        if (inst.has_data()) {
          return AddrMode::make_imm_byte(inst.data());
        } else if (inst.has_reg2()) {
          return AddrMode::make_reg(inst.reg2());
        } else {
          return AddrMode::make_imm_byte(1);
        }
      }();
      bail(target, [&]() -> bee::OrError<AddrMode> {
        if (inst.has_ea1()) {
          return _reader->decode_address_mode(size, inst.ea1());
        } else {
          return AddrMode::make_reg(inst.reg1());
        }
      }());
      return Instruction{
        .name = inst.name,
        .size = size,
        .src = bits_src,
        .dst = target,
      };
    } break;
    case InstEnum::SWAP: {
      SizeKind size = SizeKind::Long;
      auto reg = AddrMode::make_reg(inst.reg1());
      return Instruction{.name = inst.name, .size = size, .dst = reg};
    } break;
    case InstEnum::EXT: {
      auto reg = AddrMode::make_reg(inst.reg1());
      SizeKind size = inst.size();
      return Instruction{.name = inst.name, .size = size, .dst = reg};
    } break;
    case InstEnum::CMP:
    case InstEnum::CMPA: {
      SizeKind size = inst.size();
      bail(src, _reader->decode_address_mode(size, inst.ea1()));
      auto dst = AddrMode::make_reg(inst.reg1());
      return Instruction{
        .name = inst.name, .size = size, .src = src, .dst = dst};
    } break;
    case InstEnum::CMPI: {
      SizeKind size = inst.size();
      auto src = AddrMode::make_imm(size, _reader->read_pc(size));
      bail(dst, _reader->decode_address_mode(size, inst.ea1()));
      return Instruction{
        .name = inst.name, .size = size, .src = src, .dst = dst};
    } break;
    case InstEnum::JSR: {
      bail(src, _reader->decode_address_mode(SizeKind::Long, inst.ea1()));
      return Instruction{.name = inst.name, .src = src};
    } break;
    case InstEnum::JMP: {
      bail(addr, _reader->decode_address_mode(SizeKind::Long, inst.ea1()));
      return Instruction{.name = inst.name, .src = addr};
    } break;
    case InstEnum::DIVU:
    case InstEnum::DIVS:
    case InstEnum::MULU:
    case InstEnum::MULS: {
      SizeKind size = SizeKind::Word;
      bail(src, _reader->decode_address_mode(size, inst.ea1()));
      auto dst = AddrMode::make_reg(inst.reg1());
      return Instruction{
        .name = inst.name, .size = size, .src = src, .dst = dst};
    } break;
    case InstEnum::EXG: {
      auto src = AddrMode::make_reg(inst.reg1());
      auto dst = AddrMode::make_reg(inst.reg2());
      return Instruction{
        .name = inst.name, .size = SizeKind::Long, .src = src, .dst = dst};
    } break;
    case InstEnum::ABCD: {
      SizeKind size = SizeKind::Byte;
      auto to_am = [](const RegisterId& reg) -> AddrMode {
        switch (reg.kind) {
        case RegisterKind::Addr:
          return {.kind = AddrModeKind::PreDec, .reg = reg};
        case RegisterKind::SR:
        case RegisterKind::Data:
          return AddrMode::make_reg(reg);
        }
      };
      auto src = to_am(inst.reg1());
      auto dst = to_am(inst.reg2());
      return Instruction{
        .name = inst.name, .size = size, .src = src, .dst = dst};
    } break;
    case InstEnum::RTS:
    case InstEnum::RTE:
    case InstEnum::NOP: {
      return Instruction{.name = inst.name};
    } break;
    default:
      return EF("Not implemented: $", inst.name);
    }
  }

  OpcodeDecoder::ptr _opcode_decoder;
  RomReader::ptr _reader;
};

struct Program {
 public:
  std::map<ulong_t, Instruction> insts;
  std::map<ulong_t, std::string> labels;
};

bee::OrError<Program> disasm_all(const std::string& rom_content)
{
  auto rom = std::make_shared<Memory>(rom_content);
  bail(d, DisasmImpl::create(rom));

  std::map<ulong_t, Instruction> insts;

  auto is_code = [&insts](ulong_t pc) { return insts.find(pc) != insts.end(); };

  std::deque<ulong_t> queue;
  queue.push_back(0x0200);
  queue.push_back(0x4718);
  queue.push_back(0x5c88);

  queue.push_back(0x775e);
  queue.push_back(0x7810);
  queue.push_back(0x66ba);
  queue.push_back(0x9746);
  queue.push_back(0x97a6);
  queue.push_back(0x519e);
  queue.push_back(0x76b2);
  queue.push_back(0xa0d4);
  queue.push_back(0xa170);
  queue.push_back(0x66b2);
  queue.push_back(0x66b2);
  queue.push_back(0x8cd6);
  queue.push_back(0x8dbe);
  queue.push_back(0x8d2a);

  for (int i = 0x542; i < 0x5e2; i += 4) { queue.push_back(i); }
  for (int i = 0x67dc; i < 0x67ec; i += 2) { queue.push_back(i); }
  for (int i = 0x9ca; i < 0xa3a; i += 4) { queue.push_back(i); }
  for (int i = 0x66b2; i < 0x66c2; i += 2) { queue.push_back(i); }

  std::set<ulong_t> labels;

  auto create_am_label = [&labels](const std::optional<AddrMode>& am) {
    if (am.has_value()) {
      if (auto addr = am->addr_opt(); addr) { labels.insert(*addr); }
    }
  };

  while (!queue.empty()) {
    ulong_t pc = queue.front();
    queue.pop_front();
    while (true) {
      if (is_code(pc)) { break; }
      bail(inst, d->disasm_one(pc));
      insts.emplace(inst.pc, inst);
      create_am_label(inst.src);

      if (auto addr = inst.jump_addr(); addr) { queue.push_back(*addr); }
      if (inst.is_unconditional_jump()) { break; }
      pc += inst.bytes;
    }
  }

  std::map<ulong_t, std::string> label_addr_to_name;
  {
    for (auto& addr : labels) {
      label_addr_to_name.emplace(addr, F("L{x}", addr));
    }
  }

  return Program{
    .insts = std::move(insts), .labels = std::move(label_addr_to_name)};
}

} // namespace

bee::OrError<> Disasm::disasm_and_print(const std::string& rom)
{
  bail(p, disasm_all(rom));
  auto&& insts = p.insts;
  auto&& labels = p.labels;

  auto find_next_inst = [&insts](ulong_t pc) -> Instruction {
    if (auto it = insts.lower_bound(pc); it != insts.end()) {
      return it->second;
    } else {
      return {.name = InstEnum::ADD, .pc = std::numeric_limits<ulong_t>::max()};
    }
  };

  auto find_next_label_addr = [&labels](ulong_t pc) {
    if (auto it = labels.lower_bound(pc); it != labels.end()) {
      return it->first;
    } else {
      return std::numeric_limits<ulong_t>::max();
    }
  };

  ulong_t pc = 0;
  int last_block_kind = 1;
  while (pc < rom.size()) {
    if (auto it = labels.find(pc); it != labels.end()) {
      P("\n$:", it->second);
      last_block_kind = 1;
    }

    auto inst = find_next_inst(pc);
    auto label_addr = find_next_label_addr(pc + 1);
    if (inst.pc > pc) {
      auto next_pc = std::min(label_addr, inst.pc);
      if (last_block_kind != 1) { P(""); }
      HexView::print_hex(pc, rom.substr(pc, next_pc - pc));
      pc = next_pc;
      last_block_kind = 2;
    } else {
      if (last_block_kind == 2) { P(""); }
      P("{06x}: $", inst.pc, inst);
      if (inst.is_conditional_jump()) { P(""); }
      pc = inst.pc + inst.bytes;
      last_block_kind = 3;
    }
  }

  return bee::ok();
}

bee::OrError<Disasm::ptr> Disasm::create(const IOIntf::ptr& rom)
{
  return DisasmImpl::create(rom);
}

} // namespace heaven_ice
