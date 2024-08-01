#include "pegasus_alpaca_ppba.hpp"
#include "common/alpaca_exception.hpp"
#include <regex>

std::vector<std::string> pegasus_alpaca_ppba::serial_devices() {
  std::vector<std::string> serial_devices;
  try {
    auto fs = std::filesystem::path("/dev/serial/by-id");

    for (auto dir_iter : std::filesystem::directory_iterator{fs}) {
      if (dir_iter.path().string().find("PPBADV") != std::string::npos) {
        spdlog::debug("Found Pocket Power Box Advanced: {}",
                      dir_iter.path().string());
        serial_devices.push_back(dir_iter.path().string());
      }
    }
  } catch (std::exception &e) {
    spdlog::error("Problem enumerating serial by id: {}", e.what());
  }

  return serial_devices;
}

pegasus_alpaca_ppba::pegasus_alpaca_ppba()
    : _connected(false), _voltage(0), _power(0), _current_of_12v_outputs(0),
      _total_current(0), _temp(0), _humidity(0), _quadport_on(true),
      _uptime_in_mins(0), _dew_point(0), _dew_a_pwm(0), _dew_b_pwm(0),
      _current_of_dewA(0), _current_of_dewB(0), _usb2_on_off(true),
      _autodew(true), _power_warning(false), _dew_aggressiveness(0),
      _serial_port(_io_context) {}

pegasus_alpaca_ppba::~pegasus_alpaca_ppba() {
  if (_connected) {
    _connected = false;
    _ppba_update_thread.join();
    _serial_port.close();
  }
}

void pegasus_alpaca_ppba::throw_if_not_connected() {
  if (!_connected)
    throw alpaca_exception(alpaca_exception::NOT_CONNECTED,
                           "Switch not connected");
}

std::string pegasus_alpaca_ppba::send_command_to_switch(const std::string &cmd,
                                                        bool read_response,
                                                        char stop_on_char) {

  try {
    spdlog::trace("sending: {} to switch", cmd);
    std::lock_guard lock(_ppba_mtx);
    char buf[512] = {0};
    _serial_port.write_some(asio::buffer(fmt::format("{}\n", cmd)));
    std::string rsp;

    if (read_response) {
      _io_context.reset();
      // TODO: we may need to make the read timeout configurable here
      alpaca_hub_serial::blocking_reader reader(cmd, _serial_port, 250,
                                                _io_context);
      char c;
      while (reader.read_char(c)) {
        if (c == stop_on_char || stop_on_char == '\0') {
          break;
        }
        if(c != '\r')
          rsp += c;
      }
    }

    spdlog::trace("switch returned: {}", rsp);
    return rsp;
  } catch (std::exception &ex) {
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("Problem sending command to switch: ", ex.what()));
  }
}

