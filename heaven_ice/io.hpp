#pragma once

#include <exception>
#include <memory>

#include "io_intf.hpp"
#include "types.hpp"

namespace heaven_ice {

struct IO final : public IOIntf {
 public:
  using ptr = std::shared_ptr<IO>;

  IO(
    bool verbose,
    const IOIntf::ptr& ram,
    const IOIntf::ptr& rom,
    const IOIntf::ptr& vdp,
    const IOIntf::ptr& controller);
  ~IO();

  void save_state(bee::Writer& writer) override;
  void load_state(bee::Reader& reader) override;

 private:
  template <class T> T _read(ulong_t addr);
  template <class T> void _write(ulong_t addr, T v);

  ubyte_t _b(ulong_t addr) override;
  uword_t _w(ulong_t addr) override;
  ulong_t _l(ulong_t addr) override;

  void _b(ulong_t addr, ubyte_t v) override;
  void _w(ulong_t addr, uword_t v) override;
  void _l(ulong_t addr, ulong_t v) override;

  IOIntf::ptr _ram;
  IOIntf::ptr _rom;
  IOIntf::ptr _vdp;
  IOIntf::ptr _controller;

  bool _verbose;
};

} // namespace heaven_ice
