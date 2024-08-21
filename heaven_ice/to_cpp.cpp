#include "to_cpp.hpp"

#include <set>
#include <vector>

#include "addr_mode.hpp"
#include "condition.hpp"
#include "disasm.hpp"
#include "magic_constants.hpp"
#include "memory.hpp"
#include "register_id.hpp"

#include "bee/format_vector.hpp"
#include "bee/print.hpp"
#include "bee/sort.hpp"
#include "bee/string_util.hpp"
#include "heaven_ice/instruction.hpp"

namespace heaven_ice {
namespace {

const char* size_letter(SizeKind size)
{
  switch (size) {
  case SizeKind::Byte:
    return "b";
  case SizeKind::Word:
    return "w";
  case SizeKind::Long:
    return "l";
  }
}

const char* uppercase_size_letter(SizeKind size)
{
  switch (size) {
  case SizeKind::Byte:
    return "B";
  case SizeKind::Word:
    return "W";
  case SizeKind::Long:
    return "L";
  }
}

std::string reg_name(const RegisterId& reg)
{
  switch (reg.kind) {
  case RegisterKind::Data:
    return F("G.d[$]", reg.reg_id);
  case RegisterKind::Addr:
    return F("G.a[$]", reg.reg_id);
  case RegisterKind::SR:
    return "G.sr";
  }
}

std::string reg_value(SizeKind size, const RegisterId& reg)
{
  auto name = reg_name(reg);
  if (reg.kind == RegisterKind::Addr && size == SizeKind::l()) {
    return reg_name(reg);
  }
  switch (reg.kind) {
  case RegisterKind::Addr:
  case RegisterKind::Data:
    return F("$.$()", name, size_letter(size));
  case RegisterKind::SR:
    return F("$.to_int()", name);
  }
}

std::string write_register(
  SizeKind size, const RegisterId& reg, const std::string& value)
{
  auto name = reg_name(reg);
  switch (reg.kind) {
  case RegisterKind::Addr:
  case RegisterKind::Data:
    return F("$.$($)", name, size_letter(size), value);
  case RegisterKind::SR:
    return F("$.set_from_int($)", name, value);
  }
}

std::string operation(SizeKind size, InstEnum inst)
{
  switch (inst) {
  case InstEnum::AND:
  case InstEnum::ANDI:
    return F("AND<$>", size.stype_name());
  case InstEnum::ANDI_to_SR:
    return F("ANDR<$>", size.stype_name());
  case InstEnum::OR:
  case InstEnum::ORI:
    return F("OR<$>", size.stype_name());
  case InstEnum::ORI_to_SR:
    return F("ORR<$>", size.stype_name());
  case InstEnum::EOR:
  case InstEnum::EORI:
    return F("EOR<$>", size.stype_name());
  case InstEnum::ADD:
  case InstEnum::ADDA:
  case InstEnum::ADDQ:
  case InstEnum::ADDI:
    return F("ADD<$>", size.stype_name());
  case InstEnum::ABCD:
    return "ABCD";
  case InstEnum::SUB:
  case InstEnum::SUBA:
  case InstEnum::SUBQ:
  case InstEnum::SUBI:
    return F("SUB<$>", size.stype_name());
  case InstEnum::MULU:
    return "MULU";
  case InstEnum::MULS:
    return "MULS";
  case InstEnum::DIVU:
    return "DIVU";
  case InstEnum::DIVS:
    return "DIVS";
  case InstEnum::ROR:
    return F("ROR<$>", size.stype_name());
  case InstEnum::ROL:
    return F("ROL<$>", size.stype_name());
  case InstEnum::ASR:
    return F("ASR<$>", size.stype_name());
  case InstEnum::ASL:
    return F("ASL<$>", size.stype_name());
  case InstEnum::LSR:
    return F("LSR<$>", size.stype_name());
  case InstEnum::LSL:
    return F("LSL<$>", size.stype_name());
  case InstEnum::SWAP:
    return "SWAP";
  case InstEnum::NEG:
    return F("NEG<$>", size.stype_name());
  case InstEnum::NOT:
    return F("NOT<$>", size.stype_name());
  case InstEnum::EXT:
    return F("EXT<$>", size.stype_name());
  case InstEnum::BTST:
    return F("BTST<$>", size.stype_name());
  case InstEnum::BCLR:
    return F("BCLR<$>", size.stype_name());
  case InstEnum::BSET:
    return F("BSET<$>", size.stype_name());
  case InstEnum::BCHG:
    return F("BCHG<$>", size.stype_name());
  case InstEnum::CMP:
  case InstEnum::CMPI:
  case InstEnum::CMPA:
    return F("CMP<$>", size.stype_name());
  default:
    raise_error("Not supported: $", inst);
  }
}

struct Arg {
  SizeKind size;
  RegisterId reg;

  static Arg dw(int idx) { return {SizeKind::Word, RegisterId::data(idx)}; }
  static Arg dl(int idx) { return {SizeKind::Long, RegisterId::data(idx)}; }
  static Arg aw(int idx) { return {SizeKind::Word, RegisterId::addr(idx)}; }
  static Arg al(int idx) { return {SizeKind::Long, RegisterId::addr(idx)}; }

  std::string value_code() const { return reg_value(size, reg); }
};

struct Function {
  ulong_t start;
  bool skip_inlining = true;
  std::optional<std::string> manual_name;
  std::vector<Instruction> insts;
  std::vector<Arg> args;
  std::set<ulong_t> labels;

  std::string name() const
  {
    if (manual_name.has_value()) {
      return *manual_name;
    } else {
      return F("F{x}", start);
    }
  }

  std::string call_name() const
  {
    if (manual_name.has_value()) {
      return F("_m->$", *manual_name);
    } else {
      return F("F{x}", start);
    }
  }

  std::string call_code() const
  {
    std::vector<std::string> code_args;
    for (const auto& arg : args) { code_args.push_back(arg.value_code()); }
    return F("$($)", call_name(), bee::join(code_args, ", "));
  }
};

std::map<ulong_t, Function> functions;

std::optional<Function> find_function(ulong_t addr)
{
  if (auto it = functions.find(addr); it != functions.end()) {
    return it->second;
  }
  return std::nullopt;
}

struct Code {
 public:
  template <class T> void add_line(T&& line)
  {
    if (_comment_out) {
      _lines.push_back(F("// $", std::forward<T>(line)));
    } else {
      _lines.emplace_back(std::forward<T>(line));
    }
  }

  template <class... Ts> void f(const char* fmt, Ts&&... args)
  {
    return add_line(F(fmt, std::forward<Ts>(args)...));
  }

  void s(const char* s) { add_line(s); }

  const std::vector<std::string>& lines() const { return _lines; }

  void set_comment_out(bool comment_out) { _comment_out = comment_out; }

 private:
  std::vector<std::string> _lines;

