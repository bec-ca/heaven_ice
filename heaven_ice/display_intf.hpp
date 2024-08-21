#pragma once

#include <vector>

#include "input_event.hpp"

#include "pixel/image.hpp"
#include "sdl/event.hpp"

namespace heaven_ice {

struct DisplayIntf {
 public:
  using ptr = std::shared_ptr<DisplayIntf>;

  ~DisplayIntf();

  virtual void update(const pixel::Image& img) = 0;

  virtual std::vector<sdl::Event> get_events() = 0;
};

} // namespace heaven_ice
