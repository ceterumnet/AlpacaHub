#include "primaluce_focuser_rotator.hpp"
#include "common/alpaca_exception.hpp"
#include "interfaces/i_alpaca_device.hpp"
#include <memory>

// TODO: I need to refactor this into common
void arco_rotator::throw_if_not_connected() {
  if (!_connected)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Rotator must be connected");
}

// Rotator functions
arco_rotator::arco_rotator(esatto_focuser &focuser)
    : _focuser(focuser), _is_moving(false), _mechanical_position_deg(0),
      _mechanical_position_step(0), _mechanical_position_arcsec(0),
      _position_deg(0), _position_step(0), _position_arcsec(0),
      _reversed(false), _target_position_deg(0), _connected(false) {}

arco_rotator::~arco_rotator() {}

std::map<std::string, device_variant_t> arco_rotator::details() {
  std::map<std::string, device_variant_t> detail_map;
  detail_map["Connected"] = _connected;

  if (_connected) {
    detail_map["Position (deg)"] = _position_deg;
    detail_map["Position (steps)"] = _position_step;
    detail_map["Position (arcsec)"] = _position_arcsec;

    detail_map["Mechanical Position (deg)"] = _mechanical_position_deg;
    detail_map["Mechanical Position (steps)"] = _mechanical_position_step;
    detail_map["Mechanical Position (arcsec)"] = _mechanical_position_arcsec;

    detail_map["Position Offset from Mechanical (deg)"] =
        _mechanical_position_deg;
    detail_map["Position Offset from Mechanical (steps)"] =
        _mechanical_position_step;
    detail_map["Position Offset from Mechanical (arcsec)"] =
        _mechanical_position_arcsec;

    detail_map["Target Position (deg)"] = _target_position_deg;
    detail_map["Moving"] = _is_moving;
    detail_map["Reversed"] = _reversed;
  }

  return detail_map;
};

std::string arco_rotator::send_command_to_rotator(const std::string &cmd,
                                                  bool read_response,
                                                  char stop_on_char) {

  // The arco is simply a peripheral that is connected to the focuser.
  return _focuser.send_command_to_focuser(cmd);
};

bool arco_rotator::connected() { return _connected; };

std::vector<std::string> arco_rotator::supported_actions() {
  return std::vector<std::string>();
};

std::string arco_rotator::unique_id() { return "unique_id_for_rotator123131"; };

int arco_rotator::set_connected(bool connected) {
  // TODO: I'm not sure if I need to do anything else here...depends on how I
  // handle the update process
  if (!_focuser.connected())
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Focuser must be connected first.");
  _connected = connected;
  return 0;
}

std::string arco_rotator::description() { return "ARCO Robotic Rotator"; }

std::string arco_rotator::driverinfo() {
  return "AlpacaHub Driver for ARCO Robotic Rotator";
}

std::string arco_rotator::name() { return "ARCO Rotator"; }

uint32_t arco_rotator::interface_version() { return 3; }

std::string arco_rotator::driver_version() { return "v0.1"; }

bool arco_rotator::can_reverse() {
  throw_if_not_connected();
  return true;
};

bool arco_rotator::is_moving() {
  throw_if_not_connected();
  return _is_moving;
};

double arco_rotator::position() {
  throw_if_not_connected();
  return _position_deg;
};

double arco_rotator::mechanical_position() {
  throw_if_not_connected();
  return _mechanical_position_deg;
};

bool arco_rotator::reverse() {
  throw_if_not_connected();
  return _reversed;
};

int arco_rotator::set_reverse(bool reverse) {
  throw_if_not_connected();
  int reversed = reverse ? 1 : 0;
  auto resp = send_command_to_rotator(set_reverse_mot2_cmd(reversed));
    // TODO: check for errors
  return 0;

  };

double arco_rotator::step_size() {
  throw_if_not_connected();
  return 0.1;
};

double arco_rotator::target_position() {
  throw_if_not_connected();
  return _target_position_deg;
};

