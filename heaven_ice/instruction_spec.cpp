#include "instruction_spec.hpp"

#include <string>
#include <vector>

#include "instruction_field.hpp"

#include "bee/format.hpp"
#include "bee/format_vector.hpp"

namespace heaven_ice {
namespace {

static uword_t make_opcode_mask(const std::vector<InstructionField>& fields)
{
  uword_t mask = 0;
  for (auto&& field : fields) {
    int size = field.num_bits();
    mask <<= size;
    if (field.kind == FieldKind::BitPattern) { mask |= ((1 << size) - 1); }
  }
  return mask;
}

static uword_t make_masked_opcode(const std::vector<InstructionField>& fields)
{
  uword_t value = 0;
  for (auto&& field : fields) {
    int size = field.num_bits();
    if (field.kind == FieldKind::BitPattern) {
      for (auto b : field.bit_pattern) { value = (value << 1) | (b - '0'); }
    } else {
      value <<= size;
    }
  }
  return value;
}

} // namespace

InstructionSpec::InstructionSpec(
  InstEnum name, std::vector<InstructionField> fields)
    : name(name),
      fields(fields),
      opcode_mask(make_opcode_mask(fields)),
      masked_opcode(make_masked_opcode(fields))
{}

int InstructionSpec::size() const
{
  int size = 0;
  for (auto&& field : fields) { size += field.num_bits(); }
  return size;
}

bool InstructionSpec::match(uword_t opcode) const
{
  return (opcode & opcode_mask) == masked_opcode;
}

std::string InstructionSpec::to_string() const
{
  return F("$:$", name, fields);
}

bee::OrError<InstFields> InstructionSpec::parse_fields(uword_t opcode) const
{
  InstFields out(name);
  int bit = 16;
  for (auto&& f : fields) {
    bit -= f.num_bits();
    uword_t value = (opcode >> bit) & ((1u << f.num_bits()) - 1);
    switch (f.kind) {
    case FieldKind::BitPattern: {
    } break;
    case FieldKind::S1: {
      bail_assign(out._size, SizeKind::decode2(value));
    } break;
    case FieldKind::S2: {
      bail_assign(out._size, SizeKind::decode3(value));
    } break;
    case FieldKind::SM2: {
      bail_assign(out._size, SizeKind::decode_move(value));
    } break;
    case FieldKind::M1:
    case FieldKind::M2:
    case FieldKind::M3: {
      out._mode1 = value;
    } break;
    case FieldKind::An1: {
      out._reg1 = RegisterId::addr(value);
    } break;
    case FieldKind::An2: {
      out._reg2 = RegisterId::addr(value);
    } break;
    case FieldKind::Dn1: {
      out._reg1 = RegisterId::data(value);
    } break;
    case FieldKind::Dn2: {
      out._reg2 = RegisterId::data(value);
    } break;
    case FieldKind::Xn: {
      out._xn = value;
    } break;
    case FieldKind::D1: {
      out._dir = value;
    } break;
    case FieldKind::EA1: {
      out._ea1 = value;
    } break;
    case FieldKind::EA2: {
      out._ea2 = (value >> 3) | ((value & 0x7) << 3);
    } break;
    case FieldKind::Vector4:
    case FieldKind::Data8: {
      out._data = value;
    } break;
    case FieldKind::Data3: {
      out._data = value == 0 ? 8 : value;
    } break;
    case FieldKind::Cond: {
      out._cond = Condition::of_code(value);
    } break;
    case FieldKind::Disp: {
      out._disp = value;
    } break;
    }
  }
  return out;
}

} // namespace heaven_ice
