#include "io_intf.hpp"

namespace heaven_ice {

IOIntf::~IOIntf() {}

ubyte_t IOIntf::ub(ulong_t addr) { return _b(addr); }
uword_t IOIntf::uw(ulong_t addr) { return _w(addr); }
ulong_t IOIntf::ul(ulong_t addr) { return _l(addr); }

sbyte_t IOIntf::b(ulong_t addr) { return _b(addr); }
sword_t IOIntf::w(ulong_t addr) { return _w(addr); }
slong_t IOIntf::l(ulong_t addr) { return _l(addr); }

void IOIntf::b(ulong_t addr, sbyte_t v) { _b(addr, v); }
void IOIntf::w(ulong_t addr, sword_t v) { _w(addr, v); }
void IOIntf::l(ulong_t addr, slong_t v) { _l(addr, v); }

void IOIntf::ub(ulong_t addr, ubyte_t v) { _b(addr, v); }
void IOIntf::uw(ulong_t addr, uword_t v) { _w(addr, v); }
void IOIntf::ul(ulong_t addr, ulong_t v) { _l(addr, v); }

slong_t IOIntf::read_signed(SizeKind size, ulong_t addr)
{
  switch (size) {
  case SizeKind::Byte:
    return b(addr);
  case SizeKind::Word:
    return w(addr);
  case SizeKind::Long:
    return l(addr);
  }
}

void IOIntf::write_signed(SizeKind size, ulong_t addr, slong_t v)
{
  switch (size) {
  case SizeKind::Byte:
    b(addr, v);
    break;
  case SizeKind::Word:
    w(addr, v);
    break;
  case SizeKind::Long:
    l(addr, v);
    break;
  }
}

} // namespace heaven_ice
