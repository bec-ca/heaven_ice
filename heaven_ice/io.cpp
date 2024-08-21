#include "io.hpp"

#include "magic_constants.hpp"

#include "bee/print.hpp"

namespace heaven_ice {
namespace {

constexpr ulong_t ADDR_MASK = 0xffffff;

inline bool in_ranges(ulong_t addr, ulong_t begin, ulong_t size)
{
  return addr >= begin && addr < begin + size;
}

inline bool in_range(ulong_t addr, ulong_t begin, ulong_t end)
{
  return addr >= begin && addr < end;
}

} // namespace

IO::IO(
  bool verbose,
  const IOIntf::ptr& ram,
  const IOIntf::ptr& rom,
  const IOIntf::ptr& vdp,
  const IOIntf::ptr& controller)
    : _ram(ram),
      _rom(rom),
      _vdp(vdp),
      _controller(controller),
      _verbose(verbose)
{}

IO::~IO() {}

template <class T> T IO::_read(ulong_t addr_orig)
{
  auto ret = [&]() -> T {
    ulong_t addr = addr_orig & ADDR_MASK;
    if (in_ranges(addr, Z80_BUS_REQUEST, 2)) {
      return 0;
    } else if (in_range(addr, VDP_BEGIN, VDP_END)) {
      return _vdp->read<T>(addr);
    } else if (addr == VERSION_REGISTER) {
      return 0x81;
    } else if (in_range(addr, CTRL_BEGIN, CTRL_END)) {
      return _controller->read<T>(addr);
    } else if (addr < ROM_END) {
      return _rom->read<T>(addr);
    } else if (in_range(addr, RAM_BEGIN, RAM_END)) {
      return _ram->read<T>(addr - RAM_BEGIN);
    } else if (in_range(addr, Z80_RAM_BEGIN, Z80_RAM_END)) {
      // Z80 ram, ignore
      return 0;
    } else {
      raise_error("Unsupported read address: {x}", addr);
    }
  }();
  if (_verbose)
    P("({06x}).$ -> #{x}", addr_orig, SizeKind::of_type<T>().to_string(), ret);
  return ret;
}

template <class T> void IO::_write(ulong_t addr, T v)
{
  if (_verbose)
    P("({06x}).$ <- #{x}", addr, SizeKind::of_type<T>().to_string(), v);
  addr &= ADDR_MASK;
  if (in_range(addr, VDP_BEGIN, VDP_END)) {
    _vdp->write<T>(addr, v);
  } else if (in_range(addr, RAM_BEGIN, RAM_END)) {
    _ram->write<T>(addr - RAM_BEGIN, v);
  } else if (in_range(addr, CTRL_BEGIN, CTRL_END)) {
    _controller->write<T>(addr, v);
  } else if (addr == TMSS) {
    if (v != SEGA) { raise_error("Wrong value for copy protection: {x}", v); }
  } else if (addr == Z80_BUS_REQUEST || addr == Z80_RESET) {
    // Z80 controls, ignore
  } else if (in_range(addr, Z80_RAM_BEGIN, Z80_RAM_END)) {
    // Z80 ram, ignore
  } else if (addr == PSG_CONTROL) {
    // PSG chip control, ignore
  } else {
    raise_error("Unsupported write address: {x}", addr);
  }
}

ubyte_t IO::_b(ulong_t addr) { return _read<ubyte_t>(addr); }
uword_t IO::_w(ulong_t addr) { return _read<uword_t>(addr); }
ulong_t IO::_l(ulong_t addr) { return _read<ulong_t>(addr); }

void IO::_b(ulong_t addr, ubyte_t v) { _write<ubyte_t>(addr, v); }
void IO::_w(ulong_t addr, uword_t v) { _write<uword_t>(addr, v); }
void IO::_l(ulong_t addr, ulong_t v) { _write<ulong_t>(addr, v); }

void IO::save_state(bee::Writer& writer)
{
  _ram->save_state(writer);
  _vdp->save_state(writer);
  _controller->save_state(writer);
}

void IO::load_state(bee::Reader& reader)
{
  _ram->load_state(reader);
  _vdp->load_state(reader);
  _controller->load_state(reader);
}

} // namespace heaven_ice
