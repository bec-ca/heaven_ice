#include "display_hash.hpp"

#include "bee/print.hpp"
#include "bee/simple_checksum.hpp"

namespace heaven_ice {
namespace {

struct DisplayHashImpl final : public DisplayIntf {
  DisplayHashImpl() {}
  virtual ~DisplayHashImpl() {}

  void update(const pixel::Image& img) override
  {
    bee::SimpleChecksum cs;
    cs.add_string(reinterpret_cast<const char*>(img.data()), img.data_size());
    P(cs.hex());
  }

  std::vector<sdl::Event> get_events() override { return {}; }
};

} // namespace

DisplayIntf::ptr DisplayHash::create()
{
  return std::make_shared<DisplayHashImpl>();
}

} // namespace heaven_ice
