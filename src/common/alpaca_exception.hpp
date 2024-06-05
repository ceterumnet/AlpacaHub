#ifndef ALPACA_EXCEPTION_HPP
#define ALPACA_EXCEPTION_HPP

#include <cstdint>
#include <exception>
#include <stdexcept>

class alpaca_exception : public std::runtime_error {
public:
  // static constexpr int32_t NOT_IMPLEMENTED = 0x80040400;
  // static constexpr int32_t INVALID_VALUE = 0x80040401;
  // static constexpr int32_t NOT_CONNECTED = 0x80040407;
  // static constexpr int32_t DRIVER_ERROR = 0x80040500;
  // static constexpr int32_t INVALID_OPERATION = 0x8004040B;
  // static constexpr int32_t UNSPECIFIED_ERROR = 0x800404FF;

  static constexpr int32_t NOT_IMPLEMENTED = 0x400;
  static constexpr int32_t INVALID_VALUE = 0x401;
  static constexpr int32_t NOT_CONNECTED = 0x407;
  static constexpr int32_t DRIVER_ERROR = 0x500;
  static constexpr int32_t INVALID_OPERATION = 0x40B;
  static constexpr int32_t UNSPECIFIED_ERROR = 0x4FF;

  alpaca_exception(const int32_t error_code, std::string error_message);

  int32_t error_code();

private:
  int32_t _error_code;
  std::string error_message;
};

#endif
