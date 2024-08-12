#include "primaluce_focuser_rotator.hpp"
#include "common/alpaca_exception.hpp"
#include "interfaces/i_alpaca_device.hpp"
#include <memory>

arco_rotator::arco_rotator(esatto_focuser &focuser)
    : _focuser(focuser), _is_moving(false) {}

arco_rotator::~arco_rotator() {}

std::map<std::string, device_variant_t> arco_rotator::details() {
  std::map<std::string, device_variant_t> detail_map;
  detail_map["Connected"] = _connected;

  if (_connected) {
    detail_map["Position"] = _position;
    detail_map["Mechanical Position"] = _mechanical_position;
    detail_map["Target Position"] = _target_position;
    detail_map["Moving"] = _is_moving;
    detail_map["Reversed"] = _reversed;
  }

  return detail_map;
};

bool arco_rotator::connected() { return _connected; };

std::vector<std::string> arco_rotator::supported_actions() {
  return std::vector<std::string>();
};

std::string arco_rotator::unique_id() { return "unique_id_for_rotator123131"; };

int arco_rotator::set_connected(bool connected) { return 0; }

std::string arco_rotator::description() { return "ARCO Robotic Rotator"; }

std::string arco_rotator::driverinfo() {
  return "AlpacaHub Driver for ARCO Robotic Rotator";
}

std::string arco_rotator::name() { return "ARCO Rotator"; }

uint32_t arco_rotator::interface_version() { return 3; }

std::string arco_rotator::driver_version() { return "v0.1"; }

bool arco_rotator::can_reverse() { return true; };

bool arco_rotator::is_moving() { return _is_moving; };

double arco_rotator::mechanical_position() { return 0; };
bool arco_rotator::reverse() { return false; };
int arco_rotator::set_reverse() { return 0; };

double arco_rotator::step_size() { return 0; };

double arco_rotator::target_position() { return 0; };

int arco_rotator::halt() { return 0; };

int arco_rotator::move(const double &position) { return 0; };

int arco_rotator::moveabsolute(const double &absolute_position) { return 0; };

int arco_rotator::movemechanical(const double &mechanical_position) {
  return 0;
};

int arco_rotator::sync(const double &sync_position) { return 0; };

std::vector<std::string> esatto_focuser::serial_devices() {
  std::vector<std::string> device_paths;
  return device_paths;
};

esatto_focuser::esatto_focuser()
    : _connected(false), _is_moving(false), _position(0), _temperature(0),
      _backlash(0), _serial_port(_io_context){};

esatto_focuser::~esatto_focuser() {
  if (_connected) {
    _connected = false;
    _focuser_update_thread.join();
    _serial_port.close();
  }
};

bool esatto_focuser::connected() { return _connected; };

void esatto_focuser::update_properties(){

};

void esatto_focuser::update_properties_proc(){

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

      // primaluce_map_of_value_t m1{{"key", "value"}};
      // // primaluce_map_of_maps_value_t m2{{"key", m1}};
      // primaluce_dict_variant_t d1{{"key", "value"}};
      // primaluce_dict_variant_t d2{{"key", m1}};

      // using pdv = primaluce_dict_variant_t;
      // // using pmv = primaluce_map_of_maps_value_t;
      // using pd  = primaluce_dict_t;
      // using pmv = primaluce_map_of_value_t;
      // pmv x{{"foo", "bar"}};

      // primaluce_kv_node<primaluce_kv_node> root{"key", primaluce_kv_node}

      std::map<std::string,
               std::map<std::string, std::map<std::string, primaluce_value_t>>>
          req;

      req["req"]["get"]["MODNAME"] = "";
      // _serial_port.write_some(
      //     asio::buffer(R"({"req":{"get":{"MODNAME":""}}}")"));

      _serial_port.write_some(asio::buffer(nlohmann::json(req).dump()));
      _serial_port.read_some(asio::buffer(buf));

      spdlog::debug("Focuser response: {}", buf);
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

bool esatto_focuser::absolute() { return true; };

bool esatto_focuser::is_moving() { return _is_moving; };

uint32_t esatto_focuser::max_increment() { return 0; };

uint32_t esatto_focuser::max_step() { return 0; };

uint32_t esatto_focuser::position() { return _position; };

uint32_t esatto_focuser::step_size() { return _step_size; };

bool esatto_focuser::temp_comp() { return _temp_comp_enabled; };

int esatto_focuser::set_temp_comp(bool temp_comp_enabled) {
  _temp_comp_enabled = true;
  return 0;
};

bool esatto_focuser::temp_comp_available() { return true; };

double esatto_focuser::temperature() { return _temperature; };

int esatto_focuser::halt() { return 0; };

int esatto_focuser::move(const int &pos) { return 0; };

std::map<std::string, device_variant_t> esatto_focuser::details() {
  std::map<std::string, device_variant_t> detail_map;
  detail_map["Connected"] = _connected;
  detail_map["Serial Device"] = _serial_device_path;

  if (_connected) {
    detail_map["Temperature"] = _temperature;
    detail_map["Position"] = _position;
    detail_map["Moving"] = _is_moving;
    detail_map["Backlash"] = _backlash;
  }

  return detail_map;
};

// This will be used for the arco unit as well
std::string send_command_to_focuser(const std::string &cmd,
                                    bool read_response = true,
                                    char stop_on_char = '\n') {
  return "";
};