int arco_rotator::halt() {
  throw_if_not_connected();
  auto resp = send_command_to_rotator(cmd_abort_mot2_cmd());
  // TODO: check for errors
  return 0;
};

int arco_rotator::move(const double &position) {
  throw_if_not_connected();
  _target_position_deg = _position_deg + position;

  auto resp = send_command_to_rotator(cmd_move_mot2_cmd(position));
  // TODO: check for errors
  return 0;
};

int arco_rotator::moveabsolute(const double &absolute_position) {
  throw_if_not_connected();
  _target_position_deg = absolute_position;
  auto resp = send_command_to_rotator(cmd_move_abs_mot2_cmd(absolute_position));
  // TODO: check for errors
  return 0;
};

int arco_rotator::movemechanical(const double &mechanical_position) {
  throw_if_not_connected();
  // Need to calculate the absolute position
  double absolute_calculated =
      mechanical_position + _position_offset_from_mechanical_deg;
  spdlog::debug("mechanical position: {}", mechanical_position);
  spdlog::debug("moving to {}", absolute_calculated);
  _target_position_deg = absolute_calculated;
  auto resp = send_command_to_rotator(
      cmd_move_abs_mot2_cmd(absolute_calculated));
  // TODO: check for errors
  return 0;
};

int arco_rotator::sync(const double &sync_position) {
  throw_if_not_connected();
  // Need to calculate the absolute position
  auto resp = send_command_to_rotator(cmd_sync_pos_mot2_cmd(sync_position));
  // TODO: check for errors
  return 0;
};

// commands

// I'm not sure if this will be used
std::string arco_rotator::set_arco_enabled_cmd(const bool &enabled) {
  int enable = enabled ? 1 : 0;
  primaluce_kv_node root;
  root.create_object("req")->create_object("set")->push_param("ARCO", enable);
  return root.to_json().dump();
};

std::string arco_rotator::get_mot2_status_cmd() {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("get")
      ->create_object("MOT2")
      ->push_param("STATUS", "");
  return root.to_json().dump();
};

bool valid_rotator_unit(const std::string &unit) {
  if (unit == "DEG" || unit == "ARCSEC" || unit == "STEP")
    return true;
  return false;
}
std::string arco_rotator::cmd_sync_pos_mot2_cmd(const double &pos,
                                                const std::string unit) {
  if (!valid_rotator_unit(unit))
    return "";
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT2")
      ->create_object("SYNC_POS")
      ->push_param(unit, pos);
  return root.to_json().dump();
};

// Is the offset between the Absolute and Mechanical Positions.This
// parameter supports only the "get" operation
// COMPENSATION_POS_STEP
// COMPENSATION_POS_DEG
// COMPENSATION_POS_ARCSEC
std::string
arco_rotator::get_mot2_compensation_pos_cmd(const std::string unit) {
  if (!valid_rotator_unit(unit))
    return "";

  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("get")
      ->create_object("MOT2")
      ->push_param(fmt::format("COMPENSATION_POS_{}", unit), "");
  return root.to_json().dump();
};

// Return the current Rotator position, allowing for any sync offset
// POSITION_STEP = ABS_POS_STEP + COMPENSATION_POS_STEP
//
// Thisparameter supports only the "get" operation POSITION_STEP is same
// of POSITION
std::string arco_rotator::get_mot2_pos_cmd(const std::string unit) {
  if (!valid_rotator_unit(unit))
    return "";
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("get")
      ->create_object("MOT2")
      ->push_param(fmt::format("POSITION_{}", unit), "");
  return root.to_json().dump();
};

// This command returns the raw mechanical position of the rotator.
std::string arco_rotator::get_mot2_abs_pos_deg_cmd(const std::string unit) {
  if (!valid_rotator_unit(unit))
    return "";
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("get")
      ->create_object("MOT2")
      ->push_param(fmt::format("ABS_POS_{}", unit), "");
  return root.to_json().dump();
};

