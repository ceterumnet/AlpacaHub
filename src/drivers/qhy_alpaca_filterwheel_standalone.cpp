#include "qhy_alpaca_filterwheel_standalone.hpp"
#include "asio/system_error.hpp"
#include <chrono>
#include <thread>
bool qhy_alpaca_filterwheel_standalone::connected() { return _connected; }

int qhy_alpaca_filterwheel_standalone::set_connected(bool connected) {
  // covers already connected with connect request and
  // not connected with disconnect request
  if (_connected && connected) {
    spdlog::warn(
        "set_connected called with connected:{0} but is already in that state");
    return 0;
  }

  if (connected) {
    try {
      spdlog::debug("Setting connected to true");
      spdlog::debug("Attempting to open serial device at {0}",
                    _serial_device_path);
      _serial_port.open(_serial_device_path);
      _serial_port.set_option(asio::serial_port_base::baud_rate(9600));
      _serial_port.set_option(asio::serial_port_base::character_size(8));
      _serial_port.set_option(asio::serial_port_base::flow_control(
          asio::serial_port_base::flow_control::none));
      _serial_port.set_option(
          asio::serial_port_base::parity(asio::serial_port_base::parity::none));
      _serial_port.set_option(asio::serial_port_base::stop_bits(
          asio::serial_port_base::stop_bits::one));

      _connected = true;

      _filterwheel_update_thread = std::thread(std::bind(
          &qhy_alpaca_filterwheel_standalone::update_properties_proc, this));

      return 0;
    } catch (asio::system_error &e) {
      spdlog::error("problem opening serial connection. {0}", e.what());
      throw alpaca_exception(
          alpaca_exception::DRIVER_ERROR,
          fmt::format(
              "Problem opening serial connection at {0} with error: {1}",
              _serial_device_path, e.what()));
    }
  } else {
    try {
      spdlog::debug("Setting connected to false");
      if (_connected) {
        _connected = false;
        _filterwheel_update_thread.join();
        _serial_port.close();
      }
      _connected = false;
      return 0;
    } catch (asio::system_error &e) {
      spdlog::error("problem closing serial connection {}", e.what());
      throw alpaca_exception(
          alpaca_exception::DRIVER_ERROR,
          fmt::format(
              "Problem closing serial connection at {0} with error: {1}",
              _serial_device_path, e.what()));
    }
  }
  return -1;
}

std::vector<std::string>
qhy_alpaca_filterwheel_standalone::get_connected_filterwheels() {
  std::vector<std::string> filter_wheels;
  filter_wheels.push_back(
      "/dev/serial/by-id/"
      "usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0");
  return filter_wheels;
};

std::string qhy_alpaca_filterwheel_standalone::description() {
  return _description;
}

std::string qhy_alpaca_filterwheel_standalone::driverinfo() {
  return _driver_info;
}

std::string qhy_alpaca_filterwheel_standalone::name() { return _name; }

uint32_t qhy_alpaca_filterwheel_standalone::interface_version() { return 2; };

std::string qhy_alpaca_filterwheel_standalone::driver_version() {
  return _driver_version;
}

std::vector<std::string>
qhy_alpaca_filterwheel_standalone::supported_actions() {
  return std::vector<std::string>();
}

std::string qhy_alpaca_filterwheel_standalone::send_command_to_filterwheel(
    const std::string &cmd, int n_chars_to_read) {

  try {
    spdlog::trace("sending: {} to filterwheel", cmd);
    std::lock_guard lock(_filterwheel_mtx);
    char buf[512] = {0};
    if (cmd.length() > 0)
      _serial_port.write_some(asio::buffer(cmd));
    std::string rsp;

    if (n_chars_to_read > 0) {
      _io_context.reset();
      // TODO: we may need to make the read timeout configurable here
      alpaca_hub_serial::blocking_reader reader(cmd, _serial_port, 1000,
                                                _io_context, false);
      char c;
      int n_chars_read = 0;
      while (reader.read_char(c)) {
        n_chars_read++;
        // spdlog::trace("char read: {}", c);
        rsp += c;
        if (n_chars_read == n_chars_to_read) {
          break;
        }
      }
    }

    spdlog::trace("filterwheel returned: {}", rsp);
    return rsp;
  } catch (std::exception &ex) {
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("Problem sending command to filterwheel: ", ex.what()));
  }
}

