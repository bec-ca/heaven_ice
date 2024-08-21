#pragma once

#include <exception>

namespace heaven_ice {

struct ExitRequested final : public std::exception {
 public:
  ExitRequested(const char* msg) : _msg(msg) {}
  virtual const char* what() const noexcept override;

 private:
  const char* _msg;
};

} // namespace heaven_ice
