#pragma once

#include <vector>

#include "instruction_spec.hpp"

#include "bee/file_path.hpp"

namespace heaven_ice {

struct OpcodeDecoder {
 public:
  using ptr = std::unique_ptr<OpcodeDecoder>;
  OpcodeDecoder(const std::vector<InstructionSpec>& insts);

  static bee::OrError<ptr> create_from_file(const bee::FilePath& filepath);

  bee::OrError<InstFields> decode(uword_t opcode) const;

 private:
  std::vector<InstructionSpec> _insts;
};

} // namespace heaven_ice
