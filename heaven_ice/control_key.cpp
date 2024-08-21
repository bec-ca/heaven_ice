#include "control_key.hpp"

#include <map>

namespace heaven_ice {
namespace {

#define _(n) {ControlKey::n, #n}

std::map<ControlKey, const char*> names = {
  _(Start),
  _(Right),
  _(Left),
  _(Up),
  _(Down),
  _(A),
  _(B),
  _(C),
};

#undef _

} // namespace

const char* ControlKey::to_string() const
{
  if (auto it = names.find(_v); it != names.end()) { return it->second; }
  raise_error("Invalid ControlKey: $", int(_v));
}

ControlKey ControlKey::of_string(const std::string& str)
{
  for (const auto& p : names) {
    if (p.second == str) { return p.first; }
  }
  raise_error("Invalid ControlKey string: $", str);
}

} // namespace heaven_ice
