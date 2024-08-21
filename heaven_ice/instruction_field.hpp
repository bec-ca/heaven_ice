#pragma once

#include <string>

#include "bee/or_error.hpp"

namespace heaven_ice {

enum class FieldKind {
  BitPattern,
  S1,
  S2,
  SM2,
  M1,
  M2,
  M3,
  An1,
  An2,
  Dn1,
  Dn2,
  Xn,
  D1,
  EA1,
  EA2,
  Vector4,
  Data3,
  Data8,
  Cond,
  Disp,
};

struct InstructionField {
 public:
  FieldKind kind;
  std::string bit_pattern{};

  std::string to_string() const;

  int num_bits() const;

  static bee::OrError<InstructionField> parse_field(const std::string& spec);
};

} // namespace heaven_ice
