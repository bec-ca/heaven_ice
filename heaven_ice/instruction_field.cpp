#include "instruction_field.hpp"

namespace heaven_ice {

std::string InstructionField::to_string() const
{
  switch (kind) {
  case FieldKind::BitPattern:
    return bit_pattern;
  case FieldKind::S1:
    return "S:1";
  case FieldKind::S2:
    return "S:2";
  case FieldKind::SM2:
    return "SM:2";
  case FieldKind::M1:
    return "M:1";
  case FieldKind::M2:
    return "M:2";
  case FieldKind::M3:
    return "M:3";
  case FieldKind::An1:
    return "An:3";
  case FieldKind::An2:
    return "An2:3";
  case FieldKind::Dn1:
    return "Dn:3";
  case FieldKind::Dn2:
    return "Dn2:3";
  case FieldKind::Xn:
    return "Xn:3";
  case FieldKind::D1:
    return "D:3";
  case FieldKind::EA1:
    return "EA1:6";
  case FieldKind::EA2:
    return "EA2:6";
  case FieldKind::Vector4:
    return "Vector:4";
  case FieldKind::Data3:
    return "Data:3";
  case FieldKind::Data8:
    return "Data:8";
  case FieldKind::Cond:
    return "Cond:4";
  case FieldKind::Disp:
    return "Disp:8";
  }
}

int InstructionField::num_bits() const
{
  switch (kind) {
  case FieldKind::BitPattern:
    return bit_pattern.size();
  case FieldKind::S1:
    return 1;
  case FieldKind::S2:
    return 2;
  case FieldKind::SM2:
    return 2;
  case FieldKind::M1:
    return 1;
  case FieldKind::M2:
    return 2;
  case FieldKind::M3:
    return 3;
  case FieldKind::An1:
  case FieldKind::An2:
    return 3;
  case FieldKind::Dn1:
  case FieldKind::Dn2:
    return 3;
  case FieldKind::Xn:
    return 3;
  case FieldKind::D1:
    return 1;
  case FieldKind::EA1:
  case FieldKind::EA2:
    return 6;
  case FieldKind::Vector4:
    return 4;
  case FieldKind::Data3:
    return 3;
  case FieldKind::Data8:
    return 8;
  case FieldKind::Cond:
    return 4;
  case FieldKind::Disp:
    return 8;
  }
}

bee::OrError<InstructionField> InstructionField::parse_field(
  const std::string& spec)
{
  char c = spec[0];
  if (c == '0' || c == '1') {
    return InstructionField{.kind = FieldKind::BitPattern, .bit_pattern = spec};
  } else if (spec == "S:1") {
    return InstructionField{.kind = FieldKind::S1};
  } else if (spec == "S:2") {
    return InstructionField{.kind = FieldKind::S2};
  } else if (spec == "SM:2") {
    return InstructionField{.kind = FieldKind::SM2};
  } else if (spec == "M:1") {
    return InstructionField{.kind = FieldKind::M1};
  } else if (spec == "M:2") {
    return InstructionField{.kind = FieldKind::M2};
  } else if (spec == "M:3") {
    return InstructionField{.kind = FieldKind::M3};
  } else if (spec == "An:3") {
    return InstructionField{.kind = FieldKind::An1};
  } else if (spec == "An2:3") {
    return InstructionField{.kind = FieldKind::An2};
  } else if (spec == "Xn:3") {
    return InstructionField{.kind = FieldKind::Xn};
  } else if (spec == "Dn:3") {
    return InstructionField{.kind = FieldKind::Dn1};
  } else if (spec == "Dn2:3") {
    return InstructionField{.kind = FieldKind::Dn2};
  } else if (spec == "D:1") {
    return InstructionField{.kind = FieldKind::D1};
  } else if (spec == "EA1:6") {
    return InstructionField{.kind = FieldKind::EA1};
  } else if (spec == "EA2:6") {
    return InstructionField{.kind = FieldKind::EA2};
  } else if (spec == "Vector:4") {
    return InstructionField{.kind = FieldKind::Vector4};
  } else if (spec == "Data:3") {
    return InstructionField{.kind = FieldKind::Data3};
  } else if (spec == "Data:8") {
    return InstructionField{.kind = FieldKind::Data8};
  } else if (spec == "Cond:4") {
    return InstructionField{.kind = FieldKind::Cond};
  } else if (spec == "Disp:8") {
    return InstructionField{.kind = FieldKind::Disp};
  } else {
    return EF("Unrecoginized field spec: $", spec);
  }
}

} // namespace heaven_ice
