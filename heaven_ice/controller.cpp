#include "controller.hpp"

#include <array>

#include "magic_constants.hpp"

#include "bee/print.hpp"

namespace heaven_ice {
namespace {

template <class T, class... Ts>
ulong_t concat_values(ulong_t out, T v, Ts... vs)
{
  if constexpr (sizeof...(vs) == 0) {
    return out;
  } else {
    out = (out << sizeof(T) * 8) | v;
    return concat_values(out, vs...);
  }
}

template <class C> void zero_out(C&& c)
{
  for (auto& i : c) { i = 0; }
}

struct Control {
 public:
  Control()
  {
    _ctrl.fill(0);
    _data.fill(0);
    _pressed_keys.fill(false);
  }

  void key_down(ControlKey key) { _pressed_keys.at(key) = true; }
  void key_up(ControlKey key) { _pressed_keys.at(key) = false; }

  ubyte_t ctrl(int idx) const { return _ctrl[idx]; }
  ubyte_t data(int idx) const { return _data[idx]; }

  void ctrl(int idx, ubyte_t v) { _ctrl[idx] = v; }

  void data(int, ubyte_t v) { _data[1] = _make_data((v >> 6) & 1); }

 private:
  ubyte_t _make_data(int id) const
  {
    ubyte_t ret = id << 6;
    switch (id) {
    case 0: {
      ret |= _key_bit(ControlKey::Up) << 0;
      ret |= _key_bit(ControlKey::Down) << 1;
      ret |= _key_bit(ControlKey::A) << 4;
      ret |= _key_bit(ControlKey::Start) << 5;
    } break;
    case 1: {
      ret |= _key_bit(ControlKey::Up) << 0;
      ret |= _key_bit(ControlKey::Down) << 1;
      ret |= _key_bit(ControlKey::Left) << 2;
      ret |= _key_bit(ControlKey::Right) << 3;
      ret |= _key_bit(ControlKey::B) << 4;
      ret |= _key_bit(ControlKey::C) << 5;
    } break;
    default:
      raise_error("Invalid control data: $", id);
    }
    return ret;
  }

  ubyte_t _key_bit(ControlKey key) const
  {
    return _pressed_keys.at(key) ? 0 : 1;
  }

  std::array<ubyte_t, 2> _ctrl;
  std::array<ubyte_t, 2> _data;
  std::array<bool, 8> _pressed_keys;
};

struct ControllerImpl final : public Controller {
 public:
  ControllerImpl() {}

  ~ControllerImpl() {}

  void key_down(int control_id, ControlKey key) override
  {
    get_control(control_id).key_down(key);
  }

  void key_up(int control_id, ControlKey key) override
  {
    get_control(control_id).key_up(key);
  }

  void save_state(bee::Writer&) override {}
  void load_state(bee::Reader&) override {}

 private:
  Control& get_control(int control_id)
  {
    switch (control_id) {
    case 0:
      return _control1;
    case 1:
      return _control2;
    default:
      raise_error("Only controls 0 and 1 supported, got $", control_id);
    }
  }

  ubyte_t _b(ulong_t addr) override
  {
    switch (addr) {
    case CONTROLLER1_DATA1:
      return _control1.data(0);
    case CONTROLLER1_DATA2:
      return _control1.data(1);
    case CONTROLLER2_DATA1:
      return _control2.data(0);
    case CONTROLLER2_DATA2:
      return _control2.data(1);
    case CONTROLLER1_CTRL1:
      return _control1.ctrl(0);
    case CONTROLLER1_CTRL2:
      return _control1.ctrl(1);
    case CONTROLLER2_CTRL1:
      return _control2.ctrl(0);
    case CONTROLLER2_CTRL2:
      return _control2.ctrl(1);
    default:
      raise_error("Controller address not supported for byte read: {x}", addr);
    }
  }

  uword_t _w(ulong_t addr) override
  {
    switch (addr) {
    case EXPANSION_PORT_CTRL:
      // Expansion port control, only used for modem, not interesting, I don't
      // even know why heaven_ice checks this port
      return 0;
    default:
      return concat_values(_b(addr), _b(addr + 1));
    }
  }

  ulong_t _l(ulong_t addr) override
  {
    return concat_values(_w(addr), _w(addr + 2));
  }

  void _b(ulong_t addr, ubyte_t v) override
  {
    switch (addr) {
    case CONTROLLER1_CTRL1:
      _control1.ctrl(0, v);
      break;
    case CONTROLLER1_CTRL2:
      _control1.ctrl(1, v);
      break;
    case CONTROLLER2_CTRL1:
      _control2.ctrl(0, v);
      break;
    case CONTROLLER2_CTRL2:
      _control2.ctrl(1, v);
      break;
    case CONTROLLER1_DATA1:
      _control1.data(0, v);
      break;
    case CONTROLLER1_DATA2:
      _control1.data(1, v);
      break;
    case CONTROLLER2_DATA1:
      _control2.data(0, v);
      break;
    case CONTROLLER2_DATA2:
      _control2.data(1, v);
      break;
    default:
      raise_error("Controller address not supported for byte write: {x}", addr);
    }
  }

  void _w(ulong_t, uword_t) override { raise_error("Not implemented"); }
  void _l(ulong_t, ulong_t) override { raise_error("Not implemented"); }

  Control _control1;
  Control _control2;
};

} // namespace

Controller::ptr Controller::create()
{
  return std::make_shared<ControllerImpl>();
}

} // namespace heaven_ice
