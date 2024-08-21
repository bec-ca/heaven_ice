#include "globals.hpp"

#include "controller.hpp"
#include "display_ffmpeg.hpp"
#include "display_hash.hpp"
#include "display_pnm.hpp"
#include "display_sdl.hpp"
#include "exceptions.hpp"
#include "input_event.hpp"
#include "io.hpp"
#include "magic_constants.hpp"
#include "memory.hpp"
#include "save_state.hpp"
#include "vdp.hpp"

#include "bee/bytes.hpp"
#include "bee/filesystem.hpp"
#include "bee/print.hpp"
#include "bee/time.hpp"
#include "chunk_file/chunk_file.hpp"
#include "sdl/key_code.hpp"
#include "yasf/cof.hpp"

namespace heaven_ice {
namespace {

constexpr int SpeedScale = 1024;
constexpr int FPS = 60;
constexpr double FrameDurationDecay = 0.9;

DisplayIntf::ptr create_display(const std::string& name)
{
  if (name == "pnm") {
    return DisplayPnm::create();
  } else if (name == "sdl") {
    must(disp, DisplaySDL::create(4));
    return disp;
  } else if (name == "ffmpeg") {
    return DisplayFfmpeg::create(4);
  } else if (name == "hash") {
    return DisplayHash::create();
  } else if (name == "none") {
    return nullptr;
  } else {
    raise_error("Unknown display option: $", name);
  }
}

std::optional<ControlKey> to_control_key(sdl::KeyCode key)
{
  switch (key) {
  case sdl::KeyCode::W:
    return ControlKey::Up;
  case sdl::KeyCode::S:
    return ControlKey::Down;
  case sdl::KeyCode::A:
    return ControlKey::Left;
  case sdl::KeyCode::D:
    return ControlKey::Right;
  case sdl::KeyCode::J:
    return ControlKey::A;
  case sdl::KeyCode::K:
    return ControlKey::B;
  case sdl::KeyCode::L:
    return ControlKey::C;
  case sdl::KeyCode::Enter:
    return ControlKey::Start;
  case sdl::KeyCode::Escape:
    throw ExitRequested("User pressed ESC");
  default:
    return std::nullopt;
  }
}

std::optional<InputEvent> to_input_event_impl(
  const sdl::Event::KeyboardEvent& ev)
{
  auto key = to_control_key(ev.key);
  if (!key.has_value()) { return std::nullopt; }
  switch (ev.action) {
  case sdl::KeyAction::KeyDown:
    return InputEvent{.kind = InputEventKind::ControlKeyDown, .key = key};
  case sdl::KeyAction::KeyUp:
    return InputEvent{.kind = InputEventKind::ControlKeyUp, .key = key};
  }
}

template <class T> std::optional<InputEvent> to_input_event_impl(const T&)
{
  return std::nullopt;
}

} // namespace

struct Globals::GlobalsImpl {
  GlobalsImpl(const Args& args)
      : _vdp(VDP::create(args.verbose)),
        _max_frames(args.max_frames),
        _controller(Controller::create()),
        _generated(args.generated),
        _speed(args.speed),
        _speed_mult(SpeedScale / _speed),
        _skip_to_frame(args.skip_to_frame),
        _exit_after_playback(args.exit_after_playback),
        _last_frame(bee::Time::now())
  {
    auto rom = std::make_shared<Memory>(args.rom_content);
    auto ram = std::make_shared<Memory>(RAM_END - RAM_BEGIN);

    if (args.display.has_value()) {
      _display = create_display(*args.display);
      _should_wait_frame = args.display.value() == "sdl";
    }
    _bus = std::make_shared<IO>(args.verbose, ram, rom, _vdp, _controller);
    _vdp->set_bus(_bus);

    if (args.read_events) {
      must_assign(
        _events_reader, chunk_file::ChunkFileReader::open(*args.read_events));
    }

    if (args.write_events) {
      must_assign(
        _events_writer,
        chunk_file::ChunkFileWriter::create(*args.write_events));
    }
  }

  void vblank(Globals& g)
  {
    if (_max_frames && _frames_count >= *_max_frames) {
      throw ExitRequested("Max frames reached");
    }
    ++_frames_count;

    _handle_events();

    if (_generated) {
      g.pushw(g.sr.to_int());
      _generated->vblank_int();
      g.sr.set_from_int(g.popw());
    }

    // _vdp->dump_memory(_frames_count);
    // _vdp->dump_sprites(_frames_count);

    auto prev = _progress_counter;
    _progress_counter += _speed_mult;
    if (
      _frames_count >= _skip_to_frame &&
      prev / SpeedScale < _progress_counter / SpeedScale) {
      auto img = _vdp->render();
      if (_display) {
        _display->update(img);
        _wait_frame();
      }
    }
  }

  bool is_vblank_enabled() const
  {
    return G.sr.int_priority_mask() <= 6 && _vdp->vblank_enabled();
  }

  void run_native() { _generated->run(); }

  void save_state(const Globals& g, bee::Writer& writer)
  {
    _bus->save_state(writer);
    save_state_gen(g.d, writer);
    save_state_gen(g.a, writer);
    save_state_gen(g.sr, writer);
  }

  void load_state(Globals& g, bee::Reader& reader)
  {
    _bus->load_state(reader);
    load_state_gen(g.d, reader);
    load_state_gen(g.a, reader);
    load_state_gen(g.sr, reader);
  }

