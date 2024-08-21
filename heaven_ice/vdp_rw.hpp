#pragma once

#include "types.hpp"

namespace heaven_ice {

struct VDPRW {
 public:
  enum E {
    Read,
    Write,
  };

  VDPRW(E e) : _e(e) {}

  operator E() const { return _e; }

  static VDPRW of_code(uword_t code);

  const char* to_string() const;

 private:
  E _e;
};

} // namespace heaven_ice
