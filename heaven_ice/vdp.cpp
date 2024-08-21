#include "vdp.hpp"

#include <algorithm>
#include <cstring>

#include "bit_manip.hpp"
#include "magic_constants.hpp"
#include "save_state.hpp"
#include "vdp_rw.hpp"
#include "vdp_target.hpp"

#include "bee/format.hpp"
#include "bee/print.hpp"
#include "pixel/image.hpp"

namespace heaven_ice {
namespace {

constexpr ulong_t VRAM_SIZE = 0x20000;
constexpr int MAX_SPRITES = 80;
constexpr int TILE_SIZE = 8;

constexpr int SCREEN_HEIGHT = 224;
constexpr int SCREEN_WIDTH = 320;

inline bool is_word_cmd(ulong_t cmd) { return ((cmd & 0xe000) == 0x8000); }
inline bool is_long_cmd(ulong_t cmd) { return ((cmd & 0xff0c) == 0); }

template <class T> T MOD(T a, T b) { return ((a % b) + b) % b; }

template <class T> T DIV(T a, T b) { return (a - MOD<T>(a, b)) / b; }

template <class T> inline bool is_bit_set(T value, int bit)
{
  return ((value >> bit) & 1) == 1;
}

////////////////////////////////////////////////////////////////////////////////
// Transfer
//

struct Transfer {
  VDPTarget dst;
  VDPTarget src;
  ulong_t dst_addr;
  ulong_t length;
  bool dma;
  bool fill;
};

////////////////////////////////////////////////////////////////////////////////
// Plane
//

struct Plane {
 public:
  enum E {
    Foreground,
    Background,
    Window,
  };

  Plane(E e) : _e(e) {}

  operator E() const { return _e; }

  const char* to_string() const
  {
    switch (_e) {
    case Foreground:
      return "Foreground";
    case Background:
      return "Background";
    case Window:
      return "Window";
    }
  }

 private:
  E _e;
};

////////////////////////////////////////////////////////////////////////////////
// HScrollKind
//

struct HScrollKind {
 public:
  enum E {
    WholeScreen,
    Per8PixelStrips,
    PerScanLine,
  };

  HScrollKind(E e) : _e(e) {}

  operator E() const { return _e; }

  static HScrollKind of_code(int code)
  {
    switch (code) {
    case 0:
      return WholeScreen;
    case 2:
      return Per8PixelStrips;
    case 4:
      return PerScanLine;
    default:
      raise_error("Invalid HScrollKind code: $", code);
    }
  }

  const char* to_string() const
  {
    switch (_e) {
    case WholeScreen:
      return "WholeScreen";
    case Per8PixelStrips:
      return "Per8PixelStrips";
    case PerScanLine:
      return "PerScanLine";
    }
  }

 private:
  E _e;
};

////////////////////////////////////////////////////////////////////////////////
// VScrollKind
//

struct VScrollKind {
 public:
  enum E {
    WholeScreen,
    Per16PixelStrips,
  };

  VScrollKind(E e) : _e(e) {}

  operator E() const { return _e; }

  static VScrollKind of_code(int code)
  {
    switch (code) {
    case 0:
      return WholeScreen;
    case 1:
      return Per16PixelStrips;
    default:
      raise_error("Invalid HScrollKind code: $", code);
    }
  }

  const char* to_string() const
  {
    switch (_e) {
    case WholeScreen:
      return "WholeScreen";
    case Per16PixelStrips:
      return "Per16PixelStrips";
    }
  }

 private:
  E _e;
};

////////////////////////////////////////////////////////////////////////////////
// VDPMode
//

struct VDPMode {
 public:
  VDPMode(ubyte_t b1, ubyte_t b2) : _b1(b1), _b2(b2) {}

  bool horizontal_interrupts() const { return is_bit_set(_b1, 4); }
  bool normal_operation() const { return is_bit_set(_b1, 2); }
  bool freeze_hv_counter() const { return is_bit_set(_b1, 1); }
  bool display() const { return !is_bit_set(_b1, 0); }

