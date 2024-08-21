#include "bit_manip.hpp"

#include "bee/testing.hpp"

namespace heaven_ice {
namespace {

TEST(MSB)
{
  PRINT_EXPR(msb<ubyte_t>());
  PRINT_EXPR(msb<uword_t>());
  PRINT_EXPR(msb<ulong_t>());
}

} // namespace
} // namespace heaven_ice
