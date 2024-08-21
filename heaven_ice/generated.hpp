#pragma once

#include <string>

#include "generated_intf.hpp"
#include "manual_functions.hpp"

namespace heaven_ice {

struct Generated {
  static GeneratedIntf::ptr create(bool verbose, const ManualFunctions::ptr& m);
};

} // namespace heaven_ice