  bool use_128k() const { return is_bit_set(_b2, 7); }
  bool clear() const { return !is_bit_set(_b2, 6); }
  bool vertical_interrupts() const { return is_bit_set(_b2, 5); }
  bool dma() const { return is_bit_set(_b2, 4); }
  bool pal_mode() const { return is_bit_set(_b2, 3); }
  bool md_mode() const { return is_bit_set(_b2, 2); }

  std::string to_string() const
  {
    return F(
      "horiz_int:$ norm:$ freze_hv:$ display:$ 128k:$ clear:$ vert_int:$ dma:$ "
      "pal_mode:$ md_mode:$",
      horizontal_interrupts(),
      normal_operation(),
      freeze_hv_counter(),
      display(),
      use_128k(),
      clear(),
      vertical_interrupts(),
      dma(),
      pal_mode(),
      md_mode()

    );
  }

 private:
  ubyte_t _b1, _b2;
};

////////////////////////////////////////////////////////////////////////////////
// VDPStatusRegister
//

struct VDPStatusRegister {
  ulong_t to_word() const
  {
    BW ret = 0x3400;
    if (fifo_empty) { ret.set(9); }
    if (fifo_full) { ret.set(8); }
    if (vblank_progress) { ret.set(3); }
    if (pal) { ret.set(1); }
    return ret;
  }

  bool fifo_empty = false;
  bool fifo_full = false;
  bool vblank_progress = false;
  bool pal = false;
};

////////////////////////////////////////////////////////////////////////////////
// Priority
//

struct Priority {
 public:
  enum E {
    Low,
    High,
  };

  Priority(E e) : _e(e) {}

  static Priority of_code(int code)
  {
    switch (code) {
    case 0:
      return Low;
    case 1:
      return High;
    default:
      raise_error("Invalid priority code: $", code);
    }
  }

  const char* to_string() const
  {
    switch (_e) {
    case Low:
      return "low";
    case High:
      return "high";
    }
  }

  auto operator<=>(const Priority& p) const = default;

 private:
  E _e;
};

////////////////////////////////////////////////////////////////////////////////
// Sprite
//

struct Sprite {
 public:
  int y() const { return _y; }
  int x() const { return _x; }
  int height() const { return _height; }
  int width() const { return _width; }
  Priority priority() const { return _priority; }
  ulong_t palette() const { return _palette; }
  ulong_t tiles_addr() const { return _tiles_addr; }
  ulong_t next() const { return _next; }

  bool yflip() const { return _yflip; }
  bool xflip() const { return _xflip; }

  Sprite(BW w1, BW w2, BW w3, BW w4)
      : _y(w1.get(0, 9)),
        _x(w4.get(0, 9)),
        _height(parse_size(w2.get(8, 2))),
        _width(parse_size(w2.get(10, 2))),
        _priority(Priority::of_code(w3.get(15))),
        _palette(w3.get(13, 2)),
        _yflip(w3.get(12) == 1),
        _xflip(w3.get(11) == 1),
        _tiles_addr(ulong_t(w3.get(0, 11)) * 0x20),
        _next(w2.get(0, 7))
  {}

  std::string to_string() const
  {
    return F(
      "y:$ x:$ height:$ width:$ priority:$ palette:$ yflip:$ xflip:$ "
      "tiles_addr:{x} "
      "next:$",
      y(),
      x(),
      height(),
      width(),
      priority(),
      palette(),
      yflip(),
      xflip(),
      tiles_addr(),
      next());
  }

 private:
  static ulong_t parse_size(ulong_t size) { return size + 1; }
  ulong_t _y;
  ulong_t _x;
  ulong_t _height;
  ulong_t _width;
  Priority _priority;
  ulong_t _palette;

  bool _yflip;
  bool _xflip;

