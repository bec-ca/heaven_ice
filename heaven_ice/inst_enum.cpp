#include "inst_enum.hpp"

#include <map>
#include <vector>

#include "bee/or_error.hpp"

namespace heaven_ice {
namespace {

const std::map<InstEnum, const char*> enum_to_name = {
  {InstEnum::ORI_to_CCR, "ORI to CCR"},
  {InstEnum::ORI_to_SR, "ORI to SR"},
  {InstEnum::ORI, "ORI"},
  {InstEnum::ANDI_to_CCR, "ANDI to CCR"},
  {InstEnum::ANDI_to_SR, "ANDI to SR"},
  {InstEnum::ANDI, "ANDI"},
  {InstEnum::SUBI, "SUBI"},
  {InstEnum::ADDI, "ADDI"},
  {InstEnum::EORI_to_CCR, "EORI to CCR"},
  {InstEnum::EORI_to_SR, "EORI to SR"},
  {InstEnum::EORI, "EORI"},
  {InstEnum::CMPI, "CMPI"},
  {InstEnum::BTST, "BTST"},
  {InstEnum::BCHG, "BCHG"},
  {InstEnum::BCLR, "BCLR"},
  {InstEnum::BSET, "BSET"},
  {InstEnum::MOVEP, "MOVEP"},
  {InstEnum::MOVE, "MOVE"},
  {InstEnum::MOVE_from_SR, "MOVE from SR"},
  {InstEnum::MOVE_to_CCR, "MOVE to CCR"},
  {InstEnum::MOVE_to_SR, "MOVE to SR"},
  {InstEnum::NEGX, "NEGX"},
  {InstEnum::CLR, "CLR"},
  {InstEnum::NEG, "NEG"},
  {InstEnum::NOT, "NOT"},
  {InstEnum::EXT, "EXT"},
  {InstEnum::NBCD, "NBCD"},
  {InstEnum::SWAP, "SWAP"},
  {InstEnum::PEA, "PEA"},
  {InstEnum::ILLEGAL, "ILLEGAL"},
  {InstEnum::TAS, "TAS"},
  {InstEnum::TST, "TST"},
  {InstEnum::TRAP, "TRAP"},
  {InstEnum::LINK, "LINK"},
  {InstEnum::UNLK, "UNLK"},
  {InstEnum::MOVE_USP, "MOVE USP"},
  {InstEnum::RESET, "RESET"},
  {InstEnum::NOP, "NOP"},
  {InstEnum::STOP, "STOP"},
  {InstEnum::RTE, "RTE"},
  {InstEnum::RTS, "RTS"},
  {InstEnum::TRAPV, "TRAPV"},
  {InstEnum::RTR, "RTR"},
  {InstEnum::JSR, "JSR"},
  {InstEnum::JMP, "JMP"},
  {InstEnum::MOVEM, "MOVEM"},
  {InstEnum::LEA, "LEA"},
  {InstEnum::CHK, "CHK"},
  {InstEnum::ADDQ, "ADDQ"},
  {InstEnum::DBcc, "DBcc"},
  {InstEnum::Scc, "Scc"},
  {InstEnum::SUBQ, "SUBQ"},
  {InstEnum::BSR, "BSR"},
  {InstEnum::Bcc, "Bcc"},
  {InstEnum::MOVEQ, "MOVEQ"},
  {InstEnum::DIVU, "DIVU"},
  {InstEnum::DIVS, "DIVS"},
  {InstEnum::SBCD, "SBCD"},
  {InstEnum::OR, "OR"},
  {InstEnum::SUBA, "SUBA"},
  {InstEnum::SUBX, "SUBX"},
  {InstEnum::SUB, "SUB"},
  {InstEnum::CMPM, "CMPM"},
  {InstEnum::EOR, "EOR"},
  {InstEnum::CMPA, "CMPA"},
  {InstEnum::CMP, "CMP"},
  {InstEnum::MULU, "MULU"},
  {InstEnum::MULS, "MULS"},
  {InstEnum::ABCD, "ABCD"},
  {InstEnum::EXG, "EXG"},
  {InstEnum::AND, "AND"},
  {InstEnum::ADDA, "ADDA"},
  {InstEnum::ADDX, "ADDX"},
  {InstEnum::ADD, "ADD"},

  {InstEnum::ASR, "ASR"},
  {InstEnum::ASL, "ASL"},

  {InstEnum::LSR, "LSR"},
  {InstEnum::LSL, "LSL"},

  {InstEnum::ROXR, "ROXR"},
  {InstEnum::ROXL, "ROXL"},

  {InstEnum::ROR, "ROR"},
  {InstEnum::ROL, "ROL"},
};

auto make_str_to_enum()
{
  std::map<std::string, InstEnum> out;
  for (auto&& [e, name] : enum_to_name) { out.emplace(name, e); }
  return out;
}

auto make_all()
{
  std::vector<InstEnum> all;
  for (auto&& [e, name] : enum_to_name) { all.push_back(e); }
  return all;
}

const std::map<std::string, InstEnum> name_to_enum = make_str_to_enum();

} // namespace

const char* InstEnum::to_string() const
{
  auto it = enum_to_name.find(_e);
  if (it == enum_to_name.end()) {
    raise_error("Invalid instruction enum value: $", int(_e));
  }
  return it->second;
}

bee::OrError<InstEnum> InstEnum::of_string(const std::string& str)
{
  auto it = name_to_enum.find(str);
  if (it == name_to_enum.end()) {
    raise_error("Invalid instruction enum name: $", str);
  }
  return it->second;
}

const std::vector<InstEnum> InstEnum::all = make_all();

bool InstEnum::is_quick() const
{
  switch (_e) {
  case InstEnum::ADDQ:
  case InstEnum::SUBQ:
  case InstEnum::MOVEQ:
    return true;
  default:
    return false;
  }
}

bool InstEnum::is_a() const
{
  switch (_e) {
  case InstEnum::ADDA:
  case InstEnum::SUBA:
  case InstEnum::CMPA:
    return true;
  default:
    return false;
  }
}

} // namespace heaven_ice
