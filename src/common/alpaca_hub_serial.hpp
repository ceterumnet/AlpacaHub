#ifndef ALPACA_HUB_SERIAL_HPP
#define ALPACA_HUB_SERIAL_HPP

// #include "asio/"
#include "asio/io_context.hpp"
#include "asio/serial_port.hpp"
#include "asio/steady_timer.hpp"
#include "spdlog/spdlog.h"
#include <memory>

namespace alpaca_hub_serial {

// This is an attempt to summarize how this works with the ASIO weirdness
//
// The way this reader works is as follows:
// 1. It is initialized with an existing asio::serial_port, a timeout in
// milliseconds, and an asio::io_context
// 2. When the read_char is invoked, it resets the context and registers
// two handlers, 1 for timeout and another as a read_callback
// 3. When the read_complete:
//     - successfully reads, it cancels the timer which invokes the timeout
//       which receives a non successful error code which means we still have
//       to try and read more
//     - when it doesn't successfully read, it sets the read_error to true
//       which tells the caller that there is no more to read
// 4. When the time_out callback is invoked it:
//     - if it is an actual timeout, it cancels the async read operation
//       which invokes the read_complete callback with an error code
//     - if it is a result of the read_complete success, it will
//       simply return
class blocking_reader {
  asio::serial_port &port;
  size_t timeout;
  char c;
  asio::steady_timer timer;
  bool read_error;
  asio::io_context &_io_ctx;

  // Called when an async read completes or has been cancelled
  void read_complete(const asio::error_code &error, size_t bytes_transferred);
  // Called when the timer's deadline expires.
  void time_out(const asio::error_code &error);

public:
  blocking_reader(asio::serial_port &port, size_t timeout,
                  asio::io_context &io_ctx);
  bool read_char(char &val);
};
} // namespace alpaca_hub_serial

#endif