void qhy_alpaca_filterwheel_standalone::initialize() {
  spdlog::debug("Initializing filterwheel");
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(30s);
  spdlog::debug("Flushing serial receive and transmit buffer");
  auto res = ::tcflush(_serial_port.lowest_layer().native_handle(), TCIOFLUSH);

  if (res != 0) {
    asio::system_error serial_error;
    serial_error = asio::error_code(errno, asio::error::get_system_category());
    spdlog::error("problem flushing serial port: {}", serial_error.what());
  } else {
    spdlog::debug("successfully flushed serial port");
  }

  auto resp = send_command_to_filterwheel("VRS", 8);
  spdlog::debug("filterwheel returned {0}", resp);

  _firmware_version = resp;

  resp = send_command_to_filterwheel("MXP", 1);
  auto number_of_filters = std::atoi(resp.data());
  // TODO: take this number and set the filter data correctly

  _names.clear();
  _focus_offsets.clear();

  for (int i = 0; i < number_of_filters; i++) {
    char filter_name = '0' + i + 1;
    // TODO: I need to have a loadable setting for the filter names...
    _names.push_back(std::string{filter_name});
    _focus_offsets.push_back(0);
  }
}

void qhy_alpaca_filterwheel_standalone::update_properties_proc() {
  using namespace std::chrono_literals;

  initialize();
  std::string resp;
  while (_connected) {
    if (!_busy) {
      resp = send_command_to_filterwheel("NOW", 1);
      _position = std::atoi(resp.data());
    } else {
      // TODO: the mechanics of this are a little janky...needs some
      // rework
      spdlog::debug("Filterwheel is busy...waiting for idle");

      std::string rsp;

      _io_context.reset();
      // TODO: we may need to make the read timeout configurable here
      alpaca_hub_serial::blocking_reader reader("CHECKING RECV", _serial_port,
                                                500, _io_context);
      char c;
      int n_chars_to_read = 1;
      int n_chars_read = 0;
      while (reader.read_char(c)) {
        n_chars_read++;
        // spdlog::trace("char read: {}", c);
        rsp += c;
        if (n_chars_read == n_chars_to_read) {
          break;
        }
      }

      if (rsp.length() > 0)
        _busy = false;
    }
    std::this_thread::sleep_for(500ms);
  }
}

qhy_alpaca_filterwheel_standalone::qhy_alpaca_filterwheel_standalone(
    const std::string &device_path)
    : _serial_device_path(device_path), _connected(false),
      _driver_version("v0.1"), _description("QHY Filterwheel Standalone"),
      _name("QHYFW"), _serial_port(_io_context), _busy(false) {

  // TODO: This should be driven off of what the filterwheel indicates is
  // actually there
  // _names = { "0", "1", "2", "3", "4", "5", "6", "7", "8" };
  // _focus_offsets = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
}

qhy_alpaca_filterwheel_standalone::~qhy_alpaca_filterwheel_standalone() {
  spdlog::debug("filterwheel destructor called");
  try {
    if (_connected) {
      set_connected(false);
      // _filterwheel_update_thread.join();
    }
  } catch (std::exception &ex) {
    spdlog::error("Problem: {}", ex.what());
  }
}

int qhy_alpaca_filterwheel_standalone::position() { return _position; }

std::vector<std::string> qhy_alpaca_filterwheel_standalone::names() {
  return _names;
}

// TODO: need to add validation and logic for this to make sense
int qhy_alpaca_filterwheel_standalone::set_names(
    std::vector<std::string> names) {
  _names = names;
  return 0;
}

int qhy_alpaca_filterwheel_standalone::set_position(uint32_t position) {
  char pos = position + '0';
  // TODO:
  // send command on thread that updates position when received back from
  // filterwheel
  _busy = true;
  if (position < 0 || position > (_names.size() - 1))
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("{} is an invalid filter position", position));

  auto resp = send_command_to_filterwheel(std::string{pos}, 1);
  if (resp.length() > 0) {
    _position = std::atoi(resp.data());
    _busy = false;
  }
  return 0;
}

std::vector<int> qhy_alpaca_filterwheel_standalone::focus_offsets() {
  return _focus_offsets;
}

// TODO: I need to do something more permanent with guids here
std::string qhy_alpaca_filterwheel_standalone::unique_id() {
  return _serial_device_path + std::string("FW");
}

std::map<std::string, device_variant_t>
qhy_alpaca_filterwheel_standalone::details() {
  std::map<std::string, device_variant_t> detail_map;
  detail_map["Connected"] = _connected;
  if (_connected) {
    detail_map["Position"] = position();
    detail_map["Names"] = names();
    detail_map["FocusOffsets"] = focus_offsets();
  }
  return detail_map;
};
