#include "condition.hpp"

#include "bee/error.hpp"

namespace heaven_ice {

Condition Condition::of_code(int code)
{
  switch (code) {
  case 0:
    return True;
  case 1:
    return False;
  case 2:
    return HI;
  case 3:
    return LS;
  case 4:
    return CC;
  case 5:
    return CS;
  case 6:
    return NE;
  case 7:
    return EQ;
  case 8:
    return VC;
  case 9:
    return VS;
  case 10:
    return PL;
  case 11:
    return MI;
  case 12:
    return GE;
  case 13:
    return LT;
  case 14:
    return GT;
  case 15:
    return LE;
  default:
    raise_error("Invalid condition code: $", code);
  }
  return Condition(Value(code));
}

const char* Condition::to_string() const
{
  switch (_value) {
  case True:
    return "True";
  case False:
    return "False";
  case HI:
    return "HI";
  case LS:
    return "LS";
  case CC:
    return "CC";
  case CS:
    return "CS";
  case NE:
    return "NE";
  case EQ:
    return "EQ";
  case VC:
    return "VC";
  case VS:
    return "VS";
  case PL:
    return "PL";
  case MI:
    return "MI";
  case GE:
    return "GE";
  case LT:
    return "LT";
  case GT:
    return "GT";
  case LE:
    return "LE";
  }
}

} // namespace heaven_ice