  bool _comment_out = false;
};

////////////////////////////////////////////////////////////////////////////////
// Expression
//

struct ExpressionKind {
 public:
  enum E {
    Add,
    Assign,
    Call,
    Constant,
    Goto,
    If,
    Ram,
    Id,
    Reg,
    Return,
    Seq,
    Sub,
    Label,
    Comment,
  };

  ExpressionKind(E e) : _e(e) {}

  operator E() const { return _e; }

  bool operator==(const ExpressionKind& rhs) const = default;
  bool operator==(const E& rhs) const { return rhs == _e; };

  const char* to_string() const
  {
#define _(x)                                                                   \
  case x:                                                                      \
    return #x

    switch (_e) {
      _(Add);
      _(Assign);
      _(Call);
      _(Constant);
      _(Goto);
      _(If);
      _(Ram);
      _(Id);
      _(Reg);
      _(Return);
      _(Seq);
      _(Sub);
      _(Label);
      _(Comment);
    }

#undef _
  }

 private:
  E _e;
};

struct SizedValue {
  SizeKind size;
  slong_t value;

  static SizedValue b(sbyte_t b)
  {
    return {.size = SizeKind::b(), .value = slong_t(b)};
  }

  static SizedValue w(sword_t w)
  {
    return {.size = SizeKind::w(), .value = slong_t(w)};
  }

  static SizedValue l(slong_t l)
  {
    return {.size = SizeKind::l(), .value = slong_t(l)};
  }

  static SizedValue make(SizeKind size, slong_t v)
  {
    switch (size) {
    case SizeKind::Byte:
      return b(v);
    case SizeKind::Word:
      return w(v);
    case SizeKind::Long:
      return l(v);
    }
  }

  SizedValue operator+(const SizedValue& other) const
  {
    return make(std::max(size, other.size), value + other.value);
  }

  std::string to_cpp_hex() const
  {
    switch (size) {
    case SizeKind::Byte:
      return cpp_hex<ubyte_t>(value);
    case SizeKind::Word:
      return cpp_hex<uword_t>(value);
    case SizeKind::Long:
      return cpp_hex<ulong_t>(value);
    }
  }

  bool operator==(const SizedValue& rhs) const = default;
};

struct SizedReg {
  SizeKind size;
  RegisterId reg;

  static SizedReg make(SizeKind size, RegisterId reg)
  {
    return {
      .size = size,
      .reg = reg,
    };
  }
  std::string to_string() const { return F("$.$", reg, size); }

  bool operator==(const SizedReg& rhs) const = default;
};

struct SizedVariable {
  SizeKind size;
  std::string name;

  bool operator==(const SizedVariable& rhs) const = default;
};

struct Expression {
  ExpressionKind kind;
  std::optional<SizedValue> constant{};
  std::optional<std::string> fn_name{};
  std::optional<SizedReg> reg{};
  std::vector<Expression> args{};
  std::optional<SizeKind> ram_size{};
  std::optional<std::string> id{};
  std::optional<SizedVariable> variable{};
  std::optional<std::string> label_name{};
  std::optional<std::string> comment{};

  bool operator==(const Expression& rhs) const = default;

  template <class... T>
    requires(std::is_same_v<std::decay_t<T>, Expression> && ...)
  static Expression make_call(const std::string& name, T&&... args)
  {
    return make_call(name, std::vector<Expression>{std::forward<T>(args)...});
  }

  template <class Args>
    requires(std::convertible_to<Args, std::vector<Expression>>)
  static Expression make_call(const std::string& name, Args&& args)
  {
    return {
      .kind = ExpressionKind::Call,
      .fn_name = name,
      .args = std::forward<Args>(args),
    };
  }

  template <class... T>
    requires(std::is_same_v<std::decay_t<T>, Expression> && ...)
  static Expression make_seq(T&&... args)
  {
    return {
      .kind = ExpressionKind::Seq,
      .args = {std::forward<T>(args)...},
    };
  }

  template <class Arg> static Expression make_seq(Arg&& args)
  {
    return {
      .kind = ExpressionKind::Seq,
      .args = std::forward<Arg>(args),
    };
  }

  template <class LHS, class RHS>
  static Expression make_assign(LHS&& lhs, RHS&& rhs)
  {
    return {
      .kind = ExpressionKind::Assign,
      .args = {std::forward<LHS>(lhs), std::forward<RHS>(rhs)},
    };
  }

  static Expression make_const(const SizedValue& value)
  {
    return {
      .kind = ExpressionKind::Constant,
      .constant = value,
    };
  }

  static Expression make_const(SizeKind size, const slong_t v)
  {
    return {
      .kind = ExpressionKind::Constant,
      .constant = SizedValue::make(size, v),
    };
  }

  template <class Arg> static Expression make_ram(SizeKind size, Arg&& addr)
  {
    return {
      .kind = ExpressionKind::Ram,
      .args = {std::forward<Arg>(addr)},
      .ram_size = size,
    };
  }

  static Expression make_reg(SizeKind size, const RegisterId& reg)
  {
    return {
      .kind = ExpressionKind::Reg,
      .reg = SizedReg::make(size, reg),
    };
  }

  static Expression make_reg(const SizedReg& reg)
  {
    return {
      .kind = ExpressionKind::Reg,
      .reg = reg,
    };
  }

  template <class A1, class A2> static Expression make_add(A1&& a1, A2&& a2)
  {
    return {
      .kind = ExpressionKind::Add,
      .args = {std::forward<A1>(a1), std::forward<A2>(a2)},
    };
  }

  template <class A1, class A2> static Expression make_sub(A1&& a1, A2&& a2)
  {
    return {
      .kind = ExpressionKind::Sub,
      .args = {std::forward<A1>(a1), std::forward<A2>(a2)},
    };
  }

  template <class A> static Expression make_goto(A&& addr)
  {
    return {
      .kind = ExpressionKind::Goto,
      .args = {std::forward<A>(addr)},
    };
  }

  static Expression make_return()
  {
    return {
      .kind = ExpressionKind::Return,
    };
  }

  template <class C, class E> static Expression make_if(C&& cond, E&& e)
  {
    return {
      .kind = ExpressionKind::If,
      .args = {std::forward<C>(cond), std::forward<E>(e)},
    };
  }

  static Expression make_id(const std::string& name)
  {
    return {
      .kind = ExpressionKind::Id,
      .id = name,
    };
  }

  static Expression make_label(const std::string& name)
  {
    return {
      .kind = ExpressionKind::Label,
      .label_name = name,
    };
  }

  static Expression make_comment(const std::string& comment)
  {
    return {
      .kind = ExpressionKind::Comment,
      .comment = comment,
    };
  }

