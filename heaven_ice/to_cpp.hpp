#pragma once

#include "bee/or_error.hpp"

namespace heaven_ice {

struct ToCpp {
 public:
  static bee::OrError<> to_cpp(const std::string& content);
};

} // namespace heaven_ice