  const std::shared_ptr<IO> io() const { return _bus; }

 private:
  void _handle_events()
  {
    auto events = [&]() {
      std::vector<InputEvent> control_events;
      if (_display) {
        for (const auto& ev : _display->get_events()) {
          auto iev =
            ev.visit([&]<class T>(const T& ev) -> std::optional<InputEvent> {
              if constexpr (std::is_same_v<T, sdl::Event::QuitEvent>) {
                P("Exiting after $ frames", _frames_count);
                throw ExitRequested("User closed the window");
              } else if constexpr (std::
                                     is_same_v<T, sdl::Event::KeyboardEvent>) {
                switch (ev.action) {
                case sdl::KeyAction::KeyDown:
                  switch (ev.key) {
                  case sdl::KeyCode::Plus:
                  case sdl::KeyCode::Equal:
                    _add_speed(1.125);
                    return std::nullopt;
                  case sdl::KeyCode::Minus:
                    _add_speed(1.0 / 1.125);
                    return std::nullopt;
                  case sdl::KeyCode::Space:
                    _events_reader = nullptr;
                    _set_speed(1.0);
                    return std::nullopt;
                  default:
                    return to_input_event_impl(ev);
                  };
                case sdl::KeyAction::KeyUp:
                  return to_input_event_impl(ev);
                }
              } else {
                return std::nullopt;
              }
            });
          if (iev) { control_events.push_back(*iev); }
        }
      }
      if (_events_reader) {
        must(data, _events_reader->read_next());
        if (data.has_value()) {
          must(
            events,
            yasf::Cof::deserialize<std::vector<InputEvent>>(data->to_string()));
          return events;
        } else if (_exit_after_playback) {
          throw ExitRequested("End of playback reached");
        } else {
          P("Playback complete");
          _events_reader = nullptr;
        }
      }
      return control_events;
    }();

    for (const auto& ev : events) {
      switch (ev.kind) {
      case InputEventKind::ControlKeyDown:
        _controller->key_down(0, ev.key.value());
        break;
      case InputEventKind::ControlKeyUp:
        _controller->key_up(0, ev.key.value());
        break;
      }
    }

    if (_events_writer) {
      must_unit(
        _events_writer->write(bee::Bytes(yasf::Cof::serialize(events))));
    }
  }

  void _wait_frame()
  {
    if (!_should_wait_frame) return;
    auto since_last_frame = bee::Time::now() - _last_frame;

    _frame_duration_sum *= FrameDurationDecay;

    _frame_duration_weight = _frame_duration_weight * FrameDurationDecay + 1.0;
    auto target =
      bee::Span::of_seconds(_frame_duration_weight / FPS - _frame_duration_sum);
    auto diff = target - since_last_frame;
    if (diff.is_positive()) { diff.sleep(); }
    auto now = bee::Time::now();
    auto duration = now - _last_frame;
    _frame_duration_sum += duration.to_float_seconds();
    _last_frame = now;
  }

  void _add_speed(double mult) { _set_speed(_speed * mult); }

  void _set_speed(double speed)
  {
    _speed = speed;
    _speed_mult = SpeedScale / _speed;
    P("Speed: $", _speed);
  }

  VDP::ptr _vdp;
  std::optional<int64_t> _max_frames;
  Controller::ptr _controller;
  IO::ptr _bus;
  int64_t _frames_count = 0;

  GeneratedIntf::ptr _generated;
  DisplayIntf::ptr _display;

  double _speed = 1.0;
  int _speed_mult = 1024;
  int64_t _progress_counter = 0;
  int64_t _skip_to_frame = 0;

  bool _exit_after_playback = false;

  chunk_file::ChunkFileWriter::ptr _events_writer;
  chunk_file::ChunkFileReader::ptr _events_reader;

  bee::Time _last_frame;
  double _frame_duration_sum;
  double _frame_duration_weight;

  bool _should_wait_frame = false;
};

Globals G;

void Globals::vblank() { _impl->vblank(*this); }

void Globals::init_runtime(const Args& args)
{
  _impl = std::make_unique<GlobalsImpl>(args);
  io = _impl->io();
}

bool Globals::is_vblank_enabled() const { return _impl->is_vblank_enabled(); }

void Globals::run_native() { _impl->run_native(); }

void Globals::save_state(bee::Writer& writer)
{
  _impl->save_state(*this, writer);
}

void Globals::load_state(bee::Reader& reader)
{
  _impl->load_state(*this, reader);
}

void Globals::push(SizeKind size, slong_t value)
{
  a[7] -= size.num_bytes();
  io->write_signed(size, a[7], value);
}

void Globals::pushl(slong_t value) { push(SizeKind::l(), value); }
void Globals::pushw(sword_t value) { push(SizeKind::w(), value); }
void Globals::pushb(sbyte_t value) { push(SizeKind::b(), value); }

slong_t Globals::pop(SizeKind size)
{
  slong_t ret = io->read_signed(size, a[7]);
  a[7] += size.num_bytes();
  return ret;
}

slong_t Globals::popl() { return pop(SizeKind::l()); }
sword_t Globals::popw() { return pop(SizeKind::w()); }
sbyte_t Globals::popb() { return pop(SizeKind::b()); }

} // namespace heaven_ice
