#include "display_pnm.hpp"

#include "bee/file_path.hpp"
#include "bee/filesystem.hpp"
#include "bee/format.hpp"

namespace heaven_ice {
namespace {

const bee::FilePath OutputDir("screenshots");

struct DisplayPnmImpl final : public DisplayIntf {
  DisplayPnmImpl() { must_unit(bee::FileSystem::mkdirs(OutputDir)); }
  virtual ~DisplayPnmImpl() {}

  void update(const pixel::Image& img) override
  {
    _counter++;
    img.save_pnm(OutputDir / bee::FilePath(F("screenshot_{06}.pnm", _counter)));
  }

  std::vector<sdl::Event> get_events() override { return {}; }

  int _counter = 0;
};

} // namespace

DisplayIntf::ptr DisplayPnm::create()
{
  return std::make_shared<DisplayPnmImpl>();
}

} // namespace heaven_ice
