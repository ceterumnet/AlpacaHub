#include "pegasus_alpaca_focuscube3.hpp"
#include "common/alpaca_exception.hpp"
#include "common/alpaca_hub_serial.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <regex>
#include <thread>

std::vector<std::string> pegasus_alpaca_focuscube3::serial_devices() {
  std::vector<std::string> serial_devices;
  try {
    auto fs = std::filesystem::path("/dev/serial/by-id");

    for (auto dir_iter : std::filesystem::directory_iterator{fs}) {
      if (dir_iter.path().string().find("PegasusAstro_FocusCube3") !=
          std::string::npos) {
        spdlog::debug("Found focuscube3: {}", dir_iter.path().string());
        serial_devices.push_back(dir_iter.path().string());
      }
    }
  } catch (std::exception &e) {
    spdlog::error("Problem enumerating serial by id: {}", e.what());
  }

  return serial_devices;
}

pegasus_alpaca_focuscube3::pegasus_alpaca_focuscube3()
    : _connected(false), _moving(false), _position(0), _temperature(0),
      _backlash(0), _serial_port(_io_context) {}

pegasus_alpaca_focuscube3::~pegasus_alpaca_focuscube3() {
  if (_connected) {
    _connected = false;
    _focuser_update_thread.join();
    _serial_port.close();
  }
}

uint32_t pegasus_alpaca_focuscube3::interface_version() { return 3; }

std::string pegasus_alpaca_focuscube3::driver_version() { return "v0.1"; }

std::vector<std::string> pegasus_alpaca_focuscube3::supported_actions() {
  return std::vector<std::string>();
}

std::string pegasus_alpaca_focuscube3::description() {
  return "Pegasus Electronic Focuser";
}

std::string pegasus_alpaca_focuscube3::driverinfo() {
  return "AlpacaHub Pegasus Focuser Driver";
}
std::string pegasus_alpaca_focuscube3::name() { return "Pegasus Focus Cube"; }

int pegasus_alpaca_focuscube3::set_connected(bool connected) {
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
      _serial_port.set_option(asio::serial_port_base::baud_rate(115200));
      _serial_port.set_option(asio::serial_port_base::character_size(8));
      _serial_port.set_option(asio::serial_port_base::flow_control(
          asio::serial_port_base::flow_control::none));
      _serial_port.set_option(
          asio::serial_port_base::parity(asio::serial_port_base::parity::none));
      _serial_port.set_option(asio::serial_port_base::stop_bits(
          asio::serial_port_base::stop_bits::one));

      char buf[512] = {0};

      // _serial_port.write_some(asio::buffer("##\r\n"));
      // _serial_port.read_some(asio::buffer(buf));
      auto resp = send_command_to_focuser("##\r\n");
      spdlog::debug("focuser returned {0}", resp);

      // Start update thread to values
      _focuser_update_thread = std::thread(
          std::bind(&pegasus_alpaca_focuscube3::update_properties_proc, this));
      _connected = true;
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
        _focuser_update_thread.join();
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
void pegasus_alpaca_focuscube3::throw_if_not_connected() {
  if (!_connected)
    throw alpaca_exception(alpaca_exception::NOT_CONNECTED,
                           "Focuser not connected");
}

bool pegasus_alpaca_focuscube3::connected() { return _connected; }

void pegasus_alpaca_focuscube3::update_properties() {
  auto resp = send_command_to_focuser("FA\n");

  auto result = split(resp, ":");
  if (result[0] != "FC3")
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        "Did not receive correctly formated data from focuser");

  _position = atoi(result[1].c_str());
  _moving = (result[2] == "1");
  _temperature = atof(result[3].c_str());
  _backlash = atoi(result[5].c_str());
}

void pegasus_alpaca_focuscube3::update_properties_proc() {
  using namespace std::chrono_literals;
  while (_connected) {
    try {
      update_properties();
    } catch (alpaca_exception &ex) {
      spdlog::warn("problem during update_properties: {}", ex.what());
    }
    std::this_thread::sleep_for(500ms);
  }
  spdlog::debug("update_properties_proc ended");
}

std::string pegasus_alpaca_focuscube3::send_command_to_focuser(
    const std::string &cmd, bool read_response, char stop_on_char) {

  try {
    spdlog::trace("sending: {} to focuser", cmd);
    std::lock_guard lock(_focuser_mtx);
    char buf[512] = {0};
    _serial_port.write_some(asio::buffer(cmd));
    std::string rsp;

    if (read_response) {
      _io_context.reset();
      // TODO: we may need to make the read timeout configurable here
      alpaca_hub_serial::blocking_reader reader(cmd, _serial_port, 250,
                                                _io_context);
      char c;
      while (reader.read_char(c)) {
        rsp += c;
        if (c == stop_on_char || stop_on_char == '\0') {
          break;
        }
      }
    }

    spdlog::trace("focuser returned: {}", rsp);
    return rsp;
  } catch (std::exception &ex) {
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("Problem sending command to focuser: ", ex.what()));
  }
}

std::string pegasus_alpaca_focuscube3::unique_id() {
  return _serial_device_path;
}

int pegasus_alpaca_focuscube3::set_serial_device(
    const std::string &serial_device_path) {
  _serial_device_path = serial_device_path;
  return 0;
};

std::string pegasus_alpaca_focuscube3::get_serial_device_path() {
  return _serial_device_path;
};

bool pegasus_alpaca_focuscube3::absolute() {
  throw_if_not_connected();
  return true;
}

bool pegasus_alpaca_focuscube3::is_moving() {
  throw_if_not_connected();
  return _moving;
}

// Just picking an arbitrary value at the moment...
uint32_t pegasus_alpaca_focuscube3::max_increment() {
  throw_if_not_connected();
  return 500;
}

// Need to figure out what this is supposed to be for the pegasus
uint32_t pegasus_alpaca_focuscube3::max_step() {
  throw_if_not_connected();
  return 1000000;
}

uint32_t pegasus_alpaca_focuscube3::position() {
  throw_if_not_connected();
  return _position;
}

uint32_t pegasus_alpaca_focuscube3::step_size() {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Focuser does not intrisically know step size.");
  return 0;
}

// TODO: need to determine whether or not I want to implement this
bool pegasus_alpaca_focuscube3::temp_comp() {
  throw_if_not_connected();
  return false;
}

bool pegasus_alpaca_focuscube3::temp_comp_available() {
  throw_if_not_connected();
  return false;
}

int pegasus_alpaca_focuscube3::set_temp_comp(bool enable_temp_comp) {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Temp comp not implemented");
  return 0;
}

double pegasus_alpaca_focuscube3::temperature() {
  throw_if_not_connected();
  return _temperature;
}

int pegasus_alpaca_focuscube3::halt() {
  throw_if_not_connected();
  send_command_to_focuser("FH\n", false);
  return 0;
}

int pegasus_alpaca_focuscube3::move(const int &pos) {
  throw_if_not_connected();
  using namespace std::chrono_literals;
  std::string move_cmd = fmt::format("FM:{:#d}\n", pos);
  _moving = true;
  auto resp = send_command_to_focuser(move_cmd);
  return 0;
}

std::map<std::string, device_variant_t>
pegasus_alpaca_focuscube3::details() {
  std::map<std::string, device_variant_t> detail_map;
  detail_map["Connected"] = _connected;
  detail_map["Serial Device"] = _serial_device_path;

  if (_connected) {
    detail_map["Temperature"] = _temperature;
    detail_map["Position"] = _position;
    detail_map["Moving"] = _moving;
    detail_map["Backlash"] = _backlash;
  }

  return detail_map;
};
