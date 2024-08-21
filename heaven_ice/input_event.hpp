#pragma once

#include <optional>
#include <string>

#include "control_key.hpp"

#include "yasf/of_stringable_mixin.hpp"

namespace heaven_ice {

struct InputEventKind : public yasf::OfStringableMixin<InputEventKind> {
 public:
  enum E {
    ControlKeyDown,
    ControlKeyUp,
  };

  InputEventKind(E v) : _v(v) {}

  const char* to_string() const;

  operator E() const { return _v; }
  static InputEventKind of_string(const std::string& str);

 private:
  E _v;
};

struct InputEvent {
 public:
  InputEventKind kind;
  std::optional<ControlKey> key;

  std::string to_string() const;

  yasf::Value::ptr to_yasf_value() const;
  static InputEvent of_yasf_value(const yasf::Value::ptr& value);
};

} // namespace heaven_ice
