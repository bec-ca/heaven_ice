#include "binary.hpp"

namespace heaven_ice {

std::string Binary::word_to_binary(uword_t w)
{
  std::string str(16, ' ');
  for (int i = 0; i < 16; i++) {
    str[15 - i] = (w % 2) + '0';
    w /= 2;
  }
  return str;
}

} // namespace heaven_ice
