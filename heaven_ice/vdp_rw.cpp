#include "vdp_rw.hpp"

#include "bee/or_error.hpp"

namespace heaven_ice {

VDPRW VDPRW::of_code(uword_t code)
{
  switch (code) {
  case 0:
  case 4:
  case 8:
    return Read;
  case 1:
  case 3:
  case 5:
    return Write;
  default:
    raise_error("Invalid vdp transfer mode: $", code);
  }
}

const char* VDPRW::to_string() const
{
  switch (_e) {
  case Read:
    return "Read";
  case Write:
    return "Write";
  }
}

} // namespace heaven_ice
