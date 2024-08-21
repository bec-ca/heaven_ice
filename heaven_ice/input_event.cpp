#include "input_event.hpp"

#include <map>

#include "bee/format.hpp"

namespace heaven_ice {
namespace {

#define _(n) {InputEventKind::n, #n}

std::map<InputEventKind, const char*> kind_names = {
  _(ControlKeyDown),
  _(ControlKeyUp),
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// InputEventKind
//

const char* InputEventKind::to_string() const
{
  if (auto it = kind_names.find(_v); it != kind_names.end()) {
    return it->second;
  }
  raise_error("Invalid InputEventKind: $", int(_v));
}

InputEventKind InputEventKind::of_string(const std::string& str)
{
  for (const auto& n : kind_names) {
    if (n.second == str) { return n.first; }
  }
  raise_error("Invalid InputEventKind string: $", str);
}

////////////////////////////////////////////////////////////////////////////////
// InputEvent
//

using yasf_format = std::pair<InputEventKind, std::optional<ControlKey>>;

std::string InputEvent::to_string() const
{
  switch (kind) {
  case InputEventKind::ControlKeyDown:
    return F("KeyDown:$", key.value());
  case InputEventKind::ControlKeyUp:
    return F("KeyUp:$", key.value());
  }
}

yasf::Value::ptr InputEvent::to_yasf_value() const
{
  return yasf::ser(yasf_format(kind, key));
}

InputEvent InputEvent::of_yasf_value(const yasf::Value::ptr& value)
{
  must(fmt, yasf::des<yasf_format>(value));
  return {.kind = fmt.first, .key = fmt.second};
}

} // namespace heaven_ice
