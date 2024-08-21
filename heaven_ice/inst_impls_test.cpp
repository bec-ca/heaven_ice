#include <limits>

#include "inst_impls.hpp"

#include "bee/testing.hpp"

namespace heaven_ice {
namespace {

TEST(msg)
{
  PRINT_EXPR(msb<B>());
  PRINT_EXPR(msb<W>());
  PRINT_EXPR(msb<L>());
  PRINT_EXPR(msb<UB>());
  PRINT_EXPR(msb<UW>());
  PRINT_EXPR(msb<UL>());
}

TEST(CMP)
{
  auto run_test = [](B a, B b) {
    P("------------");
    auto res = CMP<B>(a, b);
    P("$ $ -> $", a, b, B(res));
    P(G.sr);
  };
  run_test(0, 0);
  run_test(0, 1);
  run_test(127, 127);
  run_test(0, -128);
  run_test(127, -128);
  run_test(-128, 1);
  run_test(127, 110);
  run_test(110, 127);
  run_test(-100, 10);
  run_test(-100, -10);
  run_test(10, 100);
  run_test(100, 10);
  run_test(-10, -10);
  run_test(127, -1);
  run_test(126, -1);
  run_test(1, -1);
  run_test(64, -64);
  run_test(127, -2);
}

TEST(ROR)
{
  auto run_test = [](UW a, int b) {
    P("------------");
    UW res = ROR<W>(a, b);
    P("ROR({x},  $) -> {x}", a, b, res);
  };
  run_test(0x1010, 0);
  run_test(0x1010, 1);
  run_test(0x1010, 4);
  run_test(0xfffa, 4);
}

TEST(ABCD)
{
  auto run_test = [](UB a, UB b) {
    G.sr.set_from_int(4);
    P("------------");
    P("ABCD({x},  {x}) -> {x}", a, b, ABCD(a, b));
    P(G.sr);
    G.sr.set_ext(true);
    P("ABCD({x},  {x}) -> {x}", a, b, ABCD(a, b));
    P(G.sr);
  };
  run_test(0, 0);
  run_test(0x1, 0x1);
  run_test(0x1, 0x9);
  run_test(0x9, 0x9);
  run_test(0x4, 0x5);
  run_test(0x19, 0x19);
  run_test(0x99, 0x1);
  run_test(0x99, 0x99);
  run_test(0x23, 0x45);
  run_test(0x28, 0x45);
  run_test(0x28, 0x85);
  run_test(0x90, 0x10);
  run_test(0xd4, 0x15);
}

} // namespace
} // namespace heaven_ice
