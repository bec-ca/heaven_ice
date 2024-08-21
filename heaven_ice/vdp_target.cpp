#include "vdp_target.hpp"

#include "bee/or_error.hpp"

namespace heaven_ice {

VDPTarget VDPTarget::of_code(uword_t code)
{
  switch (code) {
  case 0:
  case 1:
    return VRAM;
  case 8:
  case 3:
    return CRAM;
  case 4:
  case 5:
    return VSRAM;
  default:
    raise_error("Invalid vdp transfer mode: $", code);
  }
}

const char* VDPTarget::to_string() const
{
  switch (_e) {
  case VRAM:
    return "VRAM";
  case CRAM:
    return "CRAM";
  case VSRAM:
    return "VSRAM";
  case BUS:
    return "BUS";
  case DATA:
    return "DATA";
  }
}

std::string VDPTarget::format_addr(ulong_t addr) const
{
  switch (_e) {
  case VRAM:
    return F("VRAM({05x})", addr);
  case CRAM:
    return F("CRAM({02x})", addr);
  case VSRAM:
    return F("VSRAM({02x})", addr);
  case BUS:
    return F("BUS({06x})", addr);
  case DATA:
    return "DATA";
  }
}

} // namespace heaven_ice
