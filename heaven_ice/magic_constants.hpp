#pragma once

#include <map>
#include <string>
#include <type_traits>

#include "types.hpp"

namespace heaven_ice {

constexpr ulong_t VBLANK_FLAG = 0xff0002;

constexpr ulong_t VDP_BEGIN = 0xc00000;
constexpr ulong_t VDP_END = 0xc00010;

constexpr ulong_t RAM_BEGIN = 0xff0000;
constexpr ulong_t RAM_END = 0x1000000;

constexpr ulong_t ROM_BEGIN = 0x0;
constexpr ulong_t ROM_END = 0x400000;

constexpr ulong_t VDP_DATA1 = 0xc00000;
constexpr ulong_t VDP_DATA2 = 0xc00002;
constexpr ulong_t VDP_CTRL1 = 0xc00004;
constexpr ulong_t VDP_CTRL2 = 0xc00006;

constexpr ulong_t Z80_BUS_REQUEST = 0xa11100;
constexpr ulong_t Z80_RESET = 0xa11200;

constexpr ulong_t Z80_RAM_BEGIN = 0xa00000;
constexpr ulong_t Z80_RAM_END = 0xa10000;

constexpr ulong_t Z80_IMAGE_BEGIN = 0x13000;

constexpr ulong_t VBLANK_INTERRUPT = 0x78;

constexpr ulong_t PSG_CONTROL = 0xc00011;

constexpr ulong_t TMSS = 0xa14000;

constexpr ulong_t VERSION_REGISTER = 0xa10001;

constexpr ulong_t SEGA = 0x53454741;

constexpr ulong_t INITIAL_DATA = 0x29c;

constexpr ulong_t CONTROLLER1_CTRL1 = 0xa10008;
constexpr ulong_t CONTROLLER1_CTRL2 = 0xa10009;
constexpr ulong_t CONTROLLER2_CTRL1 = 0xa1000a;
constexpr ulong_t CONTROLLER2_CTRL2 = 0xa1000b;

constexpr ulong_t CONTROLLER1_DATA1 = 0xa10002;
constexpr ulong_t CONTROLLER1_DATA2 = 0xa10003;
constexpr ulong_t CONTROLLER2_DATA1 = 0xa10004;
constexpr ulong_t CONTROLLER2_DATA2 = 0xa10005;

constexpr ulong_t EXPANSION_PORT_CTRL = 0xa1000c;

constexpr ulong_t CONTROLLER_STATE1 = 0xff0070;
constexpr ulong_t CONTROLLER_STATE2 = 0xff0072;

constexpr ulong_t VSCROLL_FG = 0xff0030;
constexpr ulong_t HSCROLL_FG = 0xff0032;
constexpr ulong_t VSCROLL_BG = 0xff0034;
constexpr ulong_t HSCROLL_BG = 0xff0036;

constexpr ulong_t SPRITE_TABLE = 0xff259a;

constexpr ulong_t Z80_SYNC_FLAG = 0xa01d00;

constexpr ulong_t DMA_QUEUE_TABLE = 0xff281a;

constexpr ulong_t SOME_STATE_CONTROL = 0xff0006;

constexpr ulong_t WRITE_TO_CRAM = 0xc0000000;
constexpr ulong_t WRITE_TO_VSRAM = 0x40000010;

constexpr ulong_t SOME_STATE_COUNTER = 0xff0076;
constexpr ulong_t SOME_OTHER_FUCKING_COUNTER = 0xff2a9a;

constexpr ulong_t CTRL_BEGIN = 0xa10002;
constexpr ulong_t CTRL_END = 0xa10020;

constexpr int NUM_VDP_REGS = 24;

const char* get_constant_name(ulong_t value);

template <class T> std::string cpp_hex(T value)
{
  using U = std::make_unsigned_t<T>;
  if (value >= 0 && value < 16) {
    return F(value);
  } else if (auto name = get_constant_name(value); name) {
    return name;
  } else if (value < 0) {
    return F("-$", U(-value));
  } else {
    return F("0x{x}", U(value));
  }
}

template <class T> std::string signed_cpp_hex(T value)
{
  using U = std::make_unsigned_t<T>;
  if (value < 0) {
    return F("- $", cpp_hex<U>(-value));
  } else {
    return F("+ $", cpp_hex<U>(value));
  }
}

template <class T> std::string simple_hex(T value)
{
  using U = std::make_unsigned_t<T>;
  if (value >= 0 && value < 10) {
    return F(value);
  } else if (auto name = get_constant_name(value); name) {
    return name;
  } else {
    return F("{x}", U(value));
  }
}

template <class T> std::string signed_simple_hex(T value)
{
  using U = std::make_unsigned_t<T>;
  if (value < 0) {
    return F("-$", simple_hex<U>(-value));
  } else {
    return F("+$", simple_hex<U>(value));
  }
}

} // namespace heaven_ice
