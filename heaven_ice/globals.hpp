#include <array>

#include "generated_intf.hpp"
#include "io_intf.hpp"
#include "registers.hpp"
#include "status_register.hpp"
#include "types.hpp"

#include "bee/file_path.hpp"

namespace heaven_ice {

struct Globals {
  std::array<DataRegister, 8> d;
  std::array<AddrRegister, 8> a;
  StatusRegister sr;
  IOIntf::ptr io;

  struct Args {
    bool verbose;
    std::string rom_content;
    std::shared_ptr<GeneratedIntf> generated;
    std::optional<std::string> display;
    std::optional<int64_t> max_frames;
    double speed;
    std::optional<bee::FilePath> read_events;
    std::optional<bee::FilePath> write_events;
    bool exit_after_playback;
    int64_t skip_to_frame;
  };

  void init_runtime(const Args& args);
  void vblank();
  bool is_vblank_enabled() const;
  void run_native();
  void save_state(bee::Writer& writer);
  void load_state(bee::Reader& reader);

  void push(SizeKind size, slong_t value);

  void pushl(slong_t value);
  void pushw(sword_t value);
  void pushb(sbyte_t value);

  slong_t pop(SizeKind size);

  slong_t popl();
  sword_t popw();
  sbyte_t popb();

 private:
  struct GlobalsImpl;

  std::unique_ptr<GlobalsImpl> _impl;
};

extern Globals G;

} // namespace heaven_ice
