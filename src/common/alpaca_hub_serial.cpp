#include "alpaca_hub_serial.hpp"
#include <regex>

std::vector<std::string> split(const std::string &input,
                               const std::string &regex) {
  // passing -1 as the submatch index parameter performs splitting
  std::regex pattern(regex);
  std::sregex_token_iterator first{input.begin(), input.end(), pattern, -1},
      last;
  return {first, last};
}

namespace alpaca_hub_serial {

blocking_reader::blocking_reader(const std::string &command,
                                 asio::serial_port &port, size_t timeout,
                                 asio::io_context &io_ctx,
                                 bool warn_on_serial_timeout)
    : port(port), timeout(timeout), _io_ctx(io_ctx), timer(port.get_executor()),
      read_error(true), _command(command),
      _warn_on_serial_timeout(warn_on_serial_timeout) {}

void blocking_reader::read_complete(const asio::error_code &error,
                                    size_t bytes_transferred) {
  // spdlog::trace("read_complete invoked: msg: {0}", error.message());
  // spdlog::trace("  bytes read:               {0}", bytes_transferred);

  if (error.message() != "Success") {
    read_error = true;
    return;
  } else {
    read_error = false;
    // reset the timer
    // timer.expires_from_now(std::chrono::milliseconds(timeout));
  }
  // Read has finished, so cancel the
  // timer.
  // spdlog::trace("canceling timer in read_complete");
  timer.cancel();
}

// Called when the timer's deadline expires.
void blocking_reader::time_out(const asio::error_code &error) {
  // Was the timeout was cancelled?
  // spdlog::trace("time_out invoked {0}", error.message());
  if (error.message() != "Success") {
    // yes
    return;
  }
  // no, we have timed out, so kill
  // the read operation
  // The read callback will be called
  // with an error
  if (_warn_on_serial_timeout)
    spdlog::warn(
        "calling port.cancel() - ensure that all serial commands are "
        "explicit if they expect a response or not. Command associated: {}",
        _command);
  else
    spdlog::trace("Serial timeout for cmd: \"{}\"", _command);
  port.cancel();
}

bool blocking_reader::read_char(char &val) {
  using namespace std::chrono_literals;

  val = c = '\0';
  // After a timeout & cancel we need
  // to do a reset for subsequent reads to work.
  _io_ctx.reset();
  // Asynchronously read 1 character.
  port.async_read_some(asio::buffer(&c, 1),
                       std::bind(&blocking_reader::read_complete, this,
                                 std::placeholders::_1, std::placeholders::_2));

  // Setup a deadline time to implement our timeout.
  timer.expires_from_now(std::chrono::milliseconds(timeout));
  timer.async_wait(
      std::bind(&blocking_reader::time_out, this, std::placeholders::_1));
  // This will block until a character is read
  // or until the it is cancelled.

  _io_ctx.run();

  if (!read_error)
    val = c;
  return !read_error;
}
} // namespace alpaca_hub_serial