  ulong_t _tiles_addr;
  ulong_t _next;
};

////////////////////////////////////////////////////////////////////////////////
// PlaneCell
//

struct PlaneCell {
 public:
  PlaneCell(BW w)
      : _priority(Priority::of_code(w.get(15))),
        _palette(w.get(13, 2)),
        _yflip(w.get(12) == 1),
        _xflip(w.get(11) == 1),
        _tile_addr(ulong_t(w.get(0, 11)) * 0x20)
  {}

  Priority priority() const { return _priority; }
  ulong_t palette() const { return _palette; }
  bool yflip() const { return _yflip; }
  bool xflip() const { return _xflip; }
  ulong_t tile_addr() const { return _tile_addr; }

  std::string to_string() const
  {
    return F(
      "priority:$ palette:$ yflip:$ xflip:$ tile_addr:{x}",
      priority(),
      palette(),
      yflip(),
      xflip(),
      tile_addr());
  }

 private:
  Priority _priority;
  ulong_t _palette;
  bool _yflip;
  bool _xflip;
  ulong_t _tile_addr;
};

////////////////////////////////////////////////////////////////////////////////
// VDPRegisters
//

struct VDPRegisters {
 public:
  VDPRegisters(bool verbose) : _verbose(verbose) { _reg.fill(0); }
  void write(int idx, ubyte_t b)
  {
    at(idx) = b;
    if (_verbose) P("VDP: R{x} <- 0x{x}", idx, b);
  }

  ulong_t sprite_table_addr() const { return at(5).get(0, 7) * 0x200; }

  ulong_t plane_width() const { return _code_to_size(at(0x10).get(0, 2)); }
  ulong_t plane_height() const { return _code_to_size(at(0x10).get(4, 2)); }

  ulong_t plane_addr(Plane plane) const
  {
    switch (plane) {
    case Plane::Foreground:
      return at(2).get(3, 3) * 0x2000;
    case Plane::Background:
      return at(4).get(0, 3) * 0x2000;
    case Plane::Window:
      return at(3).get(1, 5) * 0x800;
    }
  }

  ulong_t access_stride() const { return at(0xf); }

  VDPMode mode() const { return VDPMode(at(0), at(1)); }

  ulong_t src_addr() const
  {
    ulong_t src = at(0x17).get(7, 1);
    // 0 means ram
    ubyte_t mask = src == 0 ? 0x7f : 0x3f;
    ulong_t out;
    out = ulong_t(at(0x15));
    out |= ulong_t(at(0x16)) << 8;
    out |= ulong_t(at(0x17) & mask) << 16;
    out *= 2;
    return out;
  }

  ulong_t tx_length() const
  {
    return (at(0x13) | (ulong_t(at(0x14)) << 8)) * 2;
  }

  bool tx_is_fill() const { return (at(0x17).get(6, 2)) == 2; }

  HScrollKind hscroll_kind() const
  {
    return HScrollKind::of_code(at(0xb).get(0, 2));
  }

  VScrollKind vscroll_kind() const
  {
    return VScrollKind::of_code(at(0xb).get(2, 1));
  }

  ulong_t hscroll_addr() const { return at(0xd).get(0, 6) * 0x400; }

  ulong_t window_x() const { return at(0x11).get(0, 5) * 2 * 8; }
  ulong_t window_y() const { return at(0x12).get(0, 5) * 8; }

  bool window_right() const { return at(0x11).get(7, 1) == 1; }
  bool window_bottom() const { return at(0x11).get(7, 1) == 1; }

  inline BB at(int idx) const { return _reg.at(idx); }
  inline BB& at(int idx) { return _reg.at(idx); }

 private:
  static ulong_t _code_to_size(ulong_t code)
  {
    switch (code) {
    case 0:
      return 32;
    case 1:
      return 64;
    case 3:
      return 128;
    default:
      raise_error("Invalid plane size code: $", code);
    }
  };
  std::array<BB, NUM_VDP_REGS> _reg;

