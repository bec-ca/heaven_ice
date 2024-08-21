#include "dir.hpp"

#include "bee/or_error.hpp"

namespace heaven_ice {

const char* Dir::to_string() const
{
  switch (_e) {
  case Right:
    return "R";
  case Left:
    return "L";
  }
}

Dir Dir::of_code(int code)
{
  switch (code) {
  case 0:
    return Right;
  case 1:
    return Left;
  default:
    raise_error("Invalid dir code: $", code);
  }
}

} // namespace heaven_ice