void pegasus_alpaca_ppba::update_properties() {

  // PPBA:voltage:current_of_12V_outputs_:
  // temp:humidity:dewpoint:quadport_status:
  // adj_output_status:dewA_power:dewB_power:
  // autodew_bool:pwr_warn:pwradj
  //
  // example: PPBA:12.2:0.5.22.2:45:17.2:1:1:120:130:1:0:1
  //          PPBA:12.9:111:22.4:50:11.5:1:1:0:0:1:0:3
  //                 ^   ^   ^   ^   ^   ^ ^ ^ ^ ^ ^ ^
  //                 |   |   |   |   |   | | | | | | |
  //            voltage  |   |   |   |   | | | | | | |
  //             current_12v |   |   |   | | | | | | |
  //                       temp  |   |   | | | | | | |
  //                        humidity |   | | | | | | |
  //                            dewpoint | | | | | | |
  //                              quadport | | | | | |
  //                                 adj_out | | | | |
  //                                      dewA | | | |
  //                                        dewB | | |
  //                                       autoDew | |
  //                                         pwr_wrn |
  //                                            pwradj
  spdlog::trace("Fetching Switch properties");
  auto resp = send_command_to_switch("PA");

  auto result = split(resp, ":");
  if (result[0] != "PPBA")
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        "Did not receive correctly formated data from focuser");

  spdlog::trace("Switch properties: {}", resp);

  _voltage = std::atof(result[1].c_str());
  // _current_of_12v_outputs = std::atof(result[2].c_str()) / 65;
  _temp = std::atof(result[3].c_str());
  _humidity = std::atoi(result[4].c_str());
  _dew_point = std::atof(result[5].c_str());
  if (result[6] == "1")
    _quadport_on = true;
  else
    _quadport_on = false;

  if (result[7] == "1")
    _adj_power_on = true;
  else
    _adj_power_on = false;
  _dew_a_pwm = std::atoi(result[8].c_str());
  _dew_b_pwm = std::atoi(result[9].c_str());
  if (result[10] == "1")
    _autodew = true;
  else
    _autodew = false;
  if (result[11] == "1")
    _power_warning = true;
  else
    _power_warning = false;
  _adj_power_voltage = std::atoi(result[12].c_str());

  resp = send_command_to_switch("PC");

  result = split(resp, ":");

  if (result[0] != "PC")
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        "Did not receive correct data from Print Power Metrics Command");

  _total_current = std::atof(result[1].c_str());
  _current_of_12v_outputs = std::atof(result[2].c_str());
  _current_of_dewA = std::atof(result[3].c_str());
  _current_of_dewB = std::atof(result[4].c_str());
  _uptime_in_mins = std::atoi(result[5].c_str()) / 60000.0;

  resp = send_command_to_switch("PS");

  result = split(resp, ":");

  if (result[0] != "PS")
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        "Did not receive correct data from Print Power Consumption Command");

  _power = std::atof(result[3].c_str());
  resp = send_command_to_switch("DA");
  result = split(resp, ":");
  if (result[0] != "DA")
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        "Did not receive correct data from switch for Dew Aggressiveness");

  _dew_aggressiveness = std::atoi(result[1].c_str());
}

void pegasus_alpaca_ppba::update_properties_proc() {
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

bool pegasus_alpaca_ppba::connected() { return _connected; }

int pegasus_alpaca_ppba::set_connected(bool connected) {
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

      char buf[512] = {0};

      // Send cmd for device status
      // _serial_port.write_some(asio::buffer("P#\n"));
      // _serial_port.read_some(asio::buffer(buf));
      auto resp = send_command_to_switch("P#");
      spdlog::debug("ppba returned {0}", resp);

      // Start update thread to values
      _ppba_update_thread = std::thread(
          std::bind(&pegasus_alpaca_ppba::update_properties_proc, this));
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
        _ppba_update_thread.join();
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

std::string pegasus_alpaca_ppba::unique_id() { return _serial_device_path; }

int pegasus_alpaca_ppba::set_serial_device(
    const std::string &serial_device_path) {
  _serial_device_path = serial_device_path;
  return 0;
}

std::string pegasus_alpaca_ppba::get_serial_device_path() {
  return _serial_device_path;
}

uint32_t pegasus_alpaca_ppba::interface_version() { return 3; }

std::string pegasus_alpaca_ppba::driver_version() { return "v0.1"; }

std::vector<std::string> pegasus_alpaca_ppba::supported_actions() {
  return std::vector<std::string>{};
}

std::string pegasus_alpaca_ppba::description() {
  return "Pegasus Pocket Power Box Advanced";
}

std::string pegasus_alpaca_ppba::driverinfo() {
  return "Pegasus Pocket Power Box Advanced Driver";
}

std::string pegasus_alpaca_ppba::name() {
  return "Pegasus Pocket Power Box Advanced";
}

uint32_t pegasus_alpaca_ppba::max_switch() { return 19; }

bool pegasus_alpaca_ppba::can_write(const uint32_t &switch_idx) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
  case CURRENT:
  case CURRENT_12V:
  case POWER:
  case TEMP:
  case HUMIDITY:
  case DEWPOINT:
  case POWERWARN:
  case UPTIME_MS:
  case DEWA_CURRENT:
  case DEWB_CURRENT:
    return false;
  case QUAD12V_ON_OFF:
  case ADJPOW_ON_OFF:
  case ADJVOLTAGE:
  case DEWA_PWM:
  case DEWB_PWM:
  case AUTODEW_ON_OFF:
  case DEW_AGGRESSIVENESS:
  case USB2_ON_OFF:
    return true;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }
}

