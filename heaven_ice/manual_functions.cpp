#include "manual_functions.hpp"

#include "inst_impls.hpp"
#include "magic_constants.hpp"

#include "bee/print.hpp"

namespace heaven_ice {
namespace {

constexpr int SOME_TILE_ANIMATION_ADDR = 0x5f18;

struct DMARequest {
  uword_t size;
  uword_t dst_addr;
  ulong_t src_addr;
};

struct ManualFunctionsImpl final : public ManualFunctions {
  ~ManualFunctionsImpl() {}

  inline void inc_mem_w(ulong_t addr, sword_t v)
  {
    G.io->w(addr, G.io->w(addr) + v);
  }

  void set_generated(const GeneratedIntf::ptr& generated) override
  {
    _g = generated;
  }

  void set_d0(ulong_t v)
  {
    // Yes, updating cc is important here, ugh
    G.d[0].l(UCC(v));
  }

  void vdp_control_w(W v) { G.io->w(VDP_CTRL1, v); }
  void vdp_control_l(L v) { G.io->l(VDP_CTRL1, v); }

  void send_data_w(UW v) { G.io->w(VDP_DATA1, v); }
  void send_data_w(UW v1, UW v2) { G.io->l(VDP_DATA1, UL(v1) << 16 | UL(v2)); }

  void set_vdp_reg(UW reg_id, UB data)
  {
    vdp_control_w(0x8000 | (reg_id << 8) | data);
  }

  template <class T> void write_mem_inc(ulong_t& addr, T value)
  {
    G.io->write<T>(addr, value);
    addr += sizeof(T);
  }

  void start_write_data_to_vram(ulong_t addr)
  {
    ulong_t cmd = 1 << 30; // magic constant for DATA dst
    cmd |= (addr >> 14) & 0x3;
    cmd |= (addr & 0x3fff) << 16;
    vdp_control_l(cmd);
  }

  void clear_d0() override { set_d0(0); }

  void set_d0_1() override { set_d0(1); }

  void neg_w_d2_exg_d2_d3() override
  {
    _log_call(__func__);
    neg_w_d2();
    exg_d2_d3();
    _log_ret(__func__);
  }

  void exg_d2_d3() override
  {
    _log_call(__func__);
    std::swap(G.d[2], G.d[3]);
    _log_ret(__func__);
  }

  void noop() override {}

  void exg_d2_d3_neg_w_d2() override
  {
    _log_call(__func__);
    exg_d2_d3();
    neg_w_d2();
    _log_ret(__func__);
  }

  void neg_w_d2() override
  {
    _log_call(__func__);
    G.d[2].w(NEG<W>(G.d[2].w()));
    _log_ret(__func__);
  }

  void neg_w_d3() override
  {
    _log_call(__func__);
    G.d[3].w(NEG<W>(G.d[3].w()));
    _log_ret(__func__);
  }

  void neg_w_d2_d3() override
  {
    _log_call(__func__);
    neg_w_d2();
    neg_w_d3();
    _log_ret(__func__);
  }

  void exg_neg_w_d2_d3() override
  {
    _log_call(__func__);
    exg_d2_d3();
    neg_w_d2_d3();
    _log_ret(__func__);
  }

  void wait_for_z80_bus() override {}
  void z80_useless() override {}

  void enable_vblank_int() override
  {
    _log_call(__func__);
    set_vdp_reg(0, 4);
    set_vdp_reg(1, 0x64);
    G.sr.set_from_int(0x2500);
    _log_ret(__func__);
  }

  void vblank(int count) override
  {
    _log_call(__func__);
    vblank_real(count + 1);
    _log_ret(__func__);
  }

  void vblank_real(int count)
  {
    for (int i = 0; i < count; i++) { G.vblank(); }
  }

  void clear_cram() override
  {
    _log_call(__func__);
    vdp_control_l(WRITE_TO_CRAM);
    for (int i = 0; i < 64; i++) { send_data_w(0); }
    _log_ret(__func__);
  }

  void vdp_set_d3_blocks_of_size_d2_with_d0_starting_at_d1(
    uword_t fill, ulong_t vram_addr, uword_t d2, uword_t d3) override
  {
    _log_call(__func__);

    L cmd = ((vram_addr & 0x3fff) | (1 << 14)) << 16;

    int size = d2 + 1;
    int blocks = d3 + 1;

    for (int i = 0; i < blocks; i++) {
      vdp_control_l(cmd);
      for (int j = 0; j < size; j++) { send_data_w(fill); }
      cmd += 1 << 23;
    }
    _log_ret(__func__);
  }

