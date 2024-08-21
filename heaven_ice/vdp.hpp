#pragma once

#include <memory>

#include "io_intf.hpp"
#include "types.hpp"

#include "pixel/image.hpp"

namespace heaven_ice {

struct VDP : public IOIntf {
 public:
  using ptr = std::shared_ptr<VDP>;

  virtual ~VDP();

  virtual void dump_sprites(int idx) const = 0;

  virtual void dump_memory(int dump_idx) const = 0;

  virtual bool vblank_enabled() const = 0;

  virtual pixel::Image render() const = 0;

  virtual void set_bus(const IOIntf::ptr& bus) = 0;

  static ptr create(bool verbose);
};

} // namespace heaven_ice