bool pegasus_alpaca_ppba::get_switch(const uint32_t &switch_idx) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
  case CURRENT:
  case CURRENT_12V:
  case POWER:
  case TEMP:
  case HUMIDITY:
  case DEWPOINT:
    return true;
  case POWERWARN:
    return _power_warning;
  case QUAD12V_ON_OFF:
    return _quadport_on;
  case ADJPOW_ON_OFF:
    return _adj_power_on;
  case ADJVOLTAGE:
    if (_adj_power_voltage == 0)
      return false;
    return true;
  case DEWA_PWM:
    if (_dew_a_pwm == 0)
      return false;
    return true;
  case DEWB_PWM:
    if (_dew_b_pwm == 0)
      return false;
    return true;
  case DEWA_CURRENT:
    return true;
  case DEWB_CURRENT:
    return true;
  case AUTODEW_ON_OFF:
    return _autodew;
  case DEW_AGGRESSIVENESS:
    return _dew_aggressiveness;
  case USB2_ON_OFF:
    return _usb2_on_off;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }
}

std::string
pegasus_alpaca_ppba::get_switch_description(const uint32_t &switch_idx) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
    return "Input Voltage";
  case CURRENT:
    return "Current in Amps";
  case CURRENT_12V:
    return "Quad 12V Current in Amps";
  case POWER:
    return "Power in Watts";
  case TEMP:
    return "Temperature in Celsius";
  case HUMIDITY:
    return "Humidity %";
  case DEWPOINT:
    return "Dew Point in Celsius";
  case QUAD12V_ON_OFF:
    return "Quad 12V Output On/Off";
  case ADJPOW_ON_OFF:
    return "Adjustable Power Output On/Off";
  case ADJVOLTAGE:
    return "Adjustable Power Voltage (3/5/7/8/9/12)";
  case DEWA_PWM:
    return "Dew Heater A PWM Set Point (0-255)";
  case DEWB_PWM:
    return "Dew Heater B PWM Set Point (0-255)";
  case DEWA_CURRENT:
    return "Dew Heater A Current in Amps";
  case DEWB_CURRENT:
    return "Dew Heater B Current in Amps";
  case AUTODEW_ON_OFF:
    return "AutoDew (On/Off)";
  case POWERWARN:
    return "Power Warning";
  case UPTIME_MS:
    return "Minutes of Uptime";
  case DEW_AGGRESSIVENESS:
    return "Auto Dew Aggressiveness, a higher value sets higher power based on "
           "humidity (1-254)";
  case USB2_ON_OFF:
    return "USB2 Ports (On/Off)";
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }

  throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                         "{} is not a valid switch index");
}

std::string pegasus_alpaca_ppba::get_switch_name(const uint32_t &switch_idx) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
    return "Voltage";
  case CURRENT:
    return "Total Current";
  case CURRENT_12V:
    return "Current 12V";
  case POWER:
    return "Power";
  case TEMP:
    return "Temperature";
  case HUMIDITY:
    return "Humidity";
  case DEWPOINT:
    return "DewPoint";
  case QUAD12V_ON_OFF:
    return "Quad 12V Output";
  case ADJPOW_ON_OFF:
    return "Adj Power Output";
  case ADJVOLTAGE:
    return "Adj Power Voltage";
  case DEWA_PWM:
    return "DewA PWM";
  case DEWB_PWM:
    return "DewB PWM";
  case DEWA_CURRENT:
    return "DewA Power";
  case DEWB_CURRENT:
    return "DewB Power";
  case AUTODEW_ON_OFF:
    return "Auto Dew";
  case POWERWARN:
    return "Power Warning";
  case UPTIME_MS:
    return "Uptime";
  case DEW_AGGRESSIVENESS:
    return "Autodew Aggressiveness";
  case USB2_ON_OFF:
    return "USB2 Ports";
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }
}

