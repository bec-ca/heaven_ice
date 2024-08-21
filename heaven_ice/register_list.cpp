#include "register_list.hpp"

#include <vector>

#include "register_id.hpp"

#include "bee/string_util.hpp"

namespace heaven_ice {

RegisterId RegisterList::reg(int idx) const
{
  if (reverse) {
    return idx < 8 ? RegisterId::addr(7 - idx) : RegisterId::data(15 - idx);
  } else {
    return idx < 8 ? RegisterId::data(idx) : RegisterId::addr(idx - 8);
  }
}

bool RegisterList::contains(int idx) const { return ((mask >> idx) & 1) == 1; }

std::string RegisterList::to_string() const
{
  std::vector<RegisterId> registers;
  for (int i = 0; i < 16; i++) {
    if (!contains(i)) continue;
    registers.push_back(reg(i));
  }
  return bee::join(registers, ",");
}

} // namespace heaven_ice