  std::string to_string() const
  {
    switch (kind) {
    case ExpressionKind::Add:
      return F("[$ + $]", args.at(0), args.at(1));
    case ExpressionKind::Sub:
      return F("[$ - $]", args.at(0), args.at(1));
    case ExpressionKind::Call:
      return F("$($)", fn_name.value(), bee::join(args, ", "));
    case ExpressionKind::Goto:
      return F("Goto $", args.at(0));
    case ExpressionKind::Return:
      return "Return";
    case ExpressionKind::Constant:
      return constant->to_cpp_hex();
    case ExpressionKind::Id:
      return *id;
    case ExpressionKind::Reg:
      return F(reg.value());
    case ExpressionKind::Ram:
      return F("($).$", args.at(0), ram_size.value());
    case ExpressionKind::Assign:
      return F("$ <- $", args.at(0), args.at(1));
    case ExpressionKind::If:
      return F("if $ {{ $ }", args.at(0), args.at(1));
    case ExpressionKind::Seq:
      return bee::join(args, "; ");
    case ExpressionKind::Label:
      return F("$:", label_name.value());
    case ExpressionKind::Comment:
      return comment.value();
    }
  }

  std::vector<std::string> to_string_multiline() const
  {
    switch (kind) {
    case ExpressionKind::Add:
    case ExpressionKind::Assign:
    case ExpressionKind::Call:
    case ExpressionKind::Constant:
    case ExpressionKind::Goto:
    case ExpressionKind::If:
    case ExpressionKind::Ram:
    case ExpressionKind::Id:
    case ExpressionKind::Reg:
    case ExpressionKind::Return:
    case ExpressionKind::Sub:
    case ExpressionKind::Label:
    case ExpressionKind::Comment:
      return {to_string()};
    case ExpressionKind::Seq: {
      std::vector<std::string> lines;
      for (auto& e : args) { lines.push_back(e.to_string()); }
      return lines;
    } break;
    }
  }

  std::string to_cpp_code() const
  {
    switch (kind) {
    case ExpressionKind::Add: {
      return F("$ + $", args.at(0).to_cpp_code(), args.at(1).to_cpp_code());
    } break;
    case ExpressionKind::Sub: {
      auto maybe_paren = [](const Expression& e) {
        if (e.is_sub() || e.is_add()) {
          return F("($)", e.to_cpp_code());
        } else {
          return F("$", e.to_cpp_code());
        }
      };
      return F("$ - $", args.at(0).to_cpp_code(), maybe_paren(args.at(1)));
    } break;
    case ExpressionKind::Call: {
      std::vector<std::string> cargs;
      for (auto& a : args) { cargs.push_back(a.to_cpp_code()); }
      return F("$($)", *fn_name, bee::join(cargs, ","));
    } break;
    case ExpressionKind::Constant: {
      return constant.value().to_cpp_hex();
    } break;
    case ExpressionKind::Ram: {
      return F(
        "G.io->$($)", size_letter(ram_size.value()), args.at(0).to_cpp_code());
    } break;
    case ExpressionKind::Seq: {
      std::string lines;
      for (auto& a : args) {
        if (a.kind == ExpressionKind::Comment || a.kind == ExpressionKind::If) {
          lines += F("$\n", a.to_cpp_code());
        } else {
          lines += F("$;\n", a.to_cpp_code());
        }
      }
      return lines;
    } break;
    case ExpressionKind::If: {
      auto block = [](const Expression& e) {
        auto code = e.to_cpp_code();
        if (e.kind == ExpressionKind::Seq) {
          return code;
        } else {
          return F("$;", code);
        }
      };
      return F("if ($) {{ $ }", args.at(0).to_cpp_code(), block(args.at(1)));
    } break;
    case ExpressionKind::Assign: {
      auto& dst = args.at(0);
      auto& rhs = args.at(1);
      switch (dst.kind) {
      case ExpressionKind::Ram:
        return F(
          "G.io->$($, $)",
          size_letter(dst.ram_size.value()),
          dst.args.at(0).to_cpp_code(),
          rhs.to_cpp_code());
      case ExpressionKind::Reg: {
        auto&& r = dst.reg.value();
        const bool is_long_addr = r.size == SizeKind::l() && r.reg.is_addr();
        if (rhs.is_add() && rhs.args.at(0) == dst) {
          if (is_long_addr) {
            return F("$ += $", reg_name(r.reg), rhs.args.at(1).to_cpp_code());
          } else {
            return F(
              "$.inc<$>($)",
              reg_name(r.reg),
              uppercase_size_letter(r.size),
              rhs.args.at(1).to_cpp_code());
          }
        } else if (rhs.is_sub() && rhs.args.at(0) == dst) {
          if (is_long_addr) {
            return F("$ -= $", reg_name(r.reg), rhs.args.at(1).to_cpp_code());
          } else {
            return F(
              "$.dec<$>($)",
              reg_name(r.reg),
              uppercase_size_letter(r.size),
              rhs.args.at(1).to_cpp_code());
          }
        } else if (is_long_addr) {
          return F("$ = $", reg_name(r.reg), rhs.to_cpp_code());
        } else {
          return write_register(r.size, r.reg, rhs.to_cpp_code());
        }
      } break;
      case ExpressionKind::Id: {
        return F("$ = $", dst.id.value(), rhs.to_cpp_code());
      } break;
      default: {
        raise_error("Assignment not implemented: $ <- $", dst, rhs);
      }
      }
    } break;
    case ExpressionKind::Reg: {
      const auto& r = reg.value();
      return reg_value(r.size, r.reg);
    }
    case ExpressionKind::Goto:
      return F("goto $", args.at(0).to_cpp_code());
    case ExpressionKind::Id:
      return id.value();
    case ExpressionKind::Return:
      return "goto end";
    case ExpressionKind::Label:
      return F("\n$:", label_name.value());
    case ExpressionKind::Comment:
      return F("// $", comment.value());
    }
  }

  bool is_zero() const
  {
    return kind == ExpressionKind::Constant && constant->value == 0;
  }

  bool is_one() const
  {
    return kind == ExpressionKind::Constant && constant->value == 1;
  }

  bool is_constant() const { return kind == ExpressionKind::Constant; }
  bool is_add() const { return kind == ExpressionKind::Add; }
  bool is_sub() const { return kind == ExpressionKind::Sub; }
  bool is_reg() const { return kind == ExpressionKind::Reg; }
  bool is_comment() const { return kind == ExpressionKind::Comment; }
  bool is_fn() const { return kind == ExpressionKind::Call; }

  template <class T> bool has(T&& fn) const
  {
    if (fn(*this)) { return true; }
    for (const auto& a : args) {
      if (a.has(fn)) return true;
    }
    return false;
  }

  bool has_ucc() const
  {
    return has(
      [](auto&& a) { return a.is_fn() && a.fn_name.value() == "UCC"; });
  }