double pegasus_alpaca_ppba::get_switch_value(const uint32_t &switch_idx) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
    return _voltage;
  case CURRENT:
    return _total_current;
  case CURRENT_12V:
    return _current_of_12v_outputs;
  case POWER:
    return _power;
  case TEMP:
    return _temp;
  case HUMIDITY:
    return _humidity;
  case DEWPOINT:
    return _dew_point;
  case QUAD12V_ON_OFF:
    return _quadport_on;
  case ADJPOW_ON_OFF:
    return _adj_power_on;
  case ADJVOLTAGE:
    return _adj_power_voltage;
  case DEWA_PWM:
    return _dew_a_pwm;
  case DEWB_PWM:
    return _dew_b_pwm;
  case DEWA_CURRENT:
    return _current_of_dewA;
  case DEWB_CURRENT:
    return _current_of_dewB;
  case AUTODEW_ON_OFF:
    return _autodew;
  case POWERWARN:
    return _power_warning;
  case UPTIME_MS:
    return _uptime_in_mins;
  case DEW_AGGRESSIVENESS:
    return _dew_aggressiveness;
  case USB2_ON_OFF:
    return _usb2_on_off;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }
}

double pegasus_alpaca_ppba::min_switch_value(const uint32_t &switch_idx) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
  case CURRENT:
  case CURRENT_12V:
  case POWER:
  case HUMIDITY:
  case QUAD12V_ON_OFF:
  case ADJPOW_ON_OFF:
  case ADJVOLTAGE:
  case DEWA_PWM:
  case DEWB_PWM:
  case DEWA_CURRENT:
  case DEWB_CURRENT:
  case AUTODEW_ON_OFF:
  case POWERWARN:
  case UPTIME_MS:
  case USB2_ON_OFF:
    return 0;
  case TEMP:
    return -40;
  case DEWPOINT:
    return -40;
  case DEW_AGGRESSIVENESS:
    return 1;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }
}

double pegasus_alpaca_ppba::max_switch_value(const uint32_t &switch_idx) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
    return 15;
  case CURRENT:
  case CURRENT_12V:
    return 10;
  case POWER:
    return 120;
  case TEMP:
    return 50;
  case HUMIDITY:
    return 100;
  case DEWPOINT:
    return 50;
  case QUAD12V_ON_OFF:
    return 1;
  case ADJPOW_ON_OFF:
    return 1;
  case ADJVOLTAGE:
    return 12;
  case DEWA_PWM:
  case DEWB_PWM:
    return 255;
  case DEWA_CURRENT:
  case DEWB_CURRENT:
    return 60;
  case AUTODEW_ON_OFF:
  case POWERWARN:
    return 1;
  case UPTIME_MS:
    return 9007199254740992;
  case DEW_AGGRESSIVENESS:
    return 254;
  case USB2_ON_OFF:
    return 1;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }
}

