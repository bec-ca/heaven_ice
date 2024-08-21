#pragma once

#include <string>

namespace heaven_ice {

enum class RegisterKind {
  Data,
  Addr,
  SR,
};

struct RegisterId {
 public:
  RegisterKind kind;
  int reg_id;

  std::string to_string() const;

  static RegisterId data(int reg_id);
  static RegisterId addr(int reg_id);
  static RegisterId usp();
  static RegisterId sr();
  static const char* register_kind_prefix(RegisterKind kind);

  bool is_addr() const;

  bool operator==(const RegisterId& rhs) const;

 private:
  static void check_id(int reg_id);
};

} // namespace heaven_ice
