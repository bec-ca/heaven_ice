#include "parse_spec.hpp"

#include <map>

#include "instruction_field.hpp"
#include "instruction_spec.hpp"
#include "types.hpp"

#include "bee/file_path.hpp"
#include "bee/file_reader.hpp"
#include "bee/format_vector.hpp"
#include "bee/or_error.hpp"
#include "bee/print.hpp"
#include "bee/string_util.hpp"
#include "command/command_builder.hpp"
#include "command/file_path.hpp"

namespace heaven_ice {
namespace {

bee::OrError<> parse_main(const bee::FilePath& filepath)
{
  bail(insts, ParseSpec::parse_spec(filepath));
  P("Num instructions: $", insts.size());
  for (auto&& inst : insts) {
    int size = inst.size();
    if (size != 16) {
      return EF("Instruction has the wrong size, got $, spec:'$'", size, inst);
    }
  }
  int num_matched = 0;
  int num_unmatched = 0;
  std::map<int, int> matches_by_idx;

  for (int i = 0; i < 1 << 16; i++) {
    bool matched = false;
    for (int j = 0; j < std ::ssize(insts); j++) {
      auto&& inst = insts[j];
      if (inst.match(i)) {
        matched = true;
        matches_by_idx[j]++;
        break;
      }
    }
    if (matched) {
      num_matched++;
    } else {
      num_unmatched++;
    }
  }
  P("matched:$\nunmachted:$", num_matched, num_unmatched);

  for (int i = 0; i < std::ssize(insts); i++) {
    int count = matches_by_idx[i];
    if (count == 0) {
      raise_error("No matches for spec: line:$ inst:$", i + 1, insts);
    }
  }

  return bee::ok();
}

} // namespace

command::Cmd ParseSpec::cmd()
{
  using namespace command::flags;
  auto builder = command::CommandBuilder("Parse");
  auto filepath = builder.required_anon(FilePath, "FILEPATH", "Rom file");
  return builder.run([=]() { return parse_main(*filepath); });
}

bee::OrError<std::vector<InstructionSpec>> ParseSpec::parse_spec(
  const bee::FilePath& filepath)
{
  bail(content, bee::FileReader::read_file(filepath));
  auto lines = bee::split_lines(content);
  std::vector<InstructionSpec> insts;
  for (auto&& line : lines) {
    auto parts = bee::split(line, ",");
    if (parts.size() != 2) { return EF("Wrong number of parts"); }
    auto name = bee::trim_spaces(parts[0]);
    auto fields_spec = bee::trim_spaces(parts[1]);
    auto spec_parts = bee::split_space(fields_spec);
    std::vector<InstructionField> fields;
    for (auto&& field_spec : spec_parts) {
      bail(field, InstructionField::parse_field(field_spec));
      fields.push_back(field);
    }
    bail(ie, InstEnum::of_string(name));
    insts.push_back({ie, std::move(fields)});
  }
  return insts;
}

} // namespace heaven_ice