int pegasus_alpaca_ppba::set_switch(const uint32_t &switch_idx,
                                    const bool &switch_state) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
  case CURRENT:
  case CURRENT_12V:
  case POWER:
  case TEMP:
  case HUMIDITY:
  case DEWPOINT:
  case DEWA_CURRENT:
  case DEWB_CURRENT:
  case POWERWARN:
  case UPTIME_MS:
    throw alpaca_exception(
        alpaca_exception::INVALID_OPERATION,
        fmt::format("Switch at {} doesn't support write.", switch_idx));
  case QUAD12V_ON_OFF:
    if (switch_state == true) {
      auto resp = send_command_to_switch("P1:1");
      if (resp != "P1:1")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem enabling Quad 12V outputs.");
    } else {
      auto resp = send_command_to_switch("P1:0");
      if (resp != "P1:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem disabling Quad 12V outputs.");
    }
    break;
  case ADJVOLTAGE:
  case ADJPOW_ON_OFF:
    if (switch_state == true) {
      auto resp = send_command_to_switch("P2:1");
      if (resp != "P2:1")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem enabling Adjustable output.");
    } else {
      auto resp = send_command_to_switch("P2:0");
      if (resp != "P2:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem disabling variable output.");
    }
    break;
  case DEWA_PWM:
    if (switch_state == true) {
      auto resp = send_command_to_switch("P3:255");
      if (resp != "P3:255")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem enabling DewA.");
    } else {
      auto resp = send_command_to_switch("P3:0");
      if (resp != "P3:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem disabling DewA.");
    }
    break;
  case DEWB_PWM:
    if (switch_state == true) {
      auto resp = send_command_to_switch("P4:255");
      if (resp != "P4:255")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem enabling DewB.");
    } else {
      auto resp = send_command_to_switch("P4:0");
      if (resp != "P4:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem disabling DewB.");
    }

    break;
  case AUTODEW_ON_OFF:
    if (switch_state == true) {
      auto resp = send_command_to_switch("PD:1");
      if (resp != "PD:1")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem enabling Autodew.");
    } else {
      auto resp = send_command_to_switch("PD:0");
      if (resp != "PD:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem disabling Autodew.");
    }

    break;
  case DEW_AGGRESSIVENESS:
    if (switch_state == true) {
      auto resp = send_command_to_switch("PD:254");
      if (resp != "PD:254")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem enabling Autodew.");
    } else {
      auto resp = send_command_to_switch("PD:0");
      if (resp != "PD:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem disabling Autodew.");
    }

    break;
  case USB2_ON_OFF:
    if (switch_state == true) {
      auto resp = send_command_to_switch("PU:1");
      if (resp != "PU:1")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem enabling USB2 Ports.");
      _usb2_on_off = true;
    } else {
      auto resp = send_command_to_switch("PU:0");
      if (resp != "PU:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem disabling USB2 Ports.");
      _usb2_on_off = false;
    }

    break;

  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }

  return 0;
}

int pegasus_alpaca_ppba::set_switch_name(const uint32_t &switch_idx,
                                         const std::string &switch_name) {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Setting switch names is not supported");
};