  void vdp_copy_words_to_cram(
    const int count, const ulong_t addr, const ulong_t caddr) override
  {
    _log_call(__func__);

    vdp_control_l(caddr << 16 | 0xc0000000);
    for (int i = 0; i < count; i++) { send_data_w(G.io->w(addr + i * 2)); }

    _log_ret(__func__);
  }

  void some_weird_transformation(int count)
  {
    _log_call(__func__);
    L src = G.a[0];
    L dst = G.a[1];

    for (int i = 0; i < count; i++) {
      uword_t s = G.io->w(src);
      uword_t d = G.io->w(dst);

      for (int j = 0; j < 3; j++) {
        uword_t a = s & 0xe;
        uword_t b = d & 0xe;
        if (b < a) { d += 2; }

        s = ROR<UW>(s, 4);
        d = ROR<UW>(d, 4);
      }
      d = ROR<UW>(d, 4);
      G.io->w(dst, d);

      src += 2;
      dst += 2;
    }

    G.a[0] = src;
    G.a[1] = dst;
    _log_ret(__func__);
  }

  void some_weird_transformation(sword_t d0) override
  {
    _log_call(__func__);
    some_weird_transformation(d0 + 1);
    _log_ret(__func__);
  }

  void some_weird_transformation_16_times() override
  {
    _log_call(__func__);
    some_weird_transformation(16);
    _log_ret(__func__);
  }

  void clear_vscroll() override
  {
    _log_call(__func__);
    G.io->l(VSCROLL_FG, 0);
    G.io->l(VSCROLL_BG, 0);
    _log_ret(__func__);
  }

  void disable_vblank_int() override
  {
    // Don't need to do anything here
  }

  void push_scroll_state() override
  {
    _log_call(__func__);
    vdp_control_l(WRITE_TO_VSRAM);
    send_data_w(G.io->w(VSCROLL_FG), G.io->w(VSCROLL_BG));
    start_write_data_to_vram(0x3000);
    send_data_w(G.io->w(HSCROLL_FG), G.io->w(HSCROLL_BG));
    _log_ret(__func__);
  }

  void add_to_dma_queue(
    uword_t size, ulong_t src_addr, uword_t dst_addr) override
  {
    _log_call(__func__);

    _dma_queue.push_back(
      {.size = size, .dst_addr = dst_addr, .src_addr = src_addr});

    _log_ret(__func__);
  }

  void enqueue_some_tile_animation_to_dma() override
  {
    _log_call(__func__);

    ulong_t d0 = G.io->w(SOME_STATE_COUNTER);
    if ((d0 & 1) == 0) {
      ulong_t addr = SOME_TILE_ANIMATION_ADDR + ((d0 & 2) << 2);
      uword_t size = G.io->w(addr);
      ulong_t src_addr = G.io->l(addr + 2);
      uword_t dst_addr = G.io->w(addr + 6);
      add_to_dma_queue(size, src_addr, dst_addr);
    }

    _log_ret(__func__);
  }

  void dma_push(uword_t size, ulong_t src_addr, ulong_t dst_addr) override
  {
    _log_call(__func__);

    // encode dst address in vdp command and set write mode
    ulong_t cmd =
      ((dst_addr << 21) & 0x3fff0000) | ((dst_addr >> 9) & 0x3) | 0x40000080;

    // set dma length
    uword_t length = size << 4;
    set_vdp_reg(0x13, length);
    set_vdp_reg(0x14, length >> 8);

    // set src addr
    src_addr >>= 1;
    set_vdp_reg(0x15, src_addr);
    set_vdp_reg(0x16, src_addr >> 8);
    set_vdp_reg(0x17, src_addr >> 16);

    // dma copy
    vdp_control_l(cmd);

    _log_ret(__func__);
  }

  void flush_dma() override
  {
    _log_call(__func__);

    dma_push(0x14, SPRITE_TABLE, 0x1a0);

    for (auto& r : _dma_queue) { dma_push(r.size, r.src_addr, r.dst_addr); }
    _dma_queue.clear();

    _log_ret(__func__);
  }

  void copy_something_to_vdp(ulong_t src_addr, ulong_t dst_addr) override
  {
    _log_call(__func__);

    start_write_data_to_vram(dst_addr);

    for (int i = 0;; i++) {
      sword_t d = G.io->w(src_addr + i * 2);
      if (d == -1) { break; }
      send_data_w(d);
    }

    _log_ret(__func__);
  }

