#pragma once

#include <string>

#include "types.hpp"

namespace heaven_ice {

struct HexView {
  static void print_hex(ulong_t start_address, const std::string& content);
};

} // namespace heaven_ice
