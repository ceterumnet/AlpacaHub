#include "pegasus_alpaca_focuser.hpp"
#include "common/alpaca_exception.hpp"
#include "common/alpaca_hub_serial.hpp"
#include <chrono>
#include <mutex>
#include <regex>
#include <thread>

std::vector<std::string> pegasus_alpaca_focuser::serial_devices() {
  std::vector<std::string> serial_devices{
      "/dev/serial/by-id/usb-PegasusAstro_FocusCube3_48:27:e2:44:73:14-if00"};
  return serial_devices;
}

pegasus_alpaca_focuser::pegasus_alpaca_focuser()
    : _connected(false), _moving(false), _serial_port(_io_context) {}

pegasus_alpaca_focuser::~pegasus_alpaca_focuser() {
  _connected = false;
  _focuser_update_thread.join();
  _serial_port.close();
}

uint32_t pegasus_alpaca_focuser::interface_version() { return 3; }

std::string pegasus_alpaca_focuser::driver_version() { return "v0.1"; }

std::vector<std::string> pegasus_alpaca_focuser::supported_actions() {
  return std::vector<std::string>();
}

std::string pegasus_alpaca_focuser::description() {
  return "Pegasus Electronic Focuser";
}

std::string pegasus_alpaca_focuser::driverinfo() {
  return "AlpacaHub Pegasus Focuser Driver";
}
std::string pegasus_alpaca_focuser::name() { return "Pegasus Focus Cube"; }

int pegasus_alpaca_focuser::set_connected(bool connected) {
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
          std::bind(&pegasus_alpaca_focuser::update_properties_proc, this));
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
      _connected = false;
      _focuser_update_thread.join();
      _serial_port.close();
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
void pegasus_alpaca_focuser::throw_if_not_connected() {
  if (!_connected)
    throw alpaca_exception(alpaca_exception::NOT_CONNECTED,
                           "Focuser not connected");
}

bool pegasus_alpaca_focuser::connected() { return _connected; }

std::vector<std::string> split(const std::string &input,
                               const std::string &regex) {
  // passing -1 as the submatch index parameter performs splitting
  std::regex pattern(regex);
  std::sregex_token_iterator first{input.begin(), input.end(), pattern, -1},
      last;
  return {first, last};
}

void pegasus_alpaca_focuser::update_properties() {
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

void pegasus_alpaca_focuser::update_properties_proc() {
  using namespace std::chrono_literals;
  while(_connected) {
    try {
      update_properties();
    } catch(alpaca_exception &ex) {
      spdlog::warn("problem during update_properties: {}", ex.what());
    }
    std::this_thread::sleep_for(500ms);
  }
  spdlog::debug("update_properties_proc ended");
}

std::string pegasus_alpaca_focuser::send_command_to_focuser(
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

std::string pegasus_alpaca_focuser::unique_id() { return _serial_device_path; }

int pegasus_alpaca_focuser::set_serial_device(
    const std::string &serial_device_path) {
  _serial_device_path = serial_device_path;
  return 0;
};

std::string pegasus_alpaca_focuser::get_serial_device_path() {
  return _serial_device_path;
};

bool pegasus_alpaca_focuser::absolute() {
  throw_if_not_connected();
  return true;
}

bool pegasus_alpaca_focuser::is_moving() {
  throw_if_not_connected();
  return _moving;
}

// Just picking an arbitrary value at the moment...
uint32_t pegasus_alpaca_focuser::max_increment() {
  throw_if_not_connected();
  return 500;
}

// Need to figure out what this is supposed to be for the pegasus
uint32_t pegasus_alpaca_focuser::max_step() {
  throw_if_not_connected();
  return 1000000;
}

uint32_t pegasus_alpaca_focuser::position() {
  throw_if_not_connected();
  return _position;
}

uint32_t pegasus_alpaca_focuser::step_size() {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Focuser does not intrisically know step size.");
  return 0;
}

// TODO: need to determine whether or not I want to implement this
bool pegasus_alpaca_focuser::temp_comp() {
  throw_if_not_connected();
  return false;
}

bool pegasus_alpaca_focuser::temp_comp_available() {
  throw_if_not_connected();
  return false;
}

int pegasus_alpaca_focuser::set_temp_comp(bool enable_temp_comp) {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED, "Temp comp not implemented");
  return 0;
}

double pegasus_alpaca_focuser::temperature() {
  throw_if_not_connected();
  return _temperature;
}

int pegasus_alpaca_focuser::halt() {
  throw_if_not_connected();
  send_command_to_focuser("FH\n", false);
  return 0;
}

int pegasus_alpaca_focuser::move(const int &pos) {
  throw_if_not_connected();
  using namespace std::chrono_literals;
  std::string move_cmd = fmt::format("FM:{:#d}\n", pos);
  auto resp = send_command_to_focuser(move_cmd);
  // Well...let's go ahead and set this to true so that we
  // allow it to automatically be updated during the normal update
  // thread
  _moving = true;
  // while(_moving)
  //   std::this_thread::sleep_for(100ms);
  return 0;
}

device_variant_t pegasus_alpaca_focuser::details() {
  std::map<std::string, device_variant_intermediate_t> detail_map;
  detail_map["Connected"] = _connected;
  detail_map["Serial Device"] = _serial_device_path;

  if(_connected) {
    detail_map["Temperature"] = _temperature;
    detail_map["Position"] = _position;
    detail_map["Moving"] = _moving;
    detail_map["Backlash"] = _backlash;
  }

  return detail_map;
};