// Move the rotator to the specified absolute position This command
// is similar to the GOTO command, except it works with the absolute
// position
std::string arco_rotator::cmd_move_abs_mot2_cmd(const double &pos,
                                                const std::string unit) {
  if (!valid_rotator_unit(unit))
    return "";
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT2")
      ->create_object("MOVE_ABS")
      ->push_param(unit, pos);
  return root.to_json().dump();
};

// Same as MOVE_ABS command, but writes logs in the shell at every
// second.
std::string
arco_rotator::cmd_verbose_move_abs_mot2_cmd(const double &pos,
                                            const std::string unit) {
  if (!valid_rotator_unit(unit))
    return "";
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT2")
      ->create_object("VERBOSE_MOVE_ABS")
      ->push_param(unit, pos);
  return root.to_json().dump();
};

// Causes the rotator to move Position relative to the current
// Position value.The number value could be positive or negative.
std::string arco_rotator::cmd_move_mot2_cmd(const double &pos,
                                            const std::string unit) {
  if (!valid_rotator_unit(unit))
    return "";
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT2")
      ->create_object("MOVE")
      ->push_param(unit, pos);
  return root.to_json().dump();
};

// Same as MOVE command, but writes logs in the shell at every
// second.The value could be set in "DEG", "ARCSEC" or "STEP"
std::string arco_rotator::cmd_verbose_move_mot2_cmd(const double &pos,
                                                    const std::string unit) {
  if (!valid_rotator_unit(unit))
    return "";
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT2")
      ->create_object("VERBOSE_MOVE")
      ->push_param(unit, pos);
  return root.to_json().dump();
};

// Get/Set the Hemisphere status. Its value will influence the
// motor's direction and degrees sign.
std::string arco_rotator::get_hemisphere_mot2_cmd() {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("get")
      ->create_object("MOT2")
      ->push_param("HEMISPHERE", "");
  return root.to_json().dump();
};
// Values are "northern" and "southern".
std::string
arco_rotator::set_hemisphere_mot2_cmd(const std::string &hemisphere) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("set")
      ->create_object("MOT2")
      ->push_param("HEMISPHERE", hemisphere);
  return root.to_json().dump();
};

// Stop the motor without deceleration
std::string arco_rotator::cmd_abort_mot2_cmd() {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT2")
      ->push_param("MOT_ABORT", "");
  return root.to_json().dump();
};

// Stop the motor with previous deceleration
std::string arco_rotator::cmd_stop_mot2_cmd() {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT2")
      ->push_param("MOT_ABORT", "");
  return root.to_json().dump();
};

// 0: normal angular direction
// 1: reversed
std::string arco_rotator::get_reverse_mot2_cmd() {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("get")
      ->create_object("MOT2")
      ->push_param("REVERSE", "");
  return root.to_json().dump();
};

// 0: normal angular direction
// 1: reversed
std::string arco_rotator::set_reverse_mot2_cmd(const int &reverse) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("set")
      ->create_object("MOT2")
      ->push_param("REVERSE", reverse);
  return root.to_json().dump();
};

// END arco functions

// Focuser functions
std::vector<std::string> esatto_focuser::serial_devices() {
  std::vector<std::string> device_paths;
  return device_paths;
};

esatto_focuser::esatto_focuser(const std::string &serial_device_path)
    : _serial_device_path(serial_device_path), _connected(false),
      _is_moving(false), _position(0), _temperature(0), _backlash(0),
      _serial_port(_io_context), _arco_present(false), _step_size(1) {
  spdlog::debug("Setting connected to true");
  spdlog::debug("Attempting to open serial device at {0}", _serial_device_path);
  _serial_port.open(_serial_device_path);
  _serial_port.set_option(asio::serial_port_base::baud_rate(115200));
  _serial_port.set_option(asio::serial_port_base::character_size(8));
  _serial_port.set_option(asio::serial_port_base::flow_control(
      asio::serial_port_base::flow_control::none));
  _serial_port.set_option(
      asio::serial_port_base::parity(asio::serial_port_base::parity::none));
  _serial_port.set_option(asio::serial_port_base::stop_bits(
      asio::serial_port_base::stop_bits::one));
};

