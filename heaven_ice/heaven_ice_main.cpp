#include <exception>
#include <limits>

#include "disasm.hpp"
#include "emulate.hpp"
#include "exceptions.hpp"
#include "generated.hpp"
#include "globals.hpp"
#include "manual_functions.hpp"
#include "parse_spec.hpp"
#include "to_cpp.hpp"

#include "bee/file_path.hpp"
#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "bee/or_error.hpp"
#include "bee/print.hpp"
#include "command/command_builder.hpp"
#include "command/file_path.hpp"
#include "command/group_builder.hpp"

namespace heaven_ice {
namespace {

bee::OrError<> disasm_main(const bee::FilePath& rom_filename)
{
  bail(content, bee::FileReader::read_file(rom_filename));
  return Disasm::disasm_and_print(content);
}

command::Cmd to_cpp_cmd()
{
  using namespace command::flags;
  auto builder = command::CommandBuilder("Convert to cpp");
  auto filepath = builder.required_anon(FilePath, "FILEPATH", "Rom file");
  return builder.run([=]() -> bee::OrError<> {
    bail(content, bee::FileReader::read_file(*filepath));
    return ToCpp::to_cpp(content);
  });
}

command::Cmd disasm_cmd()
{
  using namespace command::flags;
  auto builder = command::CommandBuilder("Disassemble");
  auto filepath = builder.required_anon(FilePath, "FILEPATH", "Rom file");
  return builder.run([=]() { return disasm_main(*filepath); });
}

auto env_flags(command::CommandBuilder& builder, bool native)
{
  using namespace command::flags;
  auto max_frames = builder.optional("--max-frames", Int);
  auto display = builder.optional_with_default("--display", String, "sdl");
  auto verbose = builder.no_arg("--verbose");
  auto speed = builder.optional_with_default("--speed", Float, 1.0);
  auto read_events = builder.optional("--read-events", FilePath);
  auto write_events = builder.optional("--write-events", FilePath);
  auto exit_after_playback = builder.no_arg("--exit-after-playback");
  auto skip_to_frame = builder.optional_with_default("--skip-to-frame", Int, 0);
  auto rom_filename = builder.required_anon(FilePath, "FILEPATH", "Rom file");
  return [=]() -> bee::OrError<bool> {
    bail(rom_content, bee::FileReader::read_file(*rom_filename));
    GeneratedIntf::ptr generated;
    if (native) {
      auto manual_functions = ManualFunctions::create(*verbose);
      generated = Generated::create(*verbose, manual_functions);
      manual_functions->set_generated(generated);
    }
    G.init_runtime({
      .verbose = *verbose,
      .rom_content = rom_content,
      .generated = generated,
      .display = *display,
      .max_frames = *max_frames,
      .speed = *speed,
      .read_events = *read_events,
      .write_events = *write_events,
      .exit_after_playback = *exit_after_playback,
      .skip_to_frame = *skip_to_frame,
    });
    return *verbose;
  };
}

template <class F> command::Cmd run(command::CommandBuilder& builder, F&& f)
{
  return builder.run([=]() -> bee::OrError<> {
    try {
      bail_unit(f());
    } catch (const ExitRequested& e) {
      PE("Exit requested, reason: $", e.what());
    } catch (const bee::Exn& exn) {
      std::ignore = bee::FileWriter::stdout().flush();
      throw;
    } catch (const std::exception& exn) {
      std::ignore = bee::FileWriter::stdout().flush();
      throw;
    }
    std::ignore = bee::FileWriter::stdout().flush();
    return bee::ok();
  });
}

command::Cmd emulate_cmd()
{
  using namespace command::flags;
  auto builder = command::CommandBuilder("Emulate");
  auto max_instructions = builder.optional("--max-instructions", Int);
  auto show_registers = builder.no_arg("--show-registers");
  auto load_state = builder.optional("--load-state", FilePath);
  auto save_state = builder.optional("--save-state", FilePath);
  auto init = env_flags(builder, false);
  return run(builder, [=]() -> bee::OrError<> {
    bail(verbose, init());
    return Emulate::main(
      *show_registers, verbose, *max_instructions, *load_state, *save_state);
  });
}

command::Cmd native_cmd()
{
  using namespace command::flags;
  auto builder = command::CommandBuilder("Convert to cpp");
  auto init = env_flags(builder, true);
  return run(builder, [=]() -> bee::OrError<> {
    bail_unit(init());
    G.run_native();
    return bee::ok();
  });
}

int main(int argc, char* argv[])
{
  using namespace command::flags;
  return command::GroupBuilder("Emulator")
    .cmd("emulate", emulate_cmd())
    .cmd("parse", ParseSpec::cmd())
    .cmd("disasm", disasm_cmd())
    .cmd("to-cpp", to_cpp_cmd())
    .cmd("native", native_cmd())
    .build()
    .main(argc, argv);
}

} // namespace
} // namespace heaven_ice

int main(int argc, char* argv[]) { return heaven_ice::main(argc, argv); }
