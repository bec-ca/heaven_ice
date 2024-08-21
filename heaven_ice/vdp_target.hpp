#include <string>

#include "types.hpp"

namespace heaven_ice {

struct VDPTarget {
 public:
  enum E {
    VRAM,
    CRAM,
    VSRAM,
    BUS,
    DATA,
  };

  VDPTarget(E e) : _e(e) {}

  operator E() const { return _e; }

  static VDPTarget of_code(uword_t code);
  const char* to_string() const;

  std::string format_addr(ulong_t addr) const;

 private:
  E _e;
};

} // namespace heaven_ice