  bool has_cc_updating_fn() const
  {
    constexpr const char* cc_update_fns[] = {
      "ADD<L>",
      "ADD<W>",
      "AND<W>",
      "ASL<L>",
      "ASR<L>",
      "CMP<L>",
      "SUB<L>",
      "SUB<W>",
      "UCC",
    };
    return has([&cc_update_fns](auto&& a) {
      if (!a.is_fn()) return false;
      const auto& name = a.fn_name.value();
      for (const auto& n : cc_update_fns) {
        if (name == n) { return true; }
      }
      return false;
    });
  }

  Expression remove_ucc() const
  {
    if (is_fn() && fn_name.value() == "UCC") { return args.at(0); }
    std::vector<Expression> exprs;
    for (const auto& a : args) { exprs.push_back(a.remove_ucc()); }
    auto copy = *this;
    copy.args = std::move(exprs);
    return copy;
  }

  std::vector<Expression> add_args() const
  {
    switch (kind) {
    case ExpressionKind::Add: {
      std::vector<Expression> out;
      for (const auto& arg : args)
        for (const auto& a : arg.add_args()) out.push_back(a);
      return out;
    } break;
    default:
      return {*this};
    }
  }

  std::vector<Expression> simplied_args() const
  {
    std::vector<Expression> out;
    for (auto& a : args) { out.emplace_back(a.simplify()); }
    return out;
  }

  Expression simplify_seq() const
  {
    std::vector<Expression> exprs;
    for (auto& a : simplied_args()) {
      if (a.kind == ExpressionKind::Seq) {
        for (auto& e1 : a.args) { exprs.emplace_back(std::move(e1)); }
      } else {
        exprs.emplace_back(std::move(a));
      }
    }
    Expression* last_non_comment = nullptr;
    for (int i = 0; i < std::ssize(exprs); i++) {
      auto&& a = exprs.at(i);
      if (a.is_comment()) continue;
      if (last_non_comment != nullptr) {
        if (a.has_cc_updating_fn() && last_non_comment->has_ucc()) {
          *last_non_comment = last_non_comment->remove_ucc();
        }
      }
      last_non_comment = &a;
    }
    return Expression::make_seq(std::move(exprs));
  }

