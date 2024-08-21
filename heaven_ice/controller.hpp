#pragma once

#include <memory>

#include "control_key.hpp"
#include "io_intf.hpp"

namespace heaven_ice {

struct Controller : public IOIntf {
 public:
  using ptr = std::shared_ptr<Controller>;

  virtual void key_down(int control, ControlKey key) = 0;
  virtual void key_up(int control, ControlKey key) = 0;

  static ptr create();
};

} // namespace heaven_ice
