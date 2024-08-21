#pragma once

#include <memory>

#include "generated_intf.hpp"
#include "types.hpp"

namespace heaven_ice {

struct ManualFunctions {
 public:
  using ptr = std::shared_ptr<ManualFunctions>;

  static ptr create(bool verbose);

  virtual ~ManualFunctions();

  virtual void noop() = 0;

  virtual void clear_d0() = 0;
  virtual void set_d0_1() = 0;

  virtual void neg_w_d2() = 0;
  virtual void neg_w_d3() = 0;
  virtual void neg_w_d2_d3() = 0;
  virtual void exg_d2_d3() = 0;
  virtual void neg_w_d2_exg_d2_d3() = 0;
  virtual void exg_d2_d3_neg_w_d2() = 0;
  virtual void exg_neg_w_d2_d3() = 0;

  virtual void wait_for_z80_bus() = 0;
  virtual void z80_useless() = 0;

  virtual void some_weird_transformation(sword_t d0) = 0;
  virtual void some_weird_transformation_16_times() = 0;

  virtual void update_sprite_with_something(ulong_t a6) = 0;

  // VDP functions

  virtual void enable_vblank_int() = 0;
  virtual void vblank(int count) = 0;
  virtual void clear_cram() = 0;

  virtual void vdp_set_d3_blocks_of_size_d2_with_d0_starting_at_d1(
    uword_t d0, ulong_t d1, uword_t d2, uword_t d3) = 0;

  virtual void vdp_copy_words_to_cram(
    const int count, const ulong_t addr, const ulong_t cmd) = 0;

  virtual void clear_vscroll() = 0;

  virtual void disable_vblank_int() = 0;

  virtual void push_scroll_state() = 0;

  virtual void copy_something_to_vdp(ulong_t a0, ulong_t d0) = 0;

  // DMA stuff

  virtual void dma_push(uword_t size, ulong_t src_addr, ulong_t dst_addr) = 0;

  virtual void add_to_dma_queue(
    uword_t size, ulong_t src_addr, uword_t dst_addr) = 0;
  virtual void flush_dma() = 0;

  virtual void enqueue_some_tile_animation_to_dma() = 0;

  virtual void set_generated(const GeneratedIntf::ptr& generated) = 0;

  virtual void start() = 0;

  virtual void clear_sprites() = 0;
  virtual void clear_all_planes() = 0;
  virtual void clear_window_plane() = 0;

  virtual void clear_sprite_position(sword_t sprite_idx) = 0;
  virtual void clear_sprite_on_a6_14() = 0;

  // Unnamed functions

  virtual void F9b0_manual(ulong_t a6, sword_t d2) = 0;
  virtual void F960_manual(ulong_t a5, ulong_t a6, sword_t d2) = 0;

  virtual void inc_something(ulong_t a6) = 0;
  virtual void F677c_manual(ulong_t a6) = 0;
  virtual void F928_manual(ulong_t a5, ulong_t a6) = 0;
  virtual void F91a_manual(sword_t d0, ulong_t a5, ulong_t a6) = 0;
  virtual void F8f0_manual(ulong_t a5, ulong_t a6) = 0;
};

} // namespace heaven_ice