  void update_sprite_with_something(ulong_t a6) override
  {
    _log_call(__func__);

    ulong_t a0 = SPRITE_TABLE + G.io->w(a6 + 14);
    uword_t d0 = G.io->w(a6 + 0x16) & 0xff80;
    d0 += G.io->w(a6 + 0x22);
    d0 >>= 7;
    d0 = 0xff60 - d0;
    G.io->w(a0, d0);
    a0 += 2;
    G.io->w(a0, G.io->w(a0) & 0x7f);
    d0 = G.io->w(a6 + 0x12) & 0xf00;
    G.io->w(a0, G.io->w(a0) | d0);
    a0 += 2;
    d0 = G.io->w(a6 + 0x10);
    if (((G.io->b(a6 + 1) >> 6) & 1) != 0) { d0 &= 0x9fff; }

    G.io->w(a0, d0);
    a0 += 2;
    d0 = G.io->w(a6 + 0x14) & 0xff80;
    d0 += G.io->w(a6 + 0x20);
    d0 = (d0 >> 7) + 0x80;
    G.io->w(a0, d0);

    _log_ret(__func__);
  }

  void start() override
  {
    _log_call(__func__);

    G.a[7] = RAM_END;
    G.sr.set_from_int(0x2700);

    // Set vdp registers initial value
    constexpr ubyte_t values[] = {
      0x04, 0x04, 0x00, 0x04, 0x01, 0x1a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
      0x81, 0x0c, 0x00, 0x02, 0x01, 0x14, 0x1e, 0xff, 0xff, 0x00, 0x00, 0x80,
    };
    for (int i = 0; i < NUM_VDP_REGS; i++) { set_vdp_reg(i, values[i]); }

    // populate some initial state
    G.io->l(0xff002a, 0x12500);
    G.io->w(SOME_STATE_CONTROL, 0xffff);

    // Store console version
    uword_t version = G.io->b(VERSION_REGISTER) & 0x80;
    G.io->w(RAM_BEGIN, version >> 5);

    // Wait vblank not sure what for
    vblank_real(6);

    // Unknown
    G.io->w(SOME_STATE_CONTROL, 0);
    G.io->w(0xff0004, 0);
    clear_all_planes();
    clear_sprites();

    dma_push(0x3e, 0x3b480, 0x3b4);

    main_loop();

    _log_ret(__func__);
  }

  void clear_sprites() override
  {
    _log_call(__func__);

    for (int i = 0; i < 80; i++) {
      const ulong_t addr = SPRITE_TABLE + i * 8;
      G.io->w(addr, 0);
      G.io->w(addr + 2, (1 + i) % 80);
      G.io->w(addr + 4, 0x3e1);
      G.io->w(addr + 6, 0);
    }

    flush_dma();

    _log_ret(__func__);
  }

  void clear_window_plane() override
  {
    _log_call(__func__);

    vdp_set_d3_blocks_of_size_d2_with_d0_starting_at_d1(
      0x83e2, 0x1000, 0x27, 0x1f);

    set_vdp_reg(0x11, 0x14);
    set_vdp_reg(0x12, 0x1e);

    _log_ret(__func__);
  }

  void clear_all_planes() override
  {
    _log_call(__func__);

    clear_window_plane();

    vdp_set_d3_blocks_of_size_d2_with_d0_starting_at_d1(0x83e1, 0, 0x3f, 0x1f);

    vdp_set_d3_blocks_of_size_d2_with_d0_starting_at_d1(
      0x83e1, 0x2000, 0x3f, 0x1f);

    clear_vscroll();
    push_scroll_state();

    _log_ret(__func__);
  }