  Expression simplify() const
  {
    switch (kind) {
    case ExpressionKind::Add: {
      std::vector<Expression> new_args;
      auto constant_sum = SizedValue::make(SizeKind::b(), 0);
      for (auto& sa : simplied_args()) {
        for (auto& a : sa.add_args()) {
          if (a.is_constant()) {
            constant_sum = constant_sum + *a.constant;
          } else {
            new_args.emplace_back(std::move(a));
          }
        }
      }
      if (constant_sum.value != 0 || new_args.empty()) {
        new_args.push_back(Expression::make_const(constant_sum));
      }
      Expression sum_all = std::move(new_args.at(0));
      for (int i = 1; i < std::ssize(new_args); i++) {
        sum_all = Expression::make_add(sum_all, std::move(new_args.at(i)));
      }
      return sum_all;
    } break;
    case ExpressionKind::Seq: {
      return simplify_seq();
    } break;
    case ExpressionKind::Assign: {
      return Expression::make_assign(
        args.at(0).simplify(), args.at(1).simplify());
    } break;
    case ExpressionKind::Call: {
      return Expression::make_call(fn_name.value(), simplied_args());
    } break;
    case ExpressionKind::Goto:
      return make_goto(args.at(0).simplify());
    case ExpressionKind::If:
      return make_if(args.at(0).simplify(), args.at(1).simplify());
    case ExpressionKind::Ram:
      return make_ram(ram_size.value(), args.at(0).simplify());
    case ExpressionKind::Sub:
      return make_sub(args.at(0).simplify(), args.at(1).simplify());
    case ExpressionKind::Constant:
    case ExpressionKind::Id:
    case ExpressionKind::Reg:
    case ExpressionKind::Return:
    case ExpressionKind::Label:
    case ExpressionKind::Comment:
      return *this;
    }
  }
};

Expression am_to_expression_addr(const AddrMode& am)
{
  switch (am.kind) {
  case AddrModeKind::ImmByte:
  case AddrModeKind::ImmWord:
  case AddrModeKind::ImmLong:
  case AddrModeKind::Reg:
    raise_error("Not supported am: $", am);
  case AddrModeKind::ImmAddrWord:
    return Expression::make_const(SizeKind::w(), am.imm);
  case AddrModeKind::ImmAddrLong:
    return Expression::make_const(SizeKind::l(), am.imm);
  case AddrModeKind::AReg:
    return Expression::make_reg(SizeKind::l(), am.reg);
  case AddrModeKind::PostInc:
  case AddrModeKind::PreDec:
    return Expression::make_reg(SizeKind::l(), am.reg);
  case AddrModeKind::ALongDisp:
    return Expression::make_add(
      Expression::make_reg(am.idx_size, am.reg),
      Expression::make_const(SizeKind::l(), am.imm));
  case AddrModeKind::AXByteDisp:
    return Expression::make_add(
      Expression::make_reg(SizeKind::Long, am.reg),
      Expression::make_add(
        Expression::make_reg(am.idx_size, am.reg2),
        Expression::make_const(SizeKind::b(), am.imm)));
  }
}

Expression am_to_expression(SizeKind size, const AddrMode& am)
{
  switch (am.kind) {
  case AddrModeKind::ImmByte:
    return Expression::make_const(SizeKind::b(), am.imm);
  case AddrModeKind::ImmWord:
    return Expression::make_const(SizeKind::w(), am.imm);
  case AddrModeKind::ImmLong:
    return Expression::make_const(SizeKind::l(), am.imm);
  case AddrModeKind::Reg:
    return Expression::make_reg(size, am.reg);
  default:
    return Expression::make_ram(size, am_to_expression_addr(am));
  }
}

std::optional<Expression> am_pre_expr(SizeKind size, const AddrMode& am)
{
  switch (am.kind) {
  case AddrModeKind::PreDec: {
    auto reg = Expression::make_reg(SizeKind::l(), am.reg);
    return Expression::make_assign(
      reg,
      Expression::make_sub(
        reg, Expression::make_const(SizeKind::l(), size.num_bytes())));
  } break;
  case AddrModeKind::ImmByte:
  case AddrModeKind::ImmWord:
  case AddrModeKind::ImmLong:
  case AddrModeKind::ImmAddrWord:
  case AddrModeKind::ImmAddrLong:
  case AddrModeKind::Reg:
  case AddrModeKind::AReg:
  case AddrModeKind::PostInc:
  case AddrModeKind::ALongDisp:
  case AddrModeKind::AXByteDisp:
    return std::nullopt;
  }
}

std::optional<Expression> am_post_expr(SizeKind size, const AddrMode& am)
{
  switch (am.kind) {
  case AddrModeKind::PostInc: {
    auto reg = Expression::make_reg(SizeKind::l(), am.reg);
    return Expression::make_assign(
      reg,
      Expression::make_add(
        reg, Expression::make_const(SizeKind::l(), size.num_bytes())));
  } break;
  case AddrModeKind::ImmByte:
  case AddrModeKind::ImmWord:
  case AddrModeKind::ImmLong:
  case AddrModeKind::ImmAddrWord:
  case AddrModeKind::ImmAddrLong:
  case AddrModeKind::Reg:
  case AddrModeKind::AReg:
  case AddrModeKind::PreDec:
  case AddrModeKind::ALongDisp:
  case AddrModeKind::AXByteDisp:
    return std::nullopt;
  }
}

Expression call_fn_expr(const Function& fn)
{
  std::vector<Expression> args;
  for (const auto& arg : fn.args) {
    args.push_back(Expression::make_reg(arg.size, arg.reg));
  }
  return Expression::make_call(fn.call_name(), args);
}

Expression instruction_to_expression_impl(
  const Instruction& inst, const Function& fn)
{
  auto get_jump_fn = [&inst]() -> std::optional<Function> {
    if (auto addr = inst.jump_addr(); addr) { return find_function(*addr); }
    return std::nullopt;
  };
  auto make_call = [&]() {
    if (auto jfn = get_jump_fn(); jfn && fn.start != jfn->start) {
      return call_fn_expr(*jfn);
    } else {
      return Expression::make_call(
        "JUMP_MAP", am_to_expression_addr(inst.src.value()));
    }
  };
  auto make_goto = [&]() {
    if (auto jfn = get_jump_fn(); jfn && fn.start != jfn->start) {
      return Expression::make_seq(
        call_fn_expr(*jfn), Expression::make_return());
    } else if (auto addr = inst.jump_addr(); addr) {
      auto label = F("L{x}", *addr);
      return Expression::make_goto(Expression::make_id(label));
    } else {
      return Expression::make_seq(make_call(), Expression::make_return());
    }
  };
  switch (inst.name) {
  case InstEnum::TST: {
    auto size = inst.size.value();
    return Expression::make_call(
      F("TST<$>", size.stype_name()), am_to_expression(size, inst.src.value()));
  } break;
  case InstEnum::JMP: {
    return make_goto();
  } break;
  case InstEnum::Bcc: {
    auto cond = inst.cond.value();
    auto gt = make_goto();
    if (cond == Condition::True) {
      return gt;
    } else {
      return Expression::make_if(
        Expression::make_call(
          "G.sr.check_condition",
          Expression::make_id(F("Condition::$", inst.cond->to_string()))),
        gt);
    }
  } break;
  case InstEnum::DBcc: {
    auto size = inst.size.value();
    auto cond = inst.cond.value();
    auto reg = am_to_expression(size, inst.dst.value());
    auto dec = Expression::make_assign(
      reg, Expression::make_sub(reg, Expression::make_const(size, 1)));
    auto block = Expression::make_seq(
      dec,
      Expression::make_if(
        Expression::make_call("NOT_MINUS_ONE", reg), make_goto()));
    if (cond == Condition::False) {
      return block;
    } else {
      return Expression::make_if(
        Expression::make_call(
          "!G.sr.check_condition",
          Expression::make_id(F("Condition::$", inst.cond->to_string()))),
        block);
    }
  } break;
  case InstEnum::LEA: {
    return Expression::make_assign(
      am_to_expression(SizeKind::Long, inst.dst.value()),
      am_to_expression_addr(inst.src.value()));
  } break;
  case InstEnum::MOVE_USP:
  case InstEnum::MOVE_to_SR:
  case InstEnum::MOVE_from_SR: {
    auto size = inst.size.value();
    auto dst = inst.dst.value();
    auto src = inst.src.value();
    return Expression::make_assign(
      am_to_expression(size, dst), am_to_expression(size, src));
  } break;
  case InstEnum::MOVEQ:
  case InstEnum::MOVE: {
    auto size = inst.size.value();
    auto dst = inst.dst.value();
    auto src = inst.src.value();
    return Expression::make_assign(
      am_to_expression(size, dst),
      Expression::make_call("UCC", am_to_expression(size, src)));
  } break;
  case InstEnum::DIVU:
  case InstEnum::DIVS: {
    SizeKind src_size = SizeKind::Word;
    SizeKind dst_size = SizeKind::Long;
    auto dst_expr = am_to_expression(dst_size, inst.dst.value());
    auto src_expr = am_to_expression(src_size, inst.src.value());
    auto value =
      Expression::make_call(operation(dst_size, inst.name), dst_expr, src_expr);
    return Expression::make_assign(dst_expr, value);
  } break;
  case InstEnum::MULS:
  case InstEnum::MULU: {
    SizeKind input_size = SizeKind::Word;
    SizeKind output_size = SizeKind::Long;
    auto src = inst.src.value();
    auto dst = inst.dst.value();
    auto value = Expression::make_call(
      operation(output_size, inst.name),
      am_to_expression(input_size, dst),
      am_to_expression(input_size, src));
    return Expression::make_assign(am_to_expression(output_size, dst), value);
  } break;
  case InstEnum::BTST:
  case InstEnum::CMP:
  case InstEnum::CMPA:
  case InstEnum::CMPI: {
    auto size = inst.size.value();
    return Expression::make_call(
      operation(size, inst.name),
      am_to_expression(size, inst.dst.value()),
      am_to_expression(size, inst.src.value()));
  } break;
  case InstEnum::CLR: {
    auto size = inst.size.value();
    return Expression::make_assign(
      am_to_expression(size, inst.dst.value()),
      Expression::make_call("UCC", Expression ::make_const(size, 0)));
  } break;
  case InstEnum::RTE:
  case InstEnum::RTS: {
    return Expression::make_return();
  } break;
  case InstEnum::BSR:
  case InstEnum::JSR: {
    return make_call();
  } break;
  case InstEnum::EXG: {
    auto size = inst.size.value();
    auto tmp = Expression::make_id(F("tmp_$", size));
    auto src = am_to_expression(size, inst.src.value());
    auto dst = am_to_expression(size, inst.dst.value());
    return Expression::make_seq(
      Expression::make_assign(tmp, src),
      Expression::make_assign(src, dst),
      Expression::make_assign(dst, tmp));
  } break;
  case InstEnum::ADD:
  case InstEnum::ADDA:
  case InstEnum::ADDQ:
  case InstEnum::ADDI:
  case InstEnum::ABCD:
  case InstEnum::BCLR:
  case InstEnum::BSET:
  case InstEnum::BCHG:
  case InstEnum::SUB:
  case InstEnum::SUBA:
  case InstEnum::SUBQ:
  case InstEnum::SUBI:
  case InstEnum::AND:
  case InstEnum::ANDI:
  case InstEnum::ANDI_to_SR:
  case InstEnum::OR:
  case InstEnum::ORI:
  case InstEnum::ORI_to_SR:
  case InstEnum::EOR:
  case InstEnum::EORI:
  case InstEnum::ROR:
  case InstEnum::ROL:
  case InstEnum::ASR:
  case InstEnum::ASL:
  case InstEnum::LSR:
  case InstEnum::LSL: {
    auto src_size = inst.size.value();
    auto dst = inst.dst.value();
    auto dst_size = dst.is_addr_reg() ? SizeKind::l() : inst.size.value();
    auto src = inst.src.value();
    auto fn_name = operation(dst_size, inst.name);

    auto dst_expr = am_to_expression(dst_size, dst);
    auto src_expr = am_to_expression(src_size, src);
    auto value = Expression::make_call(fn_name, dst_expr, src_expr);
    return Expression::make_assign(dst_expr, value);
  } break;
  case InstEnum::MOVEM: {
    auto make = [&](SizeKind size, const AddrMode& am, int offset) {
      auto addr = am_to_expression_addr(am);
      if (!am.is_inc_or_dec()) {
        auto inc = offset * size.num_bytes();
        addr = Expression::make_add(
          addr, Expression::make_const(SizeKind::l(), inc));
      }
      return Expression::make_ram(size, addr);
    };
    auto size = inst.size.value();
    auto l = inst.register_list.value();
    int offset = 0;
    std::vector<Expression> exprs;
    for (int i = 0; i < 16; i++) {
      if (!l.contains(i)) continue;
      auto reg = l.reg(i);
      if (inst.src.has_value()) {
        auto src = inst.src.value();
        if (auto p = am_pre_expr(size, src); p) { exprs.push_back(*p); }
        auto src_expr = make(size, src, offset);
        exprs.push_back(Expression::make_assign(
          Expression::make_reg(SizeKind::l(), reg), src_expr));
        if (auto p = am_post_expr(size, src); p) { exprs.push_back(*p); }
      } else {
        auto dst = inst.dst.value();
        if (auto p = am_pre_expr(size, dst); p) { exprs.push_back(*p); }
        auto dst_expr = make(size, dst, offset);
        exprs.push_back(
          Expression::make_assign(dst_expr, Expression::make_reg(size, reg)));
        if (auto p = am_post_expr(size, dst); p) { exprs.push_back(*p); }
      }
      offset++;
    }
    return Expression::make_seq(exprs);
  } break;
  case InstEnum::SWAP:
  case InstEnum::NEG:
  case InstEnum::NOT: {
    auto size = inst.size.value();
    auto dst = am_to_expression(size, inst.dst.value());
    return Expression::make_assign(
      dst, Expression::make_call(operation(size, inst.name), dst));
  } break;
  case InstEnum::NOP: {
    return Expression::make_seq();
  } break;
  case InstEnum::EXT: {
    auto write_size = inst.size.value();
    auto read_size = write_size.previous();
    auto dst = inst.dst.value();
    return Expression::make_assign(
      am_to_expression(write_size, dst),
      Expression::make_call(
        operation(write_size, inst.name), am_to_expression(read_size, dst)));
  } break;
  default:
    raise_error("Instruction not implemented: $", inst.name);
  }
}

Expression instruction_to_expression(
  const Instruction& inst, const Function& fn)
{
  std::vector<Expression> exprs;

  if (fn.labels.contains(inst.pc)) {
    exprs.push_back(Expression::make_label(F("L{x}", inst.pc)));
  }

  exprs.push_back(
    Expression::make_comment(F("{06x}: $", inst.pc, inst.to_string())));

  auto size = inst.size;
  if (size && inst.name != InstEnum::MOVEM) {
    auto add_pre = [&](auto&& am) {
      if (am) {
        if (auto p = am_pre_expr(size.value(), *am); p) { exprs.push_back(*p); }
      }
    };
    add_pre(inst.src);
    add_pre(inst.dst);
  }
  exprs.push_back(instruction_to_expression_impl(inst, fn));
  if (size && inst.name != InstEnum::MOVEM) {
    auto add_post = [&](auto&& am) {
      if (am) {
        if (auto p = am_post_expr(size.value(), *am); p) {
          exprs.push_back(*p);
        }
      }
    };
    add_post(inst.src);
    add_post(inst.dst);
  }

  if (exprs.size() == 1) {
    return exprs[0];
  } else {
    return Expression::make_seq(exprs);
  }
}

void print_one_fn(Code& code, const Function& fn)
{
  std::vector<Expression> exprs;
  for (auto& inst : fn.insts) {
    exprs.push_back(instruction_to_expression(inst, fn));
  }
  auto icode = Expression::make_seq(exprs).simplify().to_cpp_code();
  code.f("  $", icode);
}

template <class T>
bee::OrError<std::map<ulong_t, Instruction>> find_reachable_insts(
  Disasm& d, const T& starting_pcs)
{
  std::map<ulong_t, Instruction> insts;
  std::set<ulong_t> seen;
  std::deque<ulong_t> queue;

  auto enqueue = [&](ulong_t pc) {
    if (!seen.contains(pc)) {
      seen.insert(pc);
      queue.push_back(pc);
    }
  };

  auto dequeue = [&]() {
    ulong_t pc = queue.front();
    queue.pop_front();
    return pc;
  };

  for (auto addr : starting_pcs) { enqueue(addr); }

  while (!queue.empty()) {
    ulong_t pc = dequeue();
    bail(inst, d.disasm_one(pc));
    insts.emplace(inst.pc, inst);
    if (auto addr = inst.jump_addr(); addr) { enqueue(*addr); }
    if (!inst.is_unconditional_jump()) { enqueue(inst.pc + inst.bytes); }
  }
  return insts;
}

bee::OrError<std::vector<Instruction>> find_function_insts(
  const std::map<ulong_t, Instruction>& all_insts, ulong_t start_pc)
{
  std::vector<Instruction> fn_insts;
  std::set<ulong_t> seen;
  std::deque<ulong_t> queue;

  auto is_another_fn = [&](ulong_t pc) {
    return (pc != start_pc && find_function(pc));
  };

  auto enqueue = [&](ulong_t pc) {
    if (!seen.contains(pc)) {
      seen.insert(pc);
      queue.push_back(pc);
    }
  };

  auto dequeue = [&]() {
    ulong_t pc = queue.front();
    queue.pop_front();
    return pc;
  };

  enqueue(start_pc);
  while (!queue.empty()) {
    ulong_t pc = dequeue();
    auto inst = all_insts.at(pc);
    if (auto addr = inst.jump_addr();
        addr && !inst.is_fn_call() && !is_another_fn(*addr)) {
      enqueue(*addr);
    }
    if (!inst.is_unconditional_jump() && !is_another_fn(inst.pc)) {
      enqueue(inst.pc + inst.bytes);
    }
    if (is_another_fn(inst.pc)) {
      fn_insts.push_back({
        .name = InstEnum::JMP,
        .pc = inst.pc,
        .bytes = inst.bytes - 1,
        .src = AddrMode::make_imm_addr(inst.pc),
      });
    } else {
      fn_insts.push_back(inst);
    }
  }
  bee::sort(fn_insts, [](auto&& i1, auto&& i2) { return i1.pc < i2.pc; });
  return std::move(fn_insts);
}

} // namespace

bee::OrError<> ToCpp::to_cpp(const std::string& rom_content)
{
  auto rom = std::make_shared<Memory>(rom_content);
  bail(d, Disasm::create(rom));

  std::set<ulong_t> funcs = {
    0x00200, 0x00300, 0x00488, 0x0091a, 0x00928, 0x00958, 0x0095c, 0x00960,
    0x009b0, 0x014b4, 0x014e8, 0x01540, 0x01750, 0x0387e, 0x0389a, 0x0389e,
    0x038a0, 0x038c6, 0x03c5a, 0x04718, 0x050a0, 0x05124, 0x0519e, 0x05abc,
    0x05ac2, 0x05ae2, 0x05b00, 0x05b3c, 0x05c88, 0x05de4, 0x05dfa, 0x066a2,
    0x066aa, 0x066b2, 0x066ba, 0x06b12, 0x0736a, 0x076b2, 0x0775e, 0x07810,
    0x08bc6, 0x09746, 0x097a6, 0x0a0d4, 0x0a170, 0x10f5e, 0x01650, 0x0284e,
    0x03e5a, 0x050ca, 0x08d5e, 0x08e4e, 0x10794, 0x107a4, 0x107b2, 0x1082c,
    0x04776, 0x0fa98, 0x05f82,
  };

  std::map<ulong_t, ulong_t> fn_jump_map;

  const ulong_t ranges[][3] = {
    {0x0542, 0x05de, 4},
    {0x09ce, 0x0a36, 4},
    {0x3c70, 0x3c78, 4},
    {0x66b2, 0x66c0, 2},
    {0x67dc, 0x67ea, 2},
  };
  for (auto&& [start, end, step] : ranges) {
    for (ulong_t pc = start; pc <= end; pc += step) {
      bail(inst, d->disasm_one(pc));
      auto addr = inst.jump_addr().value();
      funcs.insert(addr);
      fn_jump_map.emplace(pc, addr);
    }
  }

  auto extra_jump_fns = {
    0x00a26, 0x03e2e, 0x03e4a, 0x03eb2, 0x03eea, 0x04718, 0x04f86, 0x050a0,
    0x050d4, 0x050e6, 0x05114, 0x0518e, 0x05192, 0x0519e, 0x051a2, 0x05984,
    0x05a9a, 0x05abc, 0x05ade, 0x05ae2, 0x05b5e, 0x05bf4, 0x05d9a, 0x05dc8,
    0x05e08, 0x05e3c, 0x05f28, 0x062d0, 0x063e8, 0x06414, 0x06442, 0x064aa,
    0x0654c, 0x065a4, 0x06600, 0x0663a, 0x06658, 0x066e2, 0x0675a, 0x0677c,
    0x06820, 0x06858, 0x06870, 0x06b12, 0x06b1c, 0x06b22, 0x06bd0, 0x0736a,
    0x075ca, 0x076b2, 0x0775e, 0x07810, 0x079a4, 0x07a34, 0x07ba4, 0x07c04,
    0x07d06, 0x07d3c, 0x07e4a, 0x07ec8, 0x07f9a, 0x0801c, 0x0814a, 0x081de,
    0x083a0, 0x0842a, 0x08558, 0x08592, 0x0861c, 0x086a4, 0x08792, 0x087d2,
    0x0885a, 0x088cc, 0x08b5e, 0x08bb8, 0x08cd6, 0x08d2a, 0x08dbe, 0x08df0,
    0x08e88, 0x08ec0, 0x08f3c, 0x08f92, 0x09084, 0x090c4, 0x090e2, 0x0915a,
    0x091fa, 0x0926c, 0x0932e, 0x093d0, 0x09608, 0x09652, 0x09746, 0x097a6,
    0x098ac, 0x099e0, 0x09cc2, 0x09d46, 0x09f5a, 0x09fb0, 0x0a0d4, 0x0a170,
    0x0a2b2, 0x0a358, 0x0a4d8, 0x0a502, 0x0a546, 0x0a5b6, 0x0a77a, 0x0a79e,
    0x0a7d0, 0x0a7f6, 0x0b3d0, 0x0b42a, 0x0b4a6, 0x0b4e4, 0x0b574, 0x0b58e,
    0x0b6ae, 0x0b768, 0x0b974, 0x0ba14, 0x0bb22, 0x0bb58, 0x0bbd4, 0x0bc2c,
    0x0bf30, 0x0c0be, 0x0c812, 0x0c8b0, 0x0cf02, 0x0d04c, 0x0d1c4, 0x0d1ce,
    0x0d324, 0x0d438, 0x0d43a, 0x0d6aa, 0x0d880, 0x0dc44, 0x0dca4, 0x0dd42,
    0x0de36, 0x0e18a, 0x0e1cc, 0x0e294, 0x0e360, 0x0e836, 0x0e940, 0x0e9ea,
    0x0ea48, 0x0eb1a, 0x0ebee, 0x0ee5c, 0x0eeb2, 0x0f6d4, 0x0f870, 0x0fc14,
    0x0fdc2, 0x10076, 0x10214, 0x10464, 0x104c6, 0x1054e, 0x105ce, 0x10638,
    0x1063c, 0x10640, 0x10644, 0x10648, 0x1064c, 0x10650, 0x10654, 0x10658,
    0x1065c, 0x10e08, 0x10e64, 0x10f5e,
  };

  for (ulong_t addr : extra_jump_fns) {
    fn_jump_map.emplace(addr, addr);
    funcs.insert(addr);
  }

  bail(insts, find_reachable_insts(*d, funcs));
  for (const auto& [_, inst] : insts) {
    if (auto addr = inst.jump_addr(); addr && inst.is_fn_call()) {
      funcs.insert(*addr);
    }
  }

  auto get_fn = [&](ulong_t addr) -> Function& {
    auto& fn = functions[addr];
    fn.start = addr;
    return fn;
  };

  auto set_manual = [&](ulong_t addr, const char* name, const auto&&... args) {
    auto& f = get_fn(addr);
    f.manual_name = name;
    f.args = {args...};
  };

  for (auto addr : funcs) { get_fn(addr); }

  set_manual(0x5d9a, "vblank", Arg::dw(0));

  set_manual(0x0958, "set_d0_1");
  set_manual(0x095c, "clear_d0");
  set_manual(0x3c5a, "noop");
  set_manual(0x67c8, "neg_w_d2_exg_d2_d3");
  set_manual(0x67ca, "exg_d2_d3");
  set_manual(0x67cc, "noop");
  set_manual(0x67ce, "exg_d2_d3_neg_w_d2");
  set_manual(0x67d0, "neg_w_d2");
  set_manual(0x67d8, "neg_w_d3");
  set_manual(0x67d4, "exg_neg_w_d2_d3");
  set_manual(0x67d6, "neg_w_d2_d3");
  set_manual(0x5d84, "enable_vblank_int");

  set_manual(0x5e08, "clear_cram");
  set_manual(0x649c, "clear_vscroll");

  set_manual(
    0x63e8,
    "vdp_set_d3_blocks_of_size_d2_with_d0_starting_at_d1",
    Arg::dw(0),
    Arg::dl(1),
    Arg::dw(2),
    Arg::dw(3));

  set_manual(0x5db2, "wait_for_z80_bus");

  set_manual(0x5dfa, "unused");
  set_manual(
    0x5de4, "vdp_copy_words_to_cram", Arg::dw(0), Arg::dl(1), Arg::dw(2));
  set_manual(
    0x5dc8, "vdp_copy_words_to_cram", Arg::dw(0), Arg::dl(1), Arg::dw(2));

  set_manual(0x5fc2, "some_weird_transformation", Arg::dw(0));
  set_manual(0x5fc0, "some_weird_transformation_16_times");

  set_manual(0x928, "F928_manual", Arg::al(5), Arg::al(6));

  set_manual(0x91a, "F91a_manual", Arg::dw(0), Arg::al(5), Arg::al(6));

  set_manual(0x8f0, "F8f0_manual", Arg::al(5), Arg::al(6));

  set_manual(0x5984, "update_sprite_with_something", Arg::al(6));

  set_manual(0x5e5c, "dma_push_with_cmd", Arg::dw(0), Arg::dl(1), Arg::dl(2));

  set_manual(0x5d6e, "disable_vblank_int");

  set_manual(0x64aa, "push_scroll_state");

  set_manual(0x5c5c, "add_to_dma_queue", Arg::dw(0), Arg::dl(1), Arg::dw(2));

  set_manual(0x5e3c, "dma_push", Arg::dw(0), Arg::dl(1), Arg::dl(2));

  set_manual(0x5c22, "flush_dma");

  set_manual(0x5ef0, "enqueue_some_tile_animation_to_dma");

  set_manual(0x1eda, "copy_something_to_vdp", Arg::al(0), Arg::dw(0));

  set_manual(0x200, "start");
  set_manual(0x300, "F300_manual");
  set_manual(0x488, "F488_manual");

  set_manual(0x9b0, "F9b0_manual", Arg::al(6), Arg::dw(2));
  set_manual(0x960, "F960_manual", Arg::dw(2));

  set_manual(0x6442, "clear_all_planes");
  set_manual(0x6476, "clear_window_plane");

  set_manual(0x5bf4, "clear_sprites");

  set_manual(0x65bc, "z80_useless");

  set_manual(0x677c, "F677c_manual", Arg::al(6));

  set_manual(0x6864, "inc_something", Arg::al(6));

  set_manual(0x5bba, "clear_sprite_on_a6_14");

  set_manual(0x5bbe, "clear_sprite_position", Arg::dw(0));

  for (auto&& [_, fn] : functions) {
    bail(ret, find_function_insts(insts, fn.start));
    fn.insts = std::move(ret);
    if (fn.start != fn.insts.front().pc) { fn.labels.insert(fn.start); }
    for (auto& inst : fn.insts) {
      if (auto addr = inst.jump_addr(); addr) { fn.labels.insert(*addr); }
    }
  }

  Code code;
  code.f("#pragma clang diagnostic ignored \"-Wunused-label\"");
  code.f("#pragma clang diagnostic ignored \"-Wunused-function\"");
  code.f("#pragma clang diagnostic ignored \"-Wunused-variable\"");
  code.f("");
  code.f("#include \"generated.hpp\"");
  code.f("");
  code.f("#include \"inst_impls.hpp\"");
  code.f("#include \"magic_constants.hpp\"");
  code.f("#include \"manual_functions.hpp\"");
  code.f("");
  code.f("#include \"bee/print.hpp\"");
  code.f("");
  code.f("namespace heaven_ice {{");
  code.f("namespace {{");
  code.f("");

  code.s("ulong_t tmp_L;");
  code.s("uword_t tmp_W;");
  code.s("ubyte_t tmp_B;");

  code.s("");
  code.s("struct GeneratedImpl final : public GeneratedIntf {");
  code.s("");
  code.s("void JUMP_MAP(ulong_t addr) {");
  code.s("  _log_call(__func__);");
  code.s("");
  code.s("  switch (addr) {");
  for (auto&& [addr, fn_addr] : fn_jump_map) {
    auto fn = functions.find(fn_addr)->second;
    code.f("  case 0x{x}: $; break;", addr, fn.call_code());
  }
  code.s("  default: raise_error(\"No mapping for address: {x}\", "
         "addr);");
  code.s("  }");
  code.s("  _log_ret(__func__);");
  code.s("}");
  code.s("");

  for (auto&& [_, fn] : functions) {
    bool is_manual = fn.manual_name.has_value();
    if (is_manual) continue;
    code.set_comment_out(is_manual);
    auto&& insts = fn.insts;
    auto name = fn.name();
    code.f("void $() {{", name);
    code.s("  _log_call(__func__);");
    code.s("");
    if (insts.begin()->pc != fn.start) { code.f("  goto L{x};", fn.start); }
    print_one_fn(code, fn);
    code.s("");
    code.s("  end:");
    code.s("  _log_ret(__func__);");
    code.s("}");
    code.set_comment_out(false);
    code.s("");
  }
  code.s("");
  code.s("GeneratedImpl(bool v, const ManualFunctions::ptr& m)");
  code.s(": _m(m), _verbose(v) {}");
  code.s("void run() { _m->start(); }");
  code.s("void jump_map(ulong_t addr) { JUMP_MAP(addr); }");
  code.s("void vblank_int() { F5c88(); }");
  code.s("void _log_call(const char* fn_name) const");
  code.s("{");
  code.s("  if (_verbose) P(\"Call $\", fn_name);");
  code.s("}");
  code.s("void _log_ret(const char* fn_name) const");
  code.s("{");
  code.s("  if (_verbose) P(\"Returned $\", fn_name);");
  code.s("}");
  code.s("ManualFunctions::ptr _m;");
  code.s("bool _verbose;");
  code.s("");
  code.s("};");
  code.s("");
  code.s("}");
  code.s("");
  code.s("GeneratedIntf::ptr Generated::create(");
  code.s("  bool verbose, const ManualFunctions::ptr& m)");
  code.s("{");
  code.s("  return std::make_shared<GeneratedImpl>(verbose, m);");
  code.s("}");
  code.s("");
  code.s("}");

  for (auto&& line : code.lines()) { P(line); }
  return bee::ok();
}

} // namespace heaven_ice
