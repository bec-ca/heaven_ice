#pragma once

#include <string>

#include "register_id.hpp"
#include "types.hpp"

namespace heaven_ice {

struct RegisterList {
 public:
  bool reverse;
  uword_t mask;

  std::string to_string() const;

  bool contains(int idx) const;
  RegisterId reg(int idx) const;
};
} // namespace heaven_ice
