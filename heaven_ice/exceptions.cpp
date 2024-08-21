#include "exceptions.hpp"

namespace heaven_ice {

const char* ExitRequested::what() const noexcept { return _msg; }

} // namespace heaven_ice
