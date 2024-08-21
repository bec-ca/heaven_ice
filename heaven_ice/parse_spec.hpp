#pragma once

#include "instruction_spec.hpp"

#include "bee/file_path.hpp"
#include "command/cmd.hpp"

namespace heaven_ice {

struct ParseSpec {
  static command::Cmd cmd();

  static bee::OrError<std::vector<InstructionSpec>> parse_spec(
    const bee::FilePath& filepath);
};

} // namespace heaven_ice
