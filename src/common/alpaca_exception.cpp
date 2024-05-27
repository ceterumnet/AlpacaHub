#include "alpaca_exception.hpp"

alpaca_exception::alpaca_exception(const int32_t error_code,
                                   std::string error_message)
    : std::runtime_error(error_message) {

  _error_code = error_code;
};


int32_t alpaca_exception::error_code() { return _error_code; };
