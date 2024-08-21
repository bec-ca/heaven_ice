#include "emulate.hpp"

#include <cstdint>
#include <string>
#include <unordered_set>

#include "disasm.hpp"
#include "inst_enum.hpp"
#include "inst_impls.hpp"
#include "instruction.hpp"
#include "machine.hpp"
#include "magic_constants.hpp"
#include "save_state.hpp"

#include "bee/file_path.hpp"
#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "bee/or_error.hpp"
#include "bee/print.hpp"

namespace heaven_ice {
namespace {

constexpr int InstsPerFrame = 1000000 / 60;

const std::string sep("-----------------------------------");

template <class T> T op2(InstEnum inst, T dst, T src)
{
  switch (inst) {
  case InstEnum::ADD:
  case InstEnum::ADDA:
  case InstEnum::ADDI:
  case InstEnum::ADDQ:
    return ADD<T>(dst, src);
  case InstEnum::BSET:
    return BSET<T>(dst, src);
  case InstEnum::BCLR:
    return BCLR<T>(dst, src);
  case InstEnum::BCHG:
    return BCHG<T>(dst, src);
  case InstEnum::SUB:
  case InstEnum::SUBA:
  case InstEnum::SUBI:
  case InstEnum::SUBQ:
    return SUB<T>(dst, src);
  case InstEnum::AND:
  case InstEnum::ANDI:
  case InstEnum::ANDI_to_SR:
    return AND<T>(dst, src);
  case InstEnum::OR:
  case InstEnum::ORI:
  case InstEnum::ORI_to_SR:
    return OR<T>(dst, src);
  case InstEnum::EORI:
  case InstEnum::EOR:
    return EOR<T>(dst, src);
  case InstEnum::MULS:
    return MULS(dst, src);
  case InstEnum::MULU:
    return MULU(dst, src);
  case InstEnum::DIVS:
    return DIVS(dst, src);
  case InstEnum::DIVU:
    return DIVU(dst, src);
  case InstEnum::ROR:
    return ROR<T>(dst, src);
  case InstEnum::ROL:
    return ROL<T>(dst, src);
  case InstEnum::LSR:
    return LSR<T>(dst, src);
  case InstEnum::LSL:
    return LSL<T>(dst, src);
  case InstEnum::ASR:
    return ASR<T>(dst, src);
  case InstEnum::ASL:
    return ASL<T>(dst, src);
  case InstEnum::ABCD:
    return ABCD(dst, src);
  default:
    raise_error("BUG!");
  }
}

slong_t op2(InstEnum inst, SizeKind size, slong_t dst, slong_t src)
{
  switch (size) {
  case SizeKind::Byte:
    return op2<B>(inst, dst, src);
  case SizeKind::Word:
    return op2<W>(inst, dst, src);
  case SizeKind::Long:
    return op2<L>(inst, dst, src);
  }
}

template <class T> void cmp_op2(InstEnum inst, T dst, T src)
{
  switch (inst) {
  case InstEnum::CMP:
  case InstEnum::CMPA:
  case InstEnum::CMPI:
    CMP<T>(dst, src);
    break;
  case InstEnum::BTST:
    BTST<T>(dst, src);
    break;
  default:
    raise_error("BUG!");
  }
}

void cmp_op2(InstEnum inst, SizeKind size, slong_t dst, slong_t src)
{
  switch (size) {
  case SizeKind::Byte:
    cmp_op2<B>(inst, dst, src);
    break;
  case SizeKind::Word:
    cmp_op2<W>(inst, dst, src);
    break;
  case SizeKind::Long:
    cmp_op2<L>(inst, dst, src);
    break;
  }
}

template <class T> T op1(InstEnum inst, T dst)
{
  switch (inst) {
  case InstEnum::SWAP:
    return SWAP(dst);
  case InstEnum::EXT:
    return EXT<T>(dst);
  case InstEnum::NEG:
    return NEG<T>(dst);
  case InstEnum::NOT:
    return NOT<T>(dst);
  default:
    raise_error("BUG!");
  }
}

slong_t op1(InstEnum inst, SizeKind size, slong_t dst)
{
  switch (size) {
  case SizeKind::Byte:
    return op1<B>(inst, dst);
  case SizeKind::Word:
    return op1<W>(inst, dst);
  case SizeKind::Long:
    return op1<L>(inst, dst);
  }
}

void TSTS(SizeKind size, slong_t value)
{
  switch (size) {
  case SizeKind::Byte:
    TST<B>(value);
    break;
  case SizeKind::Word:
    TST<W>(value);
    break;
  case SizeKind::Long:
    TST<L>(value);
    break;
  }
}

} // namespace

bee::OrError<> Emulate::main(
  bool show_registers,
  bool verbose,
  const std::optional<uint64_t> max_instructions,
  const std::optional<bee::FilePath> load_state,
  const std::optional<bee::FilePath> save_state)
{
  bail(disasm, Disasm::create(G.io));
  std::vector<std::optional<Instruction>> instruction_cache;

  Machine machine(verbose);
  uint64_t instruction_count = 0;

  std::unordered_set<ulong_t> seen_jumps;
  auto maybe_log_jump = [&](ulong_t addr) {
    if (!seen_jumps.contains(addr)) {
      P("New jump addr: 0x{05x}", addr);
      seen_jumps.insert(addr);
    }
  };

  ulong_t pc = 0x200;
  bool is_interrupting = false;

  if (load_state.has_value()) {
    must(reader, bee::FileReader::open(*load_state));
    G.load_state(*reader);
    load_state_gen(pc, *reader);
    load_state_gen(is_interrupting, *reader);

    P("loaded state from file: $", *load_state);
  }

  while (true) {
    if (
      max_instructions.has_value() && instruction_count >= *max_instructions) {
      P("Maximum instructions reached");
      break;
    }
    if (verbose) P(sep);
    StatusRegister initial_sr = G.sr;
    instruction_count++;
    if (save_state && (instruction_count % (1 << 23)) == 1) {
      P("Saving state...");
      must(writer, bee::FileWriter::create(*save_state));
      G.save_state(*writer);
      save_state_gen(pc, *writer);
      save_state_gen(is_interrupting, *writer);
    }
    if (pc % 2 == 1) { return EF("PC cannot be odd: {x}", pc); }
    ulong_t inst_idx = pc / 2;
    if (inst_idx >= instruction_cache.size()) {
      instruction_cache.resize(
        std::max<size_t>(instruction_cache.size() * 2, inst_idx + 1));
      if (verbose)
        P("Instruction cache resized: $, $ bytes",
          instruction_cache.size(),
          instruction_cache.size() * sizeof(Instruction));
    }
    const auto& inst_opt = instruction_cache[inst_idx];
    if (!inst_opt.has_value()) {
      bail_assign(instruction_cache[inst_idx], disasm->disasm_one(pc));
    }
    const auto& inst = *inst_opt;
    if (verbose) P("{06x}: $", pc, inst);
    pc += inst.bytes;
    switch (inst.name) {
    case InstEnum::TST: {
      auto size = inst.size.value();
      auto value = machine.read_value(size, inst.src.value());
      TSTS(size, value);
    } break;
    case InstEnum::CLR: {
      auto size = inst.size.value();
      machine.write_value(size, inst.dst.value(), 0);
    } break;
    case InstEnum::Bcc: {
      auto cond = inst.cond.value();
      auto addr = machine.read_address(SizeKind::l(), inst.src.value());

      if (verbose) P("SR: $", G.sr);
      if (G.sr.check_condition(cond)) {
        if (verbose) P("Branch taken");
        pc = addr.get_ram_addr();
      }
    } break;
    case InstEnum::LEA: {
      auto size = inst.size.value();
      auto src_value = machine.read_address(size, inst.src.value());
      machine.write_value(size, inst.dst.value(), src_value.get_ram_addr());
    } break;
    case InstEnum::MOVEM: {
      auto size = inst.size.value();
      auto l = inst.register_list.value();
      int offset = 0;
      for (int i = 0; i < 16; i++) {
        if (!l.contains(i)) continue;
        auto r = l.reg(i);
        if (inst.src.has_value()) {
          int inc_addr =
            inst.src->is_inc_or_dec() ? 0 : offset * size.num_bytes();
          machine.write_register(
            SizeKind::l(),
            r,
            machine.read_value(size, inst.src.value(), inc_addr));
        } else {
          int inc_addr =
            inst.dst->is_inc_or_dec() ? 0 : offset * size.num_bytes();
          machine.write_value(
            size, inst.dst.value(), machine.read_register(size, r), inc_addr);
        }
        offset++;
      }
    } break;
    case InstEnum::DBcc: {
      auto size = inst.size.value();
      auto cond = inst.cond.value();
      auto dst = inst.dst.value();
      auto src_addr = machine.read_address(SizeKind::l(), inst.src.value());

      if (verbose) P("SR: $", G.sr);
      if (!G.sr.check_condition(cond)) {
        auto value = machine.read_value(size, dst) - 1;
        machine.write_value(size, dst, value);
        if (value != -1) {
          if (verbose) P("Branch taken");
          pc = src_addr.get_ram_addr();
        }
      }
    } break;
    case InstEnum::ABCD:
    case InstEnum::ADD:
    case InstEnum::ADDA:
    case InstEnum::ADDI:
    case InstEnum::ADDQ:
    case InstEnum::AND:
    case InstEnum::ANDI:
    case InstEnum::ASL:
    case InstEnum::ASR:
    case InstEnum::BSET:
    case InstEnum::BCLR:
    case InstEnum::BCHG:
    case InstEnum::EOR:
    case InstEnum::EORI:
    case InstEnum::LSL:
    case InstEnum::LSR:
    case InstEnum::OR:
    case InstEnum::ORI:
    case InstEnum::ORI_to_SR:
    case InstEnum::ANDI_to_SR:
    case InstEnum::ROL:
    case InstEnum::ROR:
    case InstEnum::SUB:
    case InstEnum::SUBA:
    case InstEnum::SUBI:
    case InstEnum::SUBQ: {
      auto src_size = inst.size.value();
      auto dst = inst.dst.value();
      auto dst_size = dst.is_addr_reg() ? SizeKind::l() : inst.size.value();
      auto src = inst.src.value();

      auto dst_addr = machine.read_address(dst_size, dst);
      auto src_addr = machine.read_address(src_size, src);

      auto result = op2(
        inst.name,
        dst_size,
        machine.read_value(dst_size, dst_addr),
        machine.read_value(src_size, src_addr));
      machine.write_value(dst_size, dst_addr, result);
    } break;
    case InstEnum::EXG: {
      auto size = inst.size.value();
      auto src = inst.src.value();
      auto dst = inst.dst.value();

      auto dst_addr = machine.read_address(size, dst);
      auto src_addr = machine.read_address(size, src);

      auto dst_value = machine.read_value(size, dst_addr);
      auto src_value = machine.read_value(size, src_addr);

      machine.write_value(size, dst_addr, src_value);
      machine.write_value(size, src_addr, dst_value);

    } break;
    case InstEnum::MULS:
    case InstEnum::MULU: {
      SizeKind size = SizeKind::w();
      SizeKind res_size = SizeKind::l();

      auto dst_addr = machine.read_address(size, inst.dst.value());
      auto src_addr = machine.read_address(size, inst.src.value());
      auto result = op2(
        inst.name,
        res_size,
        machine.read_value(size, dst_addr),
        machine.read_value(size, src_addr));
      machine.write_value(res_size, dst_addr, result);
    } break;
    case InstEnum::DIVS:
    case InstEnum::DIVU: {
      SizeKind src_size = SizeKind::w();
      SizeKind dst_size = SizeKind::l();

      auto dst_addr = machine.read_address(dst_size, inst.dst.value());
      auto src_addr = machine.read_address(src_size, inst.src.value());
      auto result = op2(
        inst.name,
        dst_size,
        machine.read_value(dst_size, dst_addr),
        machine.read_value(src_size, src_addr));
      machine.write_value(dst_size, dst_addr, result);
    } break;
    case InstEnum::MOVE_to_SR:
    case InstEnum::MOVE_from_SR:
    case InstEnum::MOVEQ:
    case InstEnum::MOVE:
    case InstEnum::MOVE_USP: {
      auto size = inst.size.value();
      auto src = inst.src.value();
      auto dst = inst.dst.value();

      auto value = machine.read_value(size, src);
      machine.write_value(size, dst, value);

      switch (inst.name) {
      case InstEnum::MOVE:
      case InstEnum::MOVEQ:
        TSTS(size, value);
        break;
      default:
        break;
      }
    } break;
    case InstEnum::JMP: {
      pc = machine.read_address(SizeKind::l(), inst.src.value()).get_ram_addr();
      maybe_log_jump(pc);
    } break;
    case InstEnum::BSR:
    case InstEnum::JSR: {
      machine.push(SizeKind::l(), pc);
      pc = machine.read_address(SizeKind::l(), inst.src.value()).get_ram_addr();
      if (inst.name == InstEnum::JSR) { maybe_log_jump(pc); }
    } break;
    case InstEnum::RTS: {
      pc = machine.pop(SizeKind::l());
    } break;
    case InstEnum::EXT:
    case InstEnum::SWAP:
    case InstEnum::NEG:
    case InstEnum::NOT: {
      auto size = inst.size.value();
      auto dst_addr = machine.read_address(size, inst.dst.value());
      ulong_t dst = machine.read_value(size, dst_addr);
      ulong_t new_value = op1(inst.name, size, dst);
      machine.write_value(size, dst_addr, new_value);
    } break;
    case InstEnum::BTST:
    case InstEnum::CMP:
    case InstEnum::CMPA:
    case InstEnum::CMPI: {
      auto size = inst.size.value();
      auto src_addr = machine.read_address(size, inst.src.value());
      auto dst_addr = machine.read_address(size, inst.dst.value());

      int64_t dst = machine.read_value(size, dst_addr);
      int64_t src = machine.read_value(size, src_addr);
      cmp_op2(inst.name, size, dst, src);
    } break;
    case InstEnum::NOP: {
    } break;
    case InstEnum::RTE: {
      G.sr.set_from_int(machine.pop(SizeKind::w()));
      pc = machine.pop(SizeKind::l());
      is_interrupting = false;
      G.vblank();
    } break;
    default:
      raise_error("Not implemented: $", inst.name);
    }
    if (verbose)
      if (G.sr != initial_sr) { P("SR: $", G.sr); }

    if (show_registers) { machine.print_registers(); }

    if (instruction_count % InstsPerFrame == 0) {
      if (verbose) P(sep);
      if (G.is_vblank_enabled() && !is_interrupting) {
        if (verbose) P("Interrupt: VBLANK");
        machine.push(SizeKind::l(), pc);
        machine.push(SizeKind::w(), G.sr.to_int());
        ulong_t handler =
          machine.read_value(SizeKind::l(), Addr::ram(VBLANK_INTERRUPT));
        pc = handler;
        is_interrupting = true;
      } else {
        if (verbose) P("VBLANK skipped");
      }
    }
  }

  return bee::ok();
}

} // namespace heaven_ice
