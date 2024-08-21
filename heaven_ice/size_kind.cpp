#include "size_kind.hpp"

#include <limits>

#include "bee/or_error.hpp"

namespace heaven_ice {

int SizeKind::num_bytes() const
{
  switch (_e) {
  case Long:
    return 4;
  case Word:
    return 2;
  case Byte:
    return 1;
  }
}

int SizeKind::num_bits() const { return num_bytes() * 8; }

bee::OrError<SizeKind> SizeKind::decode3(int code)
{
  switch (code) {
  case 0:
    return Byte;
  case 1:
    return Word;
  case 2:
    return Long;
  default:
    return EF("Unknown size code3: $", code);
  }
}

bee::OrError<SizeKind> SizeKind::decode_move(int code)
{
  switch (code) {
  case 1:
    return Byte;
  case 3:
    return Word;
  case 2:
    return Long;
  default:
    return EF("Unknown size code_move: $", code);
  }
}

bee::OrError<SizeKind> SizeKind::decode2(int code)
{
  switch (code) {
  case 0:
    return Word;
  case 1:
    return Long;
  default:
    return EF("Unknown size code2: $", code);
  }
}

const char* SizeKind::to_string() const
{
  switch (_e) {
  case Byte:
    return "B";
  case Word:
    return "W";
  case Long:
    return "L";
  }
}

std::string SizeKind::format(slong_t value) const
{
  switch (_e) {
  case Byte:
    return F("{x}", ubyte_t(value));
  case Word:
    return F("{x}", uword_t(value));
  case Long:
    return F("{x}", ulong_t(value));
  }
}

ulong_t SizeKind::msb_mask() const { return 1u << (num_bytes() * 8 - 1); }

SizeKind SizeKind::previous() const
{
  switch (_e) {
  case Byte:
    raise_error("Byte size has no previous size");
  case Word:
    return Byte;
  case Long:
    return Word;
  }
}

slong_t SizeKind::trim(slong_t value) const
{
  switch (_e) {
  case Byte:
    return sbyte_t(value);
  case Word:
    return sword_t(value);
  case Long:
    return slong_t(value);
  }
}

ulong_t SizeKind::mask() const
{
  switch (_e) {
  case Byte:
    return 0xffu;
  case Word:
    return 0xffffu;
  case Long:
    return 0xffffffffu;
  }
}

slong_t SizeKind::min_value() const
{
  switch (_e) {
  case Byte:
    return std::numeric_limits<sbyte_t>::min();
  case Word:
    return std::numeric_limits<sword_t>::min();
  case Long:
    return std::numeric_limits<slong_t>::min();
  }
}

slong_t SizeKind::max_value() const
{
  switch (_e) {
  case Byte:
    return std::numeric_limits<sbyte_t>::max();
  case Word:
    return std::numeric_limits<sword_t>::max();
  case Long:
    return std::numeric_limits<slong_t>::max();
  }
}

const char* SizeKind::stype_name() const
{
  switch (_e) {
  case Byte:
    return "B";
  case Word:
    return "W";
  case Long:
    return "L";
  }
}

const char* SizeKind::utype_name() const
{
  switch (_e) {
  case Byte:
    return "UB";
  case Word:
    return "UW";
  case Long:
    return "UL";
  }
}

} // namespace heaven_ice
