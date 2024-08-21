#pragma once

#include <memory>

#include "size_kind.hpp"
#include "types.hpp"

#include "bee/reader.hpp"
#include "bee/writer.hpp"

namespace heaven_ice {

struct IOIntf {
 public:
  using ptr = std::shared_ptr<IOIntf>;

  virtual ~IOIntf();

  template <class T> void write(ulong_t addr, T v);
  template <> void write<sbyte_t>(ulong_t addr, sbyte_t v) { _b(addr, v); }
  template <> void write<sword_t>(ulong_t addr, sword_t v) { _w(addr, v); }
  template <> void write<slong_t>(ulong_t addr, slong_t v) { _l(addr, v); }
  template <> void write<ubyte_t>(ulong_t addr, ubyte_t v) { _b(addr, v); }
  template <> void write<uword_t>(ulong_t addr, uword_t v) { _w(addr, v); }
  template <> void write<ulong_t>(ulong_t addr, ulong_t v) { _l(addr, v); }

  template <class T> T read(ulong_t addr);
  template <> ubyte_t read<ubyte_t>(ulong_t addr) { return _b(addr); }
  template <> uword_t read<uword_t>(ulong_t addr) { return _w(addr); }
  template <> ulong_t read<ulong_t>(ulong_t addr) { return _l(addr); }
  template <> sbyte_t read<sbyte_t>(ulong_t addr) { return _b(addr); }
  template <> sword_t read<sword_t>(ulong_t addr) { return _w(addr); }
  template <> slong_t read<slong_t>(ulong_t addr) { return _l(addr); }

  sbyte_t b(ulong_t addr);
  sword_t w(ulong_t addr);
  slong_t l(ulong_t addr);
  ubyte_t ub(ulong_t addr);
  uword_t uw(ulong_t addr);
  ulong_t ul(ulong_t addr);

  void b(ulong_t addr, sbyte_t v);
  void w(ulong_t addr, sword_t v);
  void l(ulong_t addr, slong_t v);
  void ub(ulong_t addr, ubyte_t v);
  void uw(ulong_t addr, uword_t v);
  void ul(ulong_t addr, ulong_t v);

  slong_t read_signed(SizeKind size, ulong_t addr);
  void write_signed(SizeKind size, ulong_t addr, slong_t v);

  virtual void save_state(bee::Writer& writer) = 0;
  virtual void load_state(bee::Reader& reader) = 0;

 protected:
  virtual ubyte_t _b(ulong_t addr) = 0;
  virtual uword_t _w(ulong_t addr) = 0;
  virtual ulong_t _l(ulong_t addr) = 0;

  virtual void _b(ulong_t addr, ubyte_t v) = 0;
  virtual void _w(ulong_t addr, uword_t v) = 0;
  virtual void _l(ulong_t addr, ulong_t v) = 0;
};

} // namespace heaven_ice
