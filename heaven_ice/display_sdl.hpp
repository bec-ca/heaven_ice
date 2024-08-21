#pragma once

#include "controller.hpp"
#include "display_intf.hpp"

#include "bee/or_error.hpp"

namespace heaven_ice {

struct DisplaySDL {
  static bee::OrError<DisplayIntf::ptr> create(double scale);
};

} // namespace heaven_ice