  void main_loop()
  {
    _log_call(__func__);

    constexpr ulong_t magic_addr1 = 0xff2a9c;
    constexpr ulong_t magic_table = 0xff2aa4;

    while (true) {
      G.d[0].l(0);
      // 00048a: MOVE.W dst:D0 src:(SOME_OTHER_FUCKING_COUNTER)
      // 000490: ROL.W dst:D0 src:#3
      G.d[0].w(ROL<W>(G.io->w(SOME_OTHER_FUCKING_COUNTER), 3));

      // 000492: LEA.L dst:A0 src:(magic_table)
      // 000498: ADDA.L dst:A0 src:D0
      G.a[0] = magic_table + G.d[0].l();

      // 00049a: BTST.B dst:(A0) src:#7
      BTST<B>(G.io->b(G.a[0]), 7);
      if (!G.sr.check_condition(Condition::EQ)) {
        // 0004a0: LEA.L dst:A1 src:(magic_addr1)
        G.a[1] = magic_addr1;
        // 0004a6: MOVEM.L dst:-(USP) regs:A1,A0
        G.push(SizeKind::l(), G.a[1]);
        G.push(SizeKind::l(), G.a[0]);
        // 0004aa: MOVE.L dst:(A1)+ src:(A0)+
        G.io->l(G.a[1], G.io->l(G.a[0]));
        G.a[0] += 4;
        G.a[1] += 4;
        // 0004ac: MOVE.L dst:(A1)+ src:(A0)+
        G.io->l(G.a[1], G.io->l(G.a[0]));
        G.a[0] += 4;
        G.a[1] += 4;

        // 0004ae: LEA.L dst:A0 src:(542)
        G.a[0] = 0x542;
        // 0004b4: ROR.W dst:D0 src:#1
        G.d[0].w(ROR<W>(G.d[0].w(), 1));
        // 0004b6: JSR src:(A0,D0.L)+0
        _g->jump_map(G.a[0] + G.d[0].l());

        // 0004ba: MOVEM.L src:(USP)+ regs:A0,A1
        G.a[0] = G.pop(SizeKind::l());
        G.a[1] = G.pop(SizeKind::l());
        // 0004be: MOVE.L dst:(A0)+ src:(A1)+
        G.io->l(G.a[0], G.io->l(G.a[1]));
        G.a[1] += 4;
        G.a[0] += 4;
        // 0004c0: MOVE.L dst:(A0)+ src:(A1)+
        G.io->l(G.a[0], G.io->l(G.a[1]));
        G.a[1] += 4;
        G.a[0] += 4;
      }

      // 0004c2: LEA.L dst:A0 src:(SOME_OTHER_FUCKING_COUNTER)
      G.a[0] = SOME_OTHER_FUCKING_COUNTER;
      // 0004c8: ADDQ.W dst:(A0) src:#1
      G.io->w(G.a[0], G.io->w(G.a[0]) + 1);
      // 0004ca: CMPI.W dst:(A0) src:#28
      CMP<W>(G.io->w(G.a[0]), 0x28);
      // 0004ce: Bcc cond:NE src:(488)
      if (!G.sr.check_condition(Condition::NE)) {
        // 0004d0: CLR.W dst:(A0)
        G.io->w(G.a[0], 0);
        _g->F5e2();
        _g->F6888();
        enqueue_some_tile_animation_to_dma();
        _g->F127e();
        vblank(0);
        G.io->w(SOME_STATE_COUNTER, G.io->w(SOME_STATE_COUNTER) + 1);
      }
    }

    _log_ret(__func__);
  }

  void clear_sprite_on_a6_14() override
  {
    _log_call(__func__);

    clear_sprite_position(G.io->w(G.a[6] + 14));

    _log_ret(__func__);
  }

  void clear_sprite_position(sword_t sprite_idx) override
  {
    _log_call(__func__);

    const ulong_t addr = SPRITE_TABLE + sprite_idx;
    G.io->w(addr, 0);
    G.io->w(addr + 6, 0);

    _log_ret(__func__);
  }

  void inc_something(ulong_t a6) override
  {
    _log_call(__func__);
    inc_mem_w(a6 + 0x16, G.io->w(0xff008e));
    _log_ret(__func__);
  }

  // I think this function has something to do with movement of objects
  void F677c_manual(ulong_t a6) override
  {
    _log_call(__func__);

    ulong_t a0 = 0x67ec;
    sword_t d0 = G.io->w(a6 + 8);
    sword_t d1 = (d0 >> 2) & 14;
    d0 = (d0 & 7) << 2;

    if ((d1 & 2) != 0) {
      a0 += 0x20;
      d0 = -d0;
    }

    G.d[2].w(G.io->w(a0 + d0));
    G.d[3].w(G.io->w(a0 + d0 + 2));
    _g->jump_map(0x67dc + d1);

    slong_t mult = G.io->w(a6 + 6);

    sword_t d2 = (G.d[2].w() * mult) >> 3;
    sword_t d3 = (G.d[3].w() * mult) >> 3;

    inc_mem_w(a6 + 0x16, d2);
    inc_mem_w(a6 + 0x14, d3);

    // Required side effect =(
    G.d[2].w(d2);
    G.d[3].w(d3);

    _log_ret(__func__);
  }

