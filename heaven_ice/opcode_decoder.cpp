#include "opcode_decoder.hpp"

#include "parse_spec.hpp"

namespace heaven_ice {

OpcodeDecoder::OpcodeDecoder(const std::vector<InstructionSpec>& insts)
    : _insts(insts)
{}

bee::OrError<OpcodeDecoder::ptr> OpcodeDecoder::create_from_file(
  const bee::FilePath& filepath)
{
  bail(insts, ParseSpec::parse_spec(filepath));
  return std::make_unique<OpcodeDecoder>(std::move(insts));
}

bee::OrError<InstFields> OpcodeDecoder::decode(uword_t opcode) const
{
  for (auto&& inst : _insts) {
    if ((opcode & inst.opcode_mask) == inst.masked_opcode) {
      bail(fields, inst.parse_fields(opcode), inst.name);
      return fields;
    }
  }
  return EF("Invalid opcode: $", opcode);
}

} // namespace heaven_ice
