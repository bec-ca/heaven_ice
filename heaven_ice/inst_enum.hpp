#pragma once

#include <compare>
#include <vector>

#include "bee/or_error.hpp"

namespace heaven_ice {

struct InstEnum {
 public:
  enum E {
    ORI_to_CCR,
    ORI_to_SR,
    ORI,
    ANDI_to_CCR,
    ANDI_to_SR,
    ANDI,
    SUBI,
    ADDI,
    EORI_to_CCR,
    EORI_to_SR,
    EORI,
    CMPI,
    BTST,
    BCHG,
    BCLR,
    BSET,
    MOVEP,
    MOVE,
    MOVE_from_SR,
    MOVE_to_CCR,
    MOVE_to_SR,
    NEGX,
    CLR,
    NEG,
    NOT,
    EXT,
    NBCD,
    SWAP,
    PEA,
    ILLEGAL,
    TAS,
    TST,
    TRAP,
    LINK,
    UNLK,
    MOVE_USP,
    RESET,
    NOP,
    STOP,
    RTE,
    RTS,
    TRAPV,
    RTR,
    JSR,
    JMP,
    MOVEM,
    LEA,
    CHK,
    ADDQ,
    DBcc,
    Scc,
    SUBQ,
    BSR,
    Bcc,
    MOVEQ,
    DIVU,
    DIVS,
    SBCD,
    OR,
    SUBA,
    SUBX,
    SUB,
    CMPM,
    EOR,
    CMPA,
    CMP,
    MULU,
    MULS,
    ABCD,
    EXG,
    AND,
    ADDA,
    ADDX,
    ADD,

    ASR,
    ASL,

    LSR,
    LSL,

    ROXR,
    ROXL,

    ROR,
    ROL,

  };

  InstEnum(E e) : _e(e) {}

  operator E() const { return _e; }

  auto operator<=>(const InstEnum& other) const = default;
  bool operator==(E e) const { return _e == e; }

  const char* to_string() const;

  static bee::OrError<InstEnum> of_string(const std::string& str);

  static const std::vector<InstEnum> all;

  bool is_quick() const;
  bool is_a() const;

 private:
  E _e;
};

} // namespace heaven_ice