esatto_focuser::~esatto_focuser() {
  if (_connected) {
    _connected = false;
    _focuser_update_thread.join();
    _serial_port.close();
  }
};

void esatto_focuser::init_rotator() {
  // Check if the rotator is present
  spdlog::debug("Checking for ARCO rotator");
  auto resp = send_command_to_focuser(get_all_system_data_cmd());
  auto parsed_resp = nlohmann::json::parse(resp);
  if (parsed_resp["res"]["get"]["ARCO"] == 1) {
    spdlog::debug("ARCO detected");
    _arco_present = true;
  } else
    spdlog::debug("ARCO not detected");

  // Create rotator
  if (_arco_present) {
    _rotator = std::make_shared<arco_rotator>(*this);
    _rotator->_target_position_deg =
        parsed_resp["res"]["get"]["MOT2"]["POSITION_DEG"];
  }
}

std::shared_ptr<arco_rotator> esatto_focuser::rotator() { return _rotator; }

bool esatto_focuser::connected() { return _connected; };

void esatto_focuser::update_properties() {
  std::string resp;
  try {
    spdlog::trace("sending get_all_system_data_cmd");
    resp = send_command_to_focuser(get_all_system_data_cmd());
    spdlog::trace("   returned: {}", resp);

    auto all_system_data = nlohmann::json::parse(resp)["res"]["get"];

    spdlog::trace("sending get_mot1_status_cmd");
    resp = send_command_to_focuser(get_mot1_status_cmd());
    spdlog::trace("   returned: {}", resp);

    auto mot1_status_data = nlohmann::json::parse(resp)["res"]["get"]["MOT1"];

    _position = all_system_data["MOT1"]["POSITION"];

    _is_moving = mot1_status_data["STATUS"]["MST"] == "stop" ? false : true;
    _temperature =
        std::atof(all_system_data["EXT_T"].get<std::string>().c_str());
    _backlash = all_system_data["MOT1"]["BKLASH"];

    // If we have an arco, let's update the properties here
    if (_arco_present && _rotator->_connected) {
      spdlog::trace("getting reverse");
      _rotator->_reversed = all_system_data["MOT2"]["REVERSE"] == 1;

      spdlog::trace("getting position_step");
      _rotator->_position_step = all_system_data["MOT2"]["POSITION_STEP"];

      spdlog::trace("getting position_deg");
      _rotator->_position_deg = all_system_data["MOT2"]["POSITION_DEG"];

      spdlog::trace("getting position_arcsec");
      _rotator->_position_arcsec = all_system_data["MOT2"]["POSITION_ARCSEC"];

      spdlog::trace("getting compensation_deg");
      _rotator->_position_offset_from_mechanical_deg =
          all_system_data["MOT2"]["COMPENSATION_POS_DEG"];

      spdlog::trace("getting compensation_arcsec");
      _rotator->_position_offset_from_mechanical_arcsec =
          all_system_data["MOT2"]["COMPENSATION_POS_ARCSEC"];

      spdlog::trace("getting compensation_step");
      _rotator->_position_offset_from_mechanical_step =
          all_system_data["MOT2"]["COMPENSATION_POS_STEP"];

      spdlog::trace("getting abs_pos_deg");
      _rotator->_mechanical_position_deg =
          all_system_data["MOT2"]["ABS_POS_DEG"];

      spdlog::trace("getting abs_pos_arcsec");
      _rotator->_mechanical_position_arcsec =
          all_system_data["MOT2"]["ABS_POS_ARCSEC"];

      spdlog::trace("getting abs_pos_step");
      _rotator->_mechanical_position_step =
        all_system_data["MOT2"]["ABS_POS_STEP"];

      resp = send_command_to_focuser(_rotator->get_mot2_status_cmd());
      auto mot2_status_data = nlohmann::json::parse(resp)["res"]["get"]["MOT2"];
      _is_moving = mot2_status_data["STATUS"]["MST"] == "stop" ? false : true;
    }
  } catch (std::exception &ex) {
    spdlog::error("Problem parsing output: {}", ex.what());
    spdlog::error("Output from Esatto:\n{}\n", resp);
    spdlog::error("halting all");
    send_command_to_focuser(cmd_abort_mot1_cmd());
    send_command_to_focuser(_rotator->cmd_abort_mot2_cmd());
  }
}

