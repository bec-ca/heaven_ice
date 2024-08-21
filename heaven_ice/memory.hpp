#pragma once

#include <array>
#include <string>
#include <vector>

#include "io_intf.hpp"
#include "size_kind.hpp"
#include "types.hpp"

namespace heaven_ice {

struct Memory final : public IOIntf {
 public:
  Memory(size_t size);
  Memory(const std::string& content);
  virtual ~Memory();

  void load_rom(const std::string& rom_content);

  size_t size() const;

  void save_state(bee::Writer&) override;
  void load_state(bee::Reader&) override;

 private:
  ubyte_t _b(ulong_t addr) override;
  uword_t _w(ulong_t addr) override;
  ulong_t _l(ulong_t addr) override;

  void _b(ulong_t addr, ubyte_t v) override;
  void _w(ulong_t addr, uword_t v) override;
  void _l(ulong_t addr, ulong_t v) override;

  std::vector<ubyte_t> _mem;
};

} // namespace heaven_ice
