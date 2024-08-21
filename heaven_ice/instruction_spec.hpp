#pragma once

#include <string>
#include <vector>

#include "condition.hpp"
#include "inst_enum.hpp"
#include "instruction_field.hpp"
#include "register_id.hpp"
#include "types.hpp"

#include "heaven_ice/size_kind.hpp"

namespace heaven_ice {

struct InstFields {
  InstEnum name;

  InstFields(InstEnum name) : name(name) {}

  SizeKind size() const { return _size.value(); }
  Condition cond() const { return _cond.value(); }
  sword_t ea1() const { return _ea1.value(); }
  sword_t ea2() const { return _ea2.value(); }
  sword_t dir() const { return _dir.value(); }
  sbyte_t disp() const { return _disp.value(); }
  sbyte_t data() const { return _data.value(); }
  sbyte_t mode1() const { return _mode1.value(); }
  RegisterId reg1() const { return _reg1.value(); }
  RegisterId reg2() const { return _reg2.value(); }
  sbyte_t xn() const { return _xn.value(); }

  bool has_size() const { return _size.has_value(); }
  bool has_cond() const { return _cond.has_value(); }
  bool has_ea1() const { return _ea1.has_value(); }
  bool has_ea2() const { return _ea2.has_value(); }
  bool has_dir() const { return _dir.has_value(); }
  bool has_disp() const { return _disp.has_value(); }
  bool has_data() const { return _data.has_value(); }
  bool has_mode1() const { return _mode1.has_value(); }
  bool has_reg1() const { return _reg1.has_value(); }
  bool has_reg2() const { return _reg2.has_value(); }
  bool has_xn() const { return _xn.has_value(); }

 private:
  std::optional<SizeKind> _size;
  std::optional<Condition> _cond;
  std::optional<sword_t> _ea1;
  std::optional<sword_t> _ea2;
  std::optional<sword_t> _dir;
  std::optional<sbyte_t> _disp;
  std::optional<sbyte_t> _data;

  std::optional<sbyte_t> _mode1;

  std::optional<RegisterId> _reg1;
  std::optional<RegisterId> _reg2;
  std::optional<sbyte_t> _xn;

  friend struct InstructionSpec;
};

struct InstructionSpec {
 public:
  InstructionSpec(InstEnum name, std::vector<InstructionField> fields);

  InstEnum name;
  std::vector<InstructionField> fields;
  uword_t opcode_mask;
  uword_t masked_opcode;

  int size() const;
  bool match(uword_t opcode) const;

  std::string to_string() const;

  bee::OrError<InstFields> parse_fields(uword_t opcode) const;
};

} // namespace heaven_ice
