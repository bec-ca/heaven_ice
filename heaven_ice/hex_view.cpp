#include "hex_view.hpp"

#include "bee/format.hpp"
#include "bee/print.hpp"

namespace heaven_ice {
namespace {

constexpr size_t max_line_size = 32;

}

void HexView::print_hex(ulong_t start_address, const std::string& content)
{
  for (int line_num = 0;; line_num++) {
    size_t offset = line_num * max_line_size;
    if (offset >= content.size()) { break; }
    std::string left;
    std::string right;
    for (size_t i = 0; i < max_line_size; i++) {
      auto idx = offset + i;
      if (idx < content.size()) {
        char c = content[i + offset];
        left += F("{02x}", uint8_t(c));
        left += ' ';
        if (c >= 32 && c < 127) {
          right += F(c);
        } else {
          right += '.';
        }
      } else {
        left += "   ";
      }
    }
    P("{06x}: $ $", offset + start_address, left, right);
  }
}

} // namespace heaven_ice