void esatto_focuser::update_properties_proc() {
  using namespace std::chrono_literals;
  while (_connected) {
    try {
      update_properties();
    } catch (alpaca_exception &ex) {
      spdlog::warn("problem during update_properties: {}", ex.what());
    }
    std::this_thread::sleep_for(1000ms);
  }
  spdlog::debug("update_properties_proc ended");
};

// std::string build_command(const std::string &cmd_type, const std::)

int esatto_focuser::set_connected(bool connected) {
  if (_connected && connected) {
    spdlog::warn(
        "set_connected called with connected:{0} but is already in that state");
    return 0;
  }

  if (connected) {
    try {

      char buf[512] = {0};

      std::map<std::string,
               std::map<std::string, std::map<std::string, primaluce_value_t>>>
          req;

      auto resp = send_command_to_focuser(get_all_system_data_cmd());
      // spdlog::debug("Focuser response: {}", resp);
      auto parsed_resp = nlohmann::json::parse(resp);
      if (parsed_resp["res"]["get"]["MODNAME"] != "ESATTO3")
        throw alpaca_exception(
            alpaca_exception::DRIVER_ERROR,
            fmt::format("Problem getting model name. Focuser returned {}",
                        resp));

      // Start update thread to values
      _focuser_update_thread =
          std::thread(std::bind(&esatto_focuser::update_properties_proc, this));
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
        _rotator->set_connected(false);
        _connected = false;
        _focuser_update_thread.join();
        // _serial_port.close();
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
};

std::string esatto_focuser::unique_id() { return _serial_device_path; };

int esatto_focuser::set_serial_device(const std::string &serial_device_path) {
  _serial_device_path = serial_device_path;
  return 0;
};

std::string esatto_focuser::get_serial_device_path() {
  return _serial_device_path;
};

uint32_t esatto_focuser::interface_version() { return 3; };

std::string esatto_focuser::driver_version() { return "v0.1"; };

std::vector<std::string> esatto_focuser::supported_actions() {
  return std::vector<std::string>();
};

std::string esatto_focuser::description() {
  return "PrimaLuceLabs Esatto Robotic Focuser";
};

std::string esatto_focuser::driverinfo() {
  return "AlpacaHub Esatto Robotic Focuser Driver";
};

std::string esatto_focuser::name() { return "Esatto Robotic Focuser"; };

bool esatto_focuser::absolute() {
  throw_if_not_connected();
  return true;
};

bool esatto_focuser::is_moving() {
  throw_if_not_connected();
  return _is_moving;
};

uint32_t esatto_focuser::max_increment() {
  throw_if_not_connected();
  return 731000;
};

uint32_t esatto_focuser::max_step() {
  throw_if_not_connected();
  return 731000;
};

uint32_t esatto_focuser::position() {
  throw_if_not_connected();
  return _position;
};

uint32_t esatto_focuser::step_size() {
  throw_if_not_connected();
  return _step_size;
};

bool esatto_focuser::temp_comp() {
  throw_if_not_connected();
  return _temp_comp_enabled;
};

int esatto_focuser::set_temp_comp(bool temp_comp_enabled) {
  throw_if_not_connected();
  _temp_comp_enabled = true;
  return 0;
};

bool esatto_focuser::temp_comp_available() {
  throw_if_not_connected();
  return true;
};

bool esatto_focuser::arco_present() { return _arco_present; };

double esatto_focuser::temperature() {
  throw_if_not_connected();
  return _temperature;
};

int esatto_focuser::halt() {
  auto resp = send_command_to_focuser(cmd_stop_mot1_cmd());
  // TODO: check for errors
  return 0;
};

int esatto_focuser::move(const int &pos) {
  throw_if_not_connected();
  spdlog::debug("moving focuser to: {}", pos);
  auto resp = send_command_to_focuser(cmd_move_abs_mot1_cmd(pos));
  spdlog::debug("resp: {}", resp);
  // TODO: check for errors
  return 0;
};

std::map<std::string, device_variant_t> esatto_focuser::details() {
  std::map<std::string, device_variant_t> detail_map;
  detail_map["Connected"] = _connected;
  detail_map["Serial Device"] = _serial_device_path;

  if (_connected) {
    detail_map["Temperature"] = _temperature;
    detail_map["Position"] = _position;
    detail_map["Moving"] = _is_moving;
    detail_map["Backlash"] = _backlash;
    detail_map["Moving"] = _is_moving;
  }

  return detail_map;
};

// This will be used for the arco unit as well
std::string esatto_focuser::send_command_to_focuser(const std::string &cmd,
                                                    bool read_response,
                                                    char stop_on_char) {

  try {
    // spdlog::trace("flushing serial");

    spdlog::trace("sending: {} to focuser", cmd);
    std::lock_guard lock(_focuser_mtx);
    char buf[8192] = {0};
    _serial_port.write_some(asio::buffer(cmd));
    std::string rsp;

    if (read_response) {
      _io_context.reset();
      alpaca_hub_serial::blocking_reader reader(cmd, _serial_port, 500,
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
        fmt::format("Problem sending command to focuser: {}", ex.what()));
  }
};

std::string get_common_cmd(const std::string &param_name) {
  primaluce_kv_node root;
  root.create_object("req")->create_object("get")->push_param(param_name, "");
  return root.to_json().dump();
}

std::string esatto_focuser::get_product_name_cmd() {
  return get_common_cmd("MODNAME");
};

std::string esatto_focuser::get_serial_number_cmd() {
  return get_common_cmd("SN");
};

std::string esatto_focuser::get_macaddr_cmd() {
  return get_common_cmd("MACADDR");
};

std::string esatto_focuser::get_external_temp_cmd() {
  return get_common_cmd("EXT_T");
};

std::string esatto_focuser::get_vin_12v_cmd() {
  return get_common_cmd("VIN_12V");
};

std::string esatto_focuser::get_wifi_ap_cmd() {
  return get_common_cmd("WIFIAP");
};

std::string esatto_focuser::get_sw_vers_cmd() {
  return get_common_cmd("SWVERS");
};

// preset_idx 1-12
std::string esatto_focuser::get_preset_cmd(const uint32_t &preset_idx) {
  return get_common_cmd(fmt::format("PRESET_{}", preset_idx));
};

std::string esatto_focuser::get_presets_cmd() {
  return get_common_cmd("PRESETS");
};

// preset_idx 1-12
std::string esatto_focuser::cmd_recall_preset_cmd(const uint32_t &preset_idx) {
  return "";
};

std::string esatto_focuser::get_all_system_data_cmd() {
  primaluce_kv_node root;
  root.create_object("req")->push_param("get", "");
  return root.to_json().dump();
};

// on, low, middle, off
std::string esatto_focuser::cmd_dimleds_cmd(const std::string &led_brightness) {
  if (led_brightness != "on" && led_brightness != "low" &&
      led_brightness != "middle" && led_brightness != "off")
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        "DIMLEDS must be set to one of off, on, low, middle");

  return get_common_cmd("DIMLEDS");
};

std::string esatto_focuser::cmd_reboot_cmd() {
  primaluce_kv_node root;
  root.create_object("req")->create_object("cmd")->push_param("REBOOT", "");
  return root.to_json().dump();
};

// ***************************************** //
// Commands only for esatto
// ***************************************** //
std::string esatto_focuser::get_vin_usb_cmd() {
  return get_common_cmd("VIN_USB");
};

// ***************************************** //
// Commands common to esatto and sestosenso2
// ***************************************** //
std::string esatto_focuser::get_mot1_cmd() { return get_common_cmd("SN"); };

std::string esatto_focuser::get_mot1_status_cmd() {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("get")
      ->create_object("MOT1")
      ->push_param("STATUS", "");
  return root.to_json().dump();
};
// Deprecated command...won't implement
// std::string cmd_goto_cmd(const int &) { return ""; };

// Not sure I want / need to implement these or not...
// std::string cmd_fast_inward_mot1_cmd() { return ""; };
// std::string cmd_fast_outward_mot1_cmd() { return ""; };
// std::string cmd_slow_inward_mot1_cmd() { return ""; };
// std::string cmd_slow_outward_mot1_cmd() { return ""; };

std::string cmd_common_mot1_cmd(const std::string &param_name) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT1")
      ->push_param(param_name, "");
  return root.to_json().dump();
}

// Without deceleration
std::string esatto_focuser::cmd_abort_mot1_cmd() {
  return cmd_common_mot1_cmd("MOT_ABORT");
};

// With previously set deceleration
std::string esatto_focuser::cmd_stop_mot1_cmd() {
  return cmd_common_mot1_cmd("MOT_STOP");
};

// Moves the focuser to a new position like for the GOTO command,
// but it write, in the serial bus, a feedback of the current
// position. This command print every 1 sec, the current position
// and inform you when it finished your request.
std::string
esatto_focuser::cmd_verbose_time_goto_mot1_cmd(const uint32_t &pos) {
  return cmd_common_mot1_cmd("VERBOSE_TIME_GOTO");
};

std::string get_common_mot1_cmd(const std::string &param_name) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT1")
      ->push_param(param_name, "");
  return root.to_json().dump();
}

