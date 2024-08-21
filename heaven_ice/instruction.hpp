#pragma once

#include <optional>
#include <string>

#include "addr_mode.hpp"
#include "condition.hpp"
#include "inst_enum.hpp"
#include "register_list.hpp"
#include "size_kind.hpp"
#include "types.hpp"

namespace heaven_ice {

struct Instruction {
 public:
  InstEnum name;
  ulong_t pc{0};
  ulong_t bytes{0};
  std::optional<SizeKind> size{};
  std::optional<Condition> cond{};
  std::optional<AddrMode> src{};
  std::optional<AddrMode> dst{};
  std::optional<RegisterList> register_list{};

  std::string to_string() const;

  bool is_unconditional_jump() const;

  bool is_conditional_jump() const;

  bool is_fn_call() const;

  std::optional<ulong_t> jump_addr() const;
};

} // namespace heaven_ice