  bool _verbose;
};

////////////////////////////////////////////////////////////////////////////////
// VDPImpl
//

struct VDPImpl final : public VDP {
 public:
  VDPImpl(bool verbose) : _reg(verbose), _verbose(verbose)
  {
    _vram.fill(0);
    _cram.fill(0);
    _vsram.fill(0);
  }

  void set_bus(const IOIntf::ptr& io) override { _bus = io; }

  ~VDPImpl() {}

  void dump_memory(int dump_idx) const override
  {
    if (_verbose) P("---------");

    const int img_height = 256;
    const int img_width = 512;
    pixel::Image img(img_height, img_width);

    ulong_t tile_size = 32;
    ulong_t addr = 0;
    for (int y = 0; y < img_height; y += TILE_SIZE) {
      for (int x = 0; x < img_width; x += TILE_SIZE) {
        if (addr >= VRAM_SIZE - tile_size) { break; }
        render_tile(img, addr, 0, y, x, false, false);
        addr += tile_size;
      }
    }

    img.save_pnm(bee::FilePath(F("dump_{05}.pnm", dump_idx)));
  }

  void render_sprite(pixel::Image& img, const Sprite sprite, int y, int x) const
  {
    ulong_t addr = sprite.tiles_addr();
    for (int cx = 0; cx < sprite.width(); cx++) {
      int x0 = x + (sprite.xflip() ? sprite.width() - 1 - cx : cx) * TILE_SIZE;
      for (int cy = 0; cy < sprite.height(); cy++) {
        int y0 =
          y + (sprite.yflip() ? sprite.height() - 1 - cy : cy) * TILE_SIZE;
        render_tile(
          img, addr, sprite.palette(), y0, x0, sprite.yflip(), sprite.xflip());
        addr += 32;
      }
    }
  }

  std::vector<Sprite> get_sprites() const
  {
    ulong_t table_addr = _reg.sprite_table_addr();

    std::vector<Sprite> sprites;
    int sprite_idx = 0;
    for (int idx = 0; sprite_idx < MAX_SPRITES; sprite_idx++) {
      ulong_t sprite_addr = table_addr + idx * 8;
      auto w1 = _vram.at((sprite_addr + 0) / 2);
      auto w2 = _vram.at((sprite_addr + 2) / 2);
      auto w3 = _vram.at((sprite_addr + 4) / 2);
      auto w4 = _vram.at((sprite_addr + 6) / 2);
      sprites.emplace_back(w1, w2, w3, w4);
      idx = sprites.back().next();
      if (idx == 0) { break; }
    }
    std::reverse(sprites.begin(), sprites.end());
    return sprites;
  }

  void dump_sprites(int dump_idx) const override
  {
    const int img_size = 16 * 8 * 4;
    pixel::Image img(img_size, img_size);

    int idx = 0;
    auto sprites = get_sprites();
    for (auto& sprite : sprites) {
      P("VDP: Sprite: $", sprite);

      render_sprite(
        img, sprite, (idx / 16) * TILE_SIZE * 4, (idx % 16) * TILE_SIZE * 4);
      idx++;
    }
    P("VDP: Num sprites: $", sprites.size());

    img.save_pnm(bee::FilePath(F("sprites_{05}.pnm", dump_idx)));
  }

  void render_sprites(pixel::Image& img, Priority priority) const
  {
    auto sprites = get_sprites();
    for (auto& sprite : sprites) {
      if (sprite.priority() != priority) continue;
      render_sprite(img, sprite, sprite.y() - 128, sprite.x() - 128);
    }
    if (_verbose) P("VDP: Num sprites: $", sprites.size());
  }

