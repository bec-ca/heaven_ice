#pragma once

#include "bee/file_path.hpp"
#include "bee/or_error.hpp"

namespace heaven_ice {

struct Emulate {
  static bee::OrError<> main(
    bool show_registers,
    bool verbose,
    const std::optional<uint64_t> max_instructions,
    const std::optional<bee::FilePath> load_state,
    const std::optional<bee::FilePath> save_state);
};

} // namespace heaven_ice
