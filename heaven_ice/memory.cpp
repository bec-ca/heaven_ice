#include "memory.hpp"

#include "magic_constants.hpp"

namespace heaven_ice {

Memory::Memory(size_t size) : _mem(size, 0) {}
Memory::Memory(const std::string& content) { load_rom(content); }

Memory::~Memory() {}

void Memory::load_rom(const std::string& rom_content)
{
  _mem.resize(rom_content.size());
  for (ulong_t i = 0; i < rom_content.size(); i++) { _mem[i] = rom_content[i]; }
}

ubyte_t Memory::_b(ulong_t addr)
{
  if (addr >= size()) {
    raise_error("Memory access out of bounds: {x} >= {x}", addr, size());
  }
  return _mem.at(addr);
}

uword_t Memory::_w(ulong_t addr)
{
  return (uword_t(_b(addr)) << 8) | uword_t(_b(addr + 1));
}

ulong_t Memory::_l(ulong_t addr)
{
  return (ulong_t(_w(addr)) << 16) | ulong_t(_w(addr + 2));
}

void Memory::_b(ulong_t addr, ubyte_t v)
{
  if (addr >= size()) {
    raise_error("Memory access out of bounds: {x} >= {x}", addr, size());
  }
  _mem.at(addr) = v;
}

void Memory::_w(ulong_t addr, uword_t v)
{
  _b(addr, v >> 8);
  _b(addr + 1, v);
}

void Memory::_l(ulong_t addr, ulong_t v)
{
  _w(addr, v >> 16);
  _w(addr + 2, v);
}

size_t Memory::size() const { return _mem.size(); }

void Memory::save_state(bee::Writer& writer)
{
  must_unit(
    writer.write(reinterpret_cast<const std::byte*>(_mem.data()), _mem.size()));
}

void Memory::load_state(bee::Reader& reader)
{
  must_unit(
    reader.read(reinterpret_cast<std::byte*>(_mem.data()), _mem.size()));
}

} // namespace heaven_ice