  int hscroll_amount(Plane plane) const
  {
    ulong_t addr = _reg.hscroll_addr();
    auto hscroll = _reg.hscroll_kind();
    switch (hscroll) {
    case HScrollKind::WholeScreen:
      switch (plane) {
      case Plane::Foreground:
        break;
      case Plane::Background:
        addr += 2;
        break;
      case Plane::Window:
        raise_error("Window plane does not scroll");
      }
      break;
    default:
      raise_error("Unsupported hscroll kind: $", hscroll);
    }
    if (_verbose) P("VDP: HScroll addr: {x}", addr);
    return BW(_vram.at(addr / 2)).get(0, 10);
  }

  int vscroll_amount(Plane plane) const
  {
    ulong_t addr = 0;
    auto vscroll = _reg.vscroll_kind();
    switch (vscroll) {
    case VScrollKind::WholeScreen:
      switch (plane) {
      case Plane::Foreground:
        break;
      case Plane::Background:
        addr += 2;
        break;
      case Plane::Window:
        raise_error("Window plane does not scroll");
      }
      break;
    default:
      raise_error("Unsupported vscroll kind: $", vscroll);
    }
    return BW(_vsram.at(addr / 2)).get(0, 10);
  }

  void render_tile(
    pixel::Image& img,
    int tile_addr,
    int palette_idx,
    int y,
    int x,
    bool yflip,
    bool xflip) const
  {
    ulong_t paddr = palette_idx * 0x20;
    auto get_color = [&](int color) -> BW {
      return _cram.at((paddr + color * 2) / 2);
    };
    for (int dy = 0; dy < TILE_SIZE; dy++) {
      for (int dx = 0; dx < TILE_SIZE; dx++) {
        int px = xflip ? x - dx + 7 : x + dx;
        int py = yflip ? y - dy + 7 : y + dy;

        if (px < 0 || px >= img.width() || py < 0 || py >= img.height()) {
          continue;
        }

        int nb_addr = tile_addr * 2 + dy * TILE_SIZE + dx;
        BW word = _vram.at(nb_addr / 4);
        auto color_idx = word.get((3 - (nb_addr % 4)) * 4, 4);
        if (color_idx == 0) { continue; }
        BW color = get_color(color_idx);
        img.set_pixel(
          py,
          px,
          color.get(1, 3) * 36,
          color.get(5, 3) * 36,
          color.get(9, 3) * 36);
      }
    }
  }

  void render_plane_cell(
    pixel::Image& img, ulong_t addr, int y, int x, Priority priority) const
  {
    PlaneCell cell(_vram.at(addr / 2));
    if (cell.priority() != priority) return;
    render_tile(
      img, cell.tile_addr(), cell.palette(), y, x, cell.yflip(), cell.xflip());
  }

  void render_plane(pixel::Image& img, Plane plane, Priority priority) const
  {
    auto plane_addr = _reg.plane_addr(plane);
    int height = _reg.plane_height();
    int width = _reg.plane_width();
    int scroll_x = hscroll_amount(plane);
    int scroll_y = vscroll_amount(plane);
    if (_verbose) {
      P("VDP: Render plane: $", plane);
      P("VDP: Plane size: $x$", height, width);
      P("VDP: Plane addr: [{x}:{x}[",
        plane_addr,
        plane_addr + height * width * 2);
      P("VDP: Scroll: $x$", scroll_y, scroll_x);
    }

    for (int y = 0; y <= img.height(); y += TILE_SIZE) {
      int ay = DIV((y + scroll_y), TILE_SIZE);
      int cy = MOD(ay, height);
      int ty = ay * TILE_SIZE - scroll_y;
      for (int x = 0; x <= img.width(); x += TILE_SIZE) {
        int ax = DIV((x - scroll_x), TILE_SIZE);
        int cx = MOD(ax, width);
        int tx = ax * TILE_SIZE + scroll_x;

        ulong_t tile_addr = plane_addr + (cx + cy * width) * 2;
        render_plane_cell(img, tile_addr, ty, tx, priority);
      }
    }
  }

