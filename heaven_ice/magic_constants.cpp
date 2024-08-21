#include "magic_constants.hpp"

namespace heaven_ice {
namespace {

#define _(v) {v, #v}

const std::map<ulong_t, const char*> constant_names = {
  _(VBLANK_FLAG),
  _(VDP_DATA1),
  _(VDP_DATA2),
  _(VDP_CTRL1),
  _(VDP_CTRL2),
  _(Z80_BUS_REQUEST),
  _(Z80_RESET),
  _(Z80_RAM_BEGIN),
  _(Z80_IMAGE_BEGIN),
  _(TMSS),
  _(PSG_CONTROL),
  _(VERSION_REGISTER),
  _(SEGA),
  _(RAM_BEGIN),
  _(RAM_END),
  _(INITIAL_DATA),

  _(CONTROLLER1_CTRL1),
  _(CONTROLLER1_CTRL2),
  _(CONTROLLER2_CTRL1),
  _(CONTROLLER2_CTRL2),

  _(CONTROLLER1_DATA1),
  _(CONTROLLER1_DATA2),
  _(CONTROLLER2_DATA1),
  _(CONTROLLER2_DATA2),

  _(CONTROLLER_STATE1),
  _(CONTROLLER_STATE2),

  _(EXPANSION_PORT_CTRL),

  _(VSCROLL_FG),
  _(HSCROLL_FG),
  _(VSCROLL_BG),
  _(HSCROLL_BG),

  _(SPRITE_TABLE),

  _(Z80_SYNC_FLAG),

  _(DMA_QUEUE_TABLE),

  _(SOME_STATE_CONTROL),
  _(WRITE_TO_CRAM),
  _(WRITE_TO_VSRAM),

  _(SOME_STATE_COUNTER),
  _(SOME_OTHER_FUCKING_COUNTER),
};

} // namespace

const char* get_constant_name(ulong_t value)
{
  if (auto it = constant_names.find(value); it != constant_names.end()) {
    return it->second;
  }
  return nullptr;
}

} // namespace heaven_ice
