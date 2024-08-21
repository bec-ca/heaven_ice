#pragma once

#include "heaven_ice/display_intf.hpp"
namespace heaven_ice {

struct DisplayFfmpeg {
  static DisplayIntf::ptr create(int scale);
};

} // namespace heaven_ice