  void render_window(pixel::Image& img, Priority priority) const
  {
    if (_verbose) {
      P("VDP: Render plane: Window");
      P("VDP: Bottom: $", _reg.window_bottom());
      P("VDP: Right: $", _reg.window_right());
    }
    auto addr = _reg.plane_addr(Plane::Window);
    int y0, y1;
    int x0, x1;
    if (_reg.window_bottom()) {
      y0 = _reg.window_y();
      y1 = SCREEN_HEIGHT;
    } else {
      y0 = 0;
      y1 = _reg.window_y();
    }

    // if (_reg.window_right()) {
    //   x0 = _reg.window_x();
    //   x1 = SCREEN_WIDTH;
    // } else {
    //   x0 = 0;
    //   x1 = _reg.window_x();
    // }

    // TODO: this is hack because the above is not working
    x0 = 0;
    x1 = SCREEN_WIDTH;

    int height = _reg.plane_height();
    int width = _reg.plane_width();
    if (_verbose) {
      P("VDP: Plane pos: $:$x$:$", y0, y1, x0, x1);
      P("VDP: Plane addr: [{x}:{x}[", addr, addr + (height * width * 2) / 64);
    }
    for (int y = y0; y < y1; y += TILE_SIZE) {
      uword_t line_addr = addr + width * y / 4;
      for (int x = x0; x < x1; x += TILE_SIZE) {
        uword_t cell_addr = line_addr + x / 4;
        render_plane_cell(img, cell_addr, y, x, priority);
      }
    }
  }

  pixel::Image render() const override
  {
    // TODO: PAL has a diff resolution
    pixel::Image img(SCREEN_HEIGHT, SCREEN_WIDTH);

    if (_verbose) P("VDP: Render");

    for (auto pri : {Priority::Low, Priority::High}) {
      render_plane(img, Plane::Background, pri);
      render_plane(img, Plane::Foreground, pri);
      render_sprites(img, pri);
      render_window(img, pri);
    }

    return img;
  }

  bool vblank_enabled() const override
  {
    return _reg.mode().vertical_interrupts();
  }

  void save_state(bee::Writer& writer) override
  {
    save_state_gen(_reg, writer);
    save_state_array(_vram, writer);
    save_state_array(_cram, writer);
    save_state_array(_vsram, writer);
    save_state_gen(_partial_ctrl, writer);
    save_state_gen(_cmd_hi, writer);
    save_state_gen(_transfer, writer);
  }

  void load_state(bee::Reader& reader) override
  {
    load_state_gen(_reg, reader);
    load_state_array(_vram, reader);
    load_state_array(_cram, reader);
    load_state_array(_vsram, reader);
    load_state_gen(_partial_ctrl, reader);
    load_state_gen(_cmd_hi, reader);
    load_state_gen(_transfer, reader);
  }

 protected:
  ubyte_t _b(ulong_t) override { raise_error("Can't read byte from VDP"); }

  uword_t _w(ulong_t addr) override
  {
    switch (addr) {
    case VDP_CTRL1:
    case VDP_CTRL2: {
      auto mode = _reg.mode();
      VDPStatusRegister r{
        .fifo_empty = true,
        .fifo_full = false,
        .vblank_progress = true,
        .pal = mode.pal_mode(),
      };
      return r.to_word();
    }
    case VDP_DATA1:
    case VDP_DATA2: {
      return _read_data();
    }
    default:
      raise_error("Invalid vdp word read address: {06x}", addr);
    }
  }

  ulong_t _l(ulong_t addr) override
  {
    raise_error("Invalid vdp long read address: {06x}", addr);
  }

  void _b(ulong_t, ubyte_t) override { raise_error("Can't write byte to VDP"); }

  void _w(ulong_t addr, uword_t v) override
  {
    switch (addr) {
    case VDP_CTRL1:
    case VDP_CTRL2:
      _write_ctrl_word(v);
      break;
    case VDP_DATA1:
    case VDP_DATA2:
      _write_data(v);
      break;
    default:
      raise_error("Invalid vdp word write address: {06x}", addr);
    }
  }

