#include "size_kind.hpp"

#include "bee/testing.hpp"

namespace heaven_ice {
namespace {

TEST(mask)
{
  PRINT_EXPR(SizeKind(SizeKind::Byte).mask());
  PRINT_EXPR(SizeKind(SizeKind::Word).mask());
  PRINT_EXPR(SizeKind(SizeKind::Long).mask());
}

TEST(trim)
{
  PRINT_EXPR(SizeKind(SizeKind::Byte).trim(0xf));
  PRINT_EXPR(SizeKind(SizeKind::Byte).trim(0xff));
  PRINT_EXPR(SizeKind(SizeKind::Byte).trim(0x100));

  PRINT_EXPR(SizeKind(SizeKind::Word).trim(0xf));
  PRINT_EXPR(SizeKind(SizeKind::Word).trim(0xff));
  PRINT_EXPR(SizeKind(SizeKind::Word).trim(0x100));
  PRINT_EXPR(SizeKind(SizeKind::Word).trim(0x7fff));
  PRINT_EXPR(SizeKind(SizeKind::Word).trim(0xffff));
  PRINT_EXPR(SizeKind(SizeKind::Word).trim(0x8000));
  PRINT_EXPR(SizeKind(SizeKind::Word).trim(0x10000));
}

} // namespace
} // namespace heaven_ice
