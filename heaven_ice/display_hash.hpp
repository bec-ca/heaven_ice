#pragma once

#include "heaven_ice/display_intf.hpp"

namespace heaven_ice {

struct DisplayHash {
  static DisplayIntf::ptr create();
};

} // namespace heaven_ice