  void _l(ulong_t addr, ulong_t v) override
  {
    switch (addr) {
    case VDP_CTRL1:
      _write_ctrl_long(v);
      break;
    case VDP_DATA1:
      _write_data(v >> 16);
      _write_data(v);
      break;
    default:
      raise_error("Invalid vdp long write address: {06x}", addr);
    }
  }

 private:
  void _write_ctrl_word(uword_t v)
  {
    if (_partial_ctrl) {
      _partial_ctrl = false;
      _execute_long_cmd(ulong_t(_cmd_hi) << 16 | v);
    } else if (is_word_cmd(v)) {
      _execute_word_cmd(v);
    } else {
      _partial_ctrl = true;
      _cmd_hi = v;
    }
  }

  void _write_ctrl_long(ulong_t v)
  {
    if (is_long_cmd(v)) {
      _execute_long_cmd(v);
    } else {
      _execute_word_cmd(v >> 16);
      _execute_word_cmd(bmask(v, 16));
    }
  }

  void _write_data(uword_t word)
  {
    if (_transfer.has_value()) {
      if (_transfer->dma && _transfer->fill) {
        if (_verbose)
          P("VDP: dma-fill: dst_addr:{x} length:$",
            _transfer->dst_addr,
            _transfer->length);
        for (ulong_t i = 0; i < _transfer->length; i += 2) {
          _write_vdp(_transfer->dst, _transfer->dst_addr + i, word);
        }
        _transfer.reset();
      } else if (!_transfer->dma) {
        _write_vdp(_transfer->dst, _transfer->dst_addr, word);
        _transfer->dst_addr += _reg.access_stride();
      } else {
        raise_error("Unsupported vdp mode");
      }
    } else {
      raise_error("VDP: got data without active transfer: {04x}", word);
    }
  }

  ulong_t _read_data()
  {
    if (_transfer.has_value()) {
      if (!_transfer->dma && _transfer->dst == VDPTarget::DATA) {
        ulong_t ret = _read_vdp(_transfer->src, _transfer->dst_addr);
        _transfer->dst_addr += _reg.access_stride();
        return ret;
      } else {
        raise_error("Unsupported vdp mode when reading data");
      }
    } else {
      raise_error("VDP: got data read without active transfer");
    }
  }

  void _execute_word_cmd(ulong_t cmd)
  {
    if (!is_word_cmd(cmd)) {
      raise_error("VDP: Invalid word cmd: {x}", cmd);
      return;
    }
    // set register
    ubyte_t data = bmask(cmd, 8);
    cmd >>= 8;
    ulong_t reg_idx = bmask(cmd, 5);
    _reg.write(reg_idx, data);
    switch (reg_idx) {
    case 0:
    case 1:
      if (_verbose) P("VDP Mode: $", _reg.mode());
    }
  }

