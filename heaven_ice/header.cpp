#include "header.hpp"

#include "bee/format.hpp"
#include "bee/print.hpp"

namespace heaven_ice {
namespace {

struct HeaderField {
  const char* name;
  int size;
};

HeaderField fields[] = {
  {"system_type", 16},
  {"copyright_release_date", 16},
  {"game_title_domestic", 48},
  {"game_title_overseas", 48},
  {"serial_number", 14},
  {"checksum", 2},
  {"device_support", 16},
  {"rom_address_range", 8},
  {"ram_address_range", 8},
  {"extra_memory", 12},
  {"modem_suport", 12},
  {"reserved1", 40},
  {"region", 3},
  {"reserved2", 13},
};

} // namespace

void print_header(const std::string& content)
{
  int header_offset = 0x100;
  for (auto&& field : fields) {
    auto value = content.substr(header_offset, field.size);
    header_offset += field.size;
    P("$: '$'", field.name, value);
  }

  P(header_offset);
}

} // namespace heaven_ice
