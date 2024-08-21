#pragma once

#include <memory>

#include "types.hpp"

namespace heaven_ice {

struct GeneratedIntf {
 public:
  using ptr = std::shared_ptr<GeneratedIntf>;

  virtual void run() = 0;
  virtual void jump_map(ulong_t addr) = 0;
  virtual void vblank_int() = 0;

  virtual void F5e2() = 0;

  virtual void F127e() = 0;
  virtual void F6888() = 0;
};

} // namespace heaven_ice