// TODO: This code needs to be DRYed up a bit...there's a lot of
// duplication that should be refactored
int pegasus_alpaca_ppba::set_switch_value(const uint32_t &switch_idx,
                                          const double &switch_value) {

  spdlog::debug("set_switch_value called for switch {} ({}) with val: {}",
                switch_idx, get_switch_name(switch_idx), switch_value);
  switch (switch_idx) {
  case INPUT_VOLTAGE:
  case CURRENT:
  case CURRENT_12V:
  case POWER:
  case TEMP:
  case HUMIDITY:
  case DEWPOINT:
  case POWERWARN:
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Switch at {} doesn't support write operation");
  case QUAD12V_ON_OFF:
    if (switch_value < 0 || switch_value > 1)
      throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                             "Invalid value for turning quad 12v on/off");
    if (switch_value > 0) {
      auto resp = send_command_to_switch("P1:1");
      if (resp != "P1:1")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem enabling quad 12v");
    } else {
      auto resp = send_command_to_switch("P1:0");
      if (resp != "P1:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem disabling quad 12v");
    }
    break;
  case ADJPOW_ON_OFF:
    if (switch_value < 0 || switch_value > 1)
      throw alpaca_exception(
          alpaca_exception::INVALID_VALUE,
          "Invalid value for turning adj power output on/off");
    if (switch_value > 0) {
      auto resp = send_command_to_switch("P2:1");
      if (resp != "P2:1")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting adj power output on");
    } else {
      auto resp = send_command_to_switch("P2:0");
      if (resp != "P2:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting adj power output off");
    }
    break;
  case ADJVOLTAGE:
    if (switch_value < 3 || switch_value > 12)
      throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                             "Adjustable voltage must be between 3 and 12");
    if (switch_value >= 3 && switch_value < 5) {
      auto resp = send_command_to_switch("P2:3");
      if (resp != "P2:3")
        throw alpaca_exception(
            alpaca_exception::DRIVER_ERROR,
            "Problem setting adj power output, driver error");
    } else if (switch_value >= 5 && switch_value < 7) {
      auto resp = send_command_to_switch("P2:5");
      if (resp != "P2:5")
        throw alpaca_exception(
            alpaca_exception::DRIVER_ERROR,
            "Problem setting adj power output, driver error");
    } else if (switch_value >= 7 && switch_value < 8) {
      auto resp = send_command_to_switch("P2:7");
      if (resp != "P2:7")
        throw alpaca_exception(
            alpaca_exception::DRIVER_ERROR,
            "Problem setting adj power output, driver error");
    } else if (switch_value > 7 && switch_value <= 8) {
      auto resp = send_command_to_switch("P2:8");
      if (resp != "P2:8")
        throw alpaca_exception(
            alpaca_exception::DRIVER_ERROR,
            "Problem setting adj power output, driver error");
    } else if (switch_value > 8 && switch_value <= 9) {
      auto resp = send_command_to_switch("P2:9");
      if (resp != "P2:9")
        throw alpaca_exception(
            alpaca_exception::DRIVER_ERROR,
            "Problem setting adj power output, driver error");
    } else if (switch_value > 9 && switch_value <= 12) {
      auto resp = send_command_to_switch("P2:12");
      if (resp != "P2:12")
        throw alpaca_exception(
            alpaca_exception::DRIVER_ERROR,
            "Problem setting adj power output, driver error");
    }
    break;
  case DEWA_PWM:
    if (switch_value < 0 || switch_value > 255)
      throw alpaca_exception(
          alpaca_exception::INVALID_VALUE,
          fmt::format("Invalid value for DewA: {}", switch_value));
    else {
      auto resp = send_command_to_switch(fmt::format("P3:{}", switch_value));
      if (resp != fmt::format("P3:{}", switch_value))
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting value for DewA");
    }
    break;
  case DEWB_PWM:
    if (switch_value < 0 || switch_value > 255)
      throw alpaca_exception(
          alpaca_exception::INVALID_VALUE,
          fmt::format("Invalid value for DewB: {}", switch_value));
    else {
      auto resp = send_command_to_switch(fmt::format("P4:{}", switch_value));
      if (resp != fmt::format("P4:{}", switch_value))
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting value for DewB");
    }
    break;
  case DEWA_CURRENT:
    if (switch_value < 0 || switch_value > 1)
      throw alpaca_exception(
          alpaca_exception::INVALID_VALUE,
          fmt::format("Invalid value for DewA on/off: {}", switch_value));
    else {
      auto resp = send_command_to_switch(fmt::format("P3:{}", switch_value));
      if (resp != fmt::format("P3:{}", switch_value))
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting value for DewA ON/OFF");
    }
    break;
  case DEWB_CURRENT:
    if (switch_value < 0 || switch_value > 1)
      throw alpaca_exception(
          alpaca_exception::INVALID_VALUE,
          fmt::format("Invalid value for DewB on/off: {}", switch_value));
    else {
      auto resp = send_command_to_switch(fmt::format("P4:{}", switch_value));
      if (resp != fmt::format("P4:{}", switch_value))
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting value for DewB ON/OFF");
    }
    break;
  case AUTODEW_ON_OFF:
    if (switch_value < 0 || switch_value > 1)
      throw alpaca_exception(
          alpaca_exception::INVALID_VALUE,
          fmt::format("Invalid value for Autodew on/off: {}", switch_value));
    else {
      auto resp = send_command_to_switch(fmt::format("PD:{}", switch_value));
      if (resp != fmt::format("PD:{}", switch_value))
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting value for Autodew ON/OFF");
    }
    break;
  case DEW_AGGRESSIVENESS:
    if (switch_value < 1 || switch_value > 254)
      throw alpaca_exception(
          alpaca_exception::INVALID_VALUE,
          fmt::format("Invalid value for Autodew Aggressiveness: {}",
                      switch_value));
    else {
      auto resp = send_command_to_switch(fmt::format("PD:{}", switch_value));
      if (resp != fmt::format("PD:{}", switch_value))
        throw alpaca_exception(
            alpaca_exception::DRIVER_ERROR,
            "Problem setting value for Autodew Aggressiveness");
    }
    break;
  case USB2_ON_OFF:
    if (switch_value < 0 || switch_value > 1)
      throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                             "Invalid value for turning USB2 ports on/off");
    if (switch_value > 0) {
      spdlog::debug("Turning USB2 on...");

      auto resp = send_command_to_switch("PU:1");
      if (resp != "PU:1")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting USB2 Ports on");
      _usb2_on_off = true;
    } else {
      spdlog::debug("Turning USB2 off...");
      auto resp = send_command_to_switch("PU:0");
      spdlog::debug("response from switch: \"{}\"", resp);
      if (resp != "PU:0")
        throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                               "Problem setting USB2 Ports off");
      spdlog::debug("USB2 should be off...");
      _usb2_on_off = false;
    }
    break;

  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }
  return 0;
};