// Motor position, allowing for any sync offset
// POSITION = POSITION_STEP + COMPENSATION_POS_STEP
// This parameter supports only the "get" operation
std::string esatto_focuser::get_position_mot1_cmd() {
  return get_common_mot1_cmd("POSITION");
};

// Absolute (mechanical) Motor position
std::string esatto_focuser::get_position_step_mot1_cmd() {
  return get_common_mot1_cmd("POSITION_STEP");
};

// Absolute (mechanical) Motor position
std::string esatto_focuser::get_abs_position_step_mot1_cmd() {
  return get_common_mot1_cmd("ABS_POS_STEP");
};

// Syncs the motor to the specified position without moving it.This
// command changes the COMPENSATION_POS_STEP's value
std::string
esatto_focuser::cmd_sync_position_mot1_cmd(const uint32_t &sync_pos) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT1")
      ->create_object("SYNC_POS")
      ->push_param("STEP", sync_pos);
  return root.to_json().dump();
};

// Move the motor to the specified absolute position This command is
// similar to the GOTO command, except it works with the absolute
// position
std::string esatto_focuser::cmd_move_abs_mot1_cmd(const uint32_t &pos) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT1")
      ->create_object("MOVE_ABS")
      ->push_param("STEP", pos);
  return root.to_json().dump();
};