  // No idea what this function does
  void F9b0_manual(ulong_t a6, sword_t d2) override
  {
    _log_call(__func__);

    G.d[0].l(UW(G.io->w(a6 + 0x14) + 0x3000));
    G.d[1].l(UW(G.io->w(a6 + 0x16) + 0x4800));

    G.a[0] = 0xff24c2;

    _g->jump_map(d2 + 0x9ce);

    _log_ret(__func__);
  }

  void F960_manual(ulong_t a5, ulong_t a6, sword_t d2) override
  {
    _log_call(__func__);

    d2 -= 4;
    ulong_t a0 = 0xff24c2;
    G.io->b(a0 + 0x21, BSET<B>(G.io->b(a0 + 0x21), 0));
    if (G.sr.check_condition(Condition::EQ)) {
      slong_t d0 = G.io->w(a5 + 0x14);
      slong_t d1 = G.io->w(a5 + 0x16);
      d0 = uword_t(d0 + 0x3000);
      d1 = (d1 & 0xffff0000) | uword_t(d1 + 0x4800);
      slong_t d6 = d0;
      d0 += G.io->w(a5 + 0x18);
      write_mem_inc(a0, d0);
      d6 -= G.io->w(a5 + 0x1a);
      write_mem_inc(a0, d6);
      d0 <<= 1;
      write_mem_inc(a0, d0);
      d6 <<= 1;
      write_mem_inc(a0, d6);
      d0 >>= 2;
      write_mem_inc(a0, d0);
      d6 >>= 2;
      write_mem_inc(a0, d6);
      write_mem_inc(a0, d1 + G.io->w(a5 + 0x1c));
      d1 -= G.io->w(a5 + 0x1e);
      write_mem_inc(a0, d1);
    }
    F9b0_manual(a6, d2);

    _log_ret(__func__);
  }

  // TODO: I think the following functions are some collision detection code
  void F8f0_manual(ulong_t a5, ulong_t a6) override
  {
    _log_call(__func__);

    G.d[2].w(UCC(G.io->w(a6 + 2)));
    if (G.sr.check_condition(Condition::NE)) {
      F960_manual(G.a[5], a6, G.d[2].w());
    } else {
      sword_t d0 = G.io->w(a5 + 20) + 0x3000;
      sword_t d1 = G.io->w(a6 + 20) + 0x3000;
      d0 = SUB<W>(d0, d1);
      if (G.sr.check_condition(Condition::CS)) {
        F91a_manual(d0, a5, a6);
      } else {
        d1 = G.io->w(a5 + 26) + G.io->w(a6 + 24);
        CMP<W>(d0, d1);
        if (G.sr.check_condition(Condition::CS)) {
          F928_manual(a5, a6);
        } else {
          clear_d0();
        }
      }
    }

    _log_ret(__func__);
  }

  void F91a_manual(sword_t d0, ulong_t a5, ulong_t a6) override
  {
    _log_call(__func__);

    sword_t d1 = G.io->w(a5 + 24) + G.io->w(a6 + 26);
    CMP<W>(-d0, d1);
    if (G.sr.check_condition(Condition::CC)) {
      clear_d0();
    } else {
      F928_manual(a5, a6);
    }

    _log_ret(__func__);
  }

  void F928_manual(ulong_t a5, ulong_t a6) override
  {
    _log_call(__func__);

    sword_t d0 = G.io->w(a5 + 22) + 0x4800;
    sword_t d1 = G.io->w(a6 + 22) + 0x4800;
    d0 = SUB<W>(d0, d1);
    if (G.sr.check_condition(Condition::CS)) {
      sword_t d1 = G.io->w(a5 + 28) + G.io->w(a6 + 30);
      if (UW(-d0) < UW(d1)) {
        set_d0_1();
      } else {
        clear_d0();
      }
    } else {
      d1 = G.io->w(a5 + 30) + G.io->w(a6 + 28);
      CMP<W>(d0, d1);
      if (G.sr.check_condition(Condition::CS)) {
        set_d0_1();
      } else {
        clear_d0();
      }
    }

    _log_ret(__func__);
  }

  ManualFunctionsImpl(bool v) : _v(v) {}

 private:
  void _log_call(const char* n) const
  {
    if (_v) P("Call $", n);
  }
  void _log_ret(const char* n) const
  {
    if (_v) P("Returned $", n);
  }
  GeneratedIntf::ptr _g;
  bool _v;

  std::vector<DMARequest> _dma_queue;
};

} // namespace

ManualFunctions::~ManualFunctions() {}

ManualFunctions::ptr ManualFunctions::create(bool verbose)
{
  return std::make_shared<ManualFunctionsImpl>(verbose);
}

} // namespace heaven_ice