  void _execute_long_cmd(ulong_t cmd)
  {
    if (!is_long_cmd(cmd)) {
      raise_error("VDP: Invalid long cmd: {x}", cmd);
      return;
    }
    BL cmdb = cmd;
    ulong_t addr_hi = cmdb.get(0, 2);
    ulong_t mode_hi = cmdb.get(4, 2);
    bool vram_to_vram = cmdb.get(6) == 1;
    bool dma = cmdb.get(7) == 1;
    ulong_t addr_low = cmdb.get(16, 14);
    ulong_t mode_lo = cmdb.get(30, 2);

    ulong_t dst_addr = (addr_hi << 14) | addr_low;
    ulong_t mode_code = (mode_hi << 2) | mode_lo;
    auto dst = VDPTarget::of_code(mode_code);
    auto rw = VDPRW::of_code(mode_code);
    bool fill = dma && _reg.tx_is_fill();
    auto length = _reg.tx_length();

    VDPTarget src = [&]() {
      if (vram_to_vram) {
        return VDPTarget::VRAM;
      } else if (dma && fill) {
        return VDPTarget::DATA;
      } else if (dma && !fill) {
        return VDPTarget::BUS;
      } else {
        return VDPTarget::DATA;
      }
    }();

    if (rw == VDPRW::Read) { std::swap(src, dst); }

    if (_verbose)
      P("VDP: cmd: src:$ dst:$ dma:$ dst_addr:{x} fill:$ length:$",
        src,
        dst,
        dma,
        dst_addr,
        fill,
        length);

    Transfer transfer{
      .dst = dst,
      .src = src,
      .dst_addr = dst_addr,
      .length = length,
      .dma = dma,
      .fill = fill,
    };

    _transfer.reset();
    if (dma && !fill) {
      _dma_copy(transfer);
    } else {
      _transfer.emplace(transfer);
    }
  }

  void _dma_copy(const Transfer& transfer)
  {
    ulong_t length = _reg.tx_length();
    ulong_t src_addr = _reg.src_addr();

    if (_verbose)
      P("VDP: dma: src:$ dst:$ src_addr:{x} dst_addr:{x} length:$",
        transfer.src,
        transfer.dst,
        src_addr,
        transfer.dst_addr,
        length);

    for (ulong_t i = 0; i < length; i += 2) {
      ulong_t as = src_addr + i;
      ulong_t ad = transfer.dst_addr + i;
      _write_vdp(transfer.dst, ad, _read_vdp(transfer.src, as));
    }
  }

  void _write_vdp(VDPTarget dest, ulong_t addr, uword_t data)
  {
    if (_verbose)
      if (dest != VDPTarget::BUS) {
        P("VDP: $.W <- {x}", dest.format_addr(addr), data);
      }
    if (addr % 2 == 1) { raise_error("Invalid odd write address: {x}", addr); }
    auto waddr = addr / 2;
    switch (dest) {
    case VDPTarget::VRAM:
      _vram.at(waddr) = data;
      break;
    case VDPTarget::CRAM:
      _cram.at(waddr) = data;
      break;
    case VDPTarget::VSRAM:
      _vsram.at(waddr) = data;
      break;
    case VDPTarget::BUS:
      _bus->w(addr, data);
      break;
    case VDPTarget::DATA:
      raise_error("Cannot write to DATA destination");
    }
  }

  uword_t _read_vdp(VDPTarget dest, ulong_t addr) const
  {
    if (addr % 2 == 1) { raise_error("Invalid odd read address: {x}", addr); }
    auto waddr = addr / 2;
    uword_t data;
    switch (dest) {
    case VDPTarget::VRAM:
      data = _vram.at(waddr);
      break;
    case VDPTarget::CRAM:
      data = _cram.at(waddr);
      break;
    case VDPTarget::VSRAM:
      data = _vsram.at(waddr);
      break;
    case VDPTarget::BUS:
      data = _bus->w(addr);
      break;
    case VDPTarget::DATA:
      raise_error("Cannot read from DATA destination");
    }
    if (_verbose)
      if (dest != VDPTarget::BUS) {
        P("VDP: $.W -> {x}", dest.format_addr(addr), data);
      }
    return data;
  }

  VDPRegisters _reg;

  std::array<ulong_t, VRAM_SIZE / 2> _vram;
  std::array<ulong_t, 0x40> _cram;
  std::array<ulong_t, 0x28> _vsram;

  bool _partial_ctrl = false;
  ulong_t _cmd_hi;

  std::optional<Transfer> _transfer;

  std::shared_ptr<IOIntf> _bus;

  bool _verbose;
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// VDP
//

VDP::~VDP() {}

std::shared_ptr<VDP> VDP::create(bool verbose)
{
  return std::make_shared<VDPImpl>(verbose);
}

} // namespace heaven_ice
