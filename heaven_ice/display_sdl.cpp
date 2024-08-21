#include "display_sdl.hpp"

#include "exceptions.hpp"

#include "bee/file_path.hpp"
#include "bee/format.hpp"
#include "bee/or_error.hpp"
#include "bee/print.hpp"
#include "heaven_ice/controller.hpp"
#include "heaven_ice/input_event.hpp"
#include "sdl/event.hpp"
#include "sdl/renderer.hpp"
#include "sdl/sdl_context.hpp"
#include "sdl/window.hpp"

namespace heaven_ice {
namespace {

struct DisplaySDLImpl final : public DisplayIntf {
  virtual ~DisplaySDLImpl() {}

  void update(const pixel::Image& img) override
  {
    must_unit(_ren->fill_all(img));
    _ren->present();
  }

  static bee::OrError<ptr> create(double scale)
  {
    bail(ctx, sdl::SDLContext::create());
    bail(
      win, sdl::Window::create(*ctx, "HeavenIce", {320 * scale, 224 * scale}));
    bail(ren, sdl::Renderer::create(*win, {}));

    auto ret = std::make_shared<DisplaySDLImpl>(
      std::move(ctx), std::move(win), std::move(ren));

    return ret;
  }

  DisplaySDLImpl(
    sdl::SDLContext::ptr&& ctx,
    sdl::Window::ptr&& win,
    const sdl::Renderer::ptr&& ren)
      : _ctx(std::move(ctx)), _win(std::move(win)), _ren(std::move(ren))
  {}

  std::vector<sdl::Event> get_events() override
  {
    std::vector<sdl::Event> evs;
    while (true) {
      must(event, _ctx->poll_event());
      if (!event.has_value()) { break; }
      evs.push_back(*event);
    }
    return evs;
  }

 private:
  sdl::SDLContext::ptr _ctx;
  sdl::Window::ptr _win;
  sdl::Renderer::ptr _ren;
};

} // namespace

bee::OrError<DisplayIntf::ptr> DisplaySDL::create(double scale)
{
  return DisplaySDLImpl::create(scale);
}

} // namespace heaven_ice