double pegasus_alpaca_ppba::switch_step(const uint32_t &switch_idx) {
  switch (switch_idx) {
  case INPUT_VOLTAGE:
  case CURRENT:
  case CURRENT_12V:
  case POWER:
  case TEMP:
  case HUMIDITY:
  case DEWPOINT:
  case POWERWARN:
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Switch at {} doesn't support write operation");
  case QUAD12V_ON_OFF:
  case ADJPOW_ON_OFF:
  case ADJVOLTAGE:
  case DEWA_PWM:
  case DEWB_PWM:
  case DEWA_CURRENT:
  case DEWB_CURRENT:
  case AUTODEW_ON_OFF:
  case DEW_AGGRESSIVENESS:
  case USB2_ON_OFF:
    return 1;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "{} is not a valid switch index");
  }
};

std::map<std::string, device_variant_t> pegasus_alpaca_ppba::details() {
  std::map<std::string, device_variant_t> detail_map;
  detail_map["Connected"] = _connected;
  detail_map["Serial Device"] = _serial_device_path;

  if (_connected) {
    detail_map["Voltage"] = _voltage;
    detail_map["Power"] = _power;
    detail_map["Current"] = _total_current;
    detail_map["Current 12V"] = _current_of_12v_outputs;
    detail_map["Temperature"] = _temp;
    detail_map["Humidity"] = _humidity;
    detail_map["Quad Port 12V On"] = _quadport_on;
    detail_map["Uptime (mins)"] = _uptime_in_mins;
    detail_map["Dewpoint"] = _dew_point;
    detail_map["DewA PWM"] = _dew_a_pwm;
    detail_map["DewB PWM"] = _dew_b_pwm;
    detail_map["DewA Current"] = _current_of_dewA;
    detail_map["DewB Current"] = _current_of_dewB;
    detail_map["Autodew"] = _autodew;
    detail_map["Autodew Aggressiveness"] = _dew_aggressiveness;
    detail_map["Power Warning"] = _power_warning;
    detail_map["Adj Output On"] = _adj_power_on;
    detail_map["Adj Output Voltage"] = _adj_power_voltage;
    detail_map["USB2 Ports On"] = _usb2_on_off;
  }

  return detail_map;
}