// Same as MOVE_ABS command, but writes logs in the shell at every
// second
std::string esatto_focuser::cmd_verbose_move_abs_mot1_cmd(const uint32_t &pos) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT1")
      ->create_object("VERBOSE_MOVE_ABS")
      ->push_param("STEP", pos);
  return root.to_json().dump();
};
// Causes the motor to move Position relative to the
// current Position value
std::string esatto_focuser::cmd_move_mot1_cmd(const uint32_t &pos) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("cmd")
      ->create_object("MOT1")
      ->create_object("MOVE")
      ->push_param("STEP", pos);
  return root.to_json().dump();
};

std::string esatto_focuser::get_backlash_cmd() {
  return get_common_mot1_cmd("BKLASH");
};

// The Motor's backlash is corrected in the factory, with a specific
// calibration value.  The user could customize this motor's
// parameter using this command, increasing the calibration setup.
// The range of possible values is from 0 to 5000.  The default
// value is zero.
std::string esatto_focuser::set_backlash_cmd(const uint32_t &steps) {
  primaluce_kv_node root;
  root.create_object("req")
      ->create_object("set")
      ->create_object("MOT1")
      ->push_param("BKLASH", steps);
  return root.to_json().dump();
};

void esatto_focuser::throw_if_not_connected() {
  if (!_connected)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Focuser must be connected");
}
