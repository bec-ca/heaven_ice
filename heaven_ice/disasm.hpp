#pragma once

#include <memory>
#include <string>

#include "instruction.hpp"
#include "io_intf.hpp"

#include "bee/or_error.hpp"

namespace heaven_ice {

struct Disasm {
 public:
  using ptr = std::shared_ptr<Disasm>;

  virtual bee::OrError<Instruction> disasm_one(ulong_t addr) = 0;

  static bee::OrError<ptr> create(const IOIntf::ptr& bus);

  static bee::OrError<> disasm_and_print(const std::string& rom_content);
};

} // namespace heaven_ice
