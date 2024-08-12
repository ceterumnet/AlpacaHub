#ifndef PRIMALUCE_FOCUSER_ROTATOR_HPP
#define PRIMALUCE_FOCUSER_ROTATOR_HPP

#include "asio/io_context.hpp"
#include "common/alpaca_hub_serial.hpp"
#include "interfaces/i_alpaca_focuser.hpp"
#include "interfaces/i_alpaca_rotator.hpp"
#include <memory>

// Basic type
using primaluce_value_t = std::variant<double, int, uint32_t, std::string>;

// Basic single level deep object
using primaluce_map_of_value_t = std::map<std::string, primaluce_value_t>;

using primaluce_variant_t =
    std::variant<primaluce_value_t, primaluce_map_of_value_t>;

using primaluce_dict_variant_t = std::map<std::string, primaluce_variant_t>;

using primaluce_dict_t =
    std::map<std::string, std::map<std::string, primaluce_variant_t>>;

struct primaluce_node {
  std::string key;
  primaluce_value_t value;
  primaluce_node(const std::string k, const primaluce_value_t v)
      : key(k), value(v){};

  // std::visit([](const auto &elem) { std::cout << elem << '\n'; }, value);
};

template <> struct fmt::formatter<primaluce_value_t> : fmt::formatter<std::string> {
  auto format(primaluce_value_t my, format_context &ctx) const -> decltype(ctx.out()) {
    if (std::holds_alternative<double>(my)) {
      return format_to(ctx.out(), "{}", std::get<double>(my));
    } else if (std::holds_alternative<int>(my)) {
      return format_to(ctx.out(), "{}", std::get<int>(my));
    } else if (std::holds_alternative<uint32_t>(my)) {
      return format_to(ctx.out(), "{}", std::get<uint32_t>(my));
    } else {
      return format_to(ctx.out(), "{}", std::get<std::string>(my));
    }
  }
};

// template <typename T>
struct primaluce_kv_node : public std::enable_shared_from_this<primaluce_kv_node> {
  std::string key;
  std::vector<std::shared_ptr<primaluce_node>> nodes;
  std::vector<std::shared_ptr<primaluce_kv_node>> edges;

  std::shared_ptr<primaluce_kv_node> this_root() {
    return shared_from_this();
  }

  std::shared_ptr<primaluce_kv_node> push_param(std::string k,
                                              primaluce_value_t v) {
    auto node = std::make_shared<primaluce_node>(k, v);
    nodes.push_back(node);
    return this_root();
  }

  std::shared_ptr<primaluce_kv_node> create_object(std::string k) {
    auto edge = std::make_shared<primaluce_kv_node>();
    edge->key = k;
    edges.push_back(edge);
    return edge;
  }

  std::string dump() {
    return to_json().dump();
  }

  nlohmann::json to_json(nlohmann::json j = nlohmann::json()) {
    for (auto iter : nodes) {
      j[iter->key] = iter->value;
    }

    for (auto iter : edges) {
      j[iter->key] = iter->to_json();
    }
    return j;
  }
};

// using primaluce_kv_t = primaluce_kv_node<primaluce_value_t>;
// using primaluce_kv_dict_t = primaluce_kv_node<primaluce_kv_t>;

class esatto_focuser;

class arco_rotator : public i_alpaca_rotator {
public:
  arco_rotator(esatto_focuser &focuser);
  ~arco_rotator();

  std::map<std::string, device_variant_t> details();
  bool connected();
  int set_connected(bool);

  std::string unique_id();

  uint32_t interface_version();
  std::string driver_version();
  std::vector<std::string> supported_actions();
  std::string description();
  std::string driverinfo();
  std::string name();

  bool can_reverse();
  bool is_moving();
  double mechanical_position();
  bool reverse();
  int set_reverse();
  double step_size();
  double target_position();
  int halt();
  int move(const double &position);
  int moveabsolute(const double &absolute_position);
  int movemechanical(const double &mechanical_position);
  int sync(const double &sync_position);

private:
  esatto_focuser &_focuser;
  bool _is_moving;
  bool _connected;
  bool _reversed;
  double _position;
  double _mechanical_position;
  double _target_position;
};

class esatto_focuser : public i_alpaca_focuser {
public:
  // This is the one that pops up on my system...
  // usb-Silicon_Labs_CP2102N_USB_to_UART_Bridge_Controller_b2f14184e185eb11ad7b8b1ab7d59897-if00-port0
  static std::vector<std::string> serial_devices();
  friend class arco_rotator;

  esatto_focuser();
  ~esatto_focuser();

  std::map<std::string, device_variant_t> details();
  bool connected();
  int set_connected(bool);

  std::string unique_id();
  int set_serial_device(const std::string &serial_device_path);
  std::string get_serial_device_path();

  uint32_t interface_version();
  std::string driver_version();
  std::vector<std::string> supported_actions();
  std::string description();
  std::string driverinfo();
  std::string name();

  bool absolute();
  bool is_moving();
  uint32_t max_increment();
  uint32_t max_step();
  uint32_t position();
  uint32_t step_size();
  bool temp_comp();
  int set_temp_comp(bool);
  bool temp_comp_available();
  double temperature();
  int halt();
  int move(const int &pos);

  // This will be used for the arco unit as well
  std::string send_command_to_focuser(const std::string &cmd,
                                      bool read_response = true,
                                      char stop_on_char = '\n');

private:
  void throw_if_not_connected();
  void update_properties();
  void update_properties_proc();
  std::thread _focuser_update_thread;
  std::string _serial_device_path;
  bool _connected;
  bool _is_moving;
  asio::io_context _io_context;
  asio::serial_port _serial_port;
  std::mutex _focuser_mtx;
  uint32_t _position;
  int _backlash;
  double _temperature;
  std::unique_ptr<arco_rotator> _rotator;
  uint32_t _step_size;
  bool _temp_comp_enabled;

  // ***************************************** //
  // Commands common to all
  // ***************************************** //
  std::string get_product_name_cmd();
  std::string get_serial_number_cmd();
  std::string get_macaddr_cmd();
  std::string get_external_temp_cmd();
  std::string get_vin_12v_cmd();
  std::string get_wifi_ap_cmd();
  std::string get_sw_vers_cmd();
  // preset_idx 1-12
  std::string get_preset_cmd(const uint32_t &preset_idx);
  std::string get_presets_cmd();
  // preset_idx 1-12
  std::string cmd_recall_preset_cmd(const uint32_t &preset_idx);
  std::string get_all_system_data_cmd();
  // on, low, middle, off
  std::string cmd_dimleds_cmd(const std::string &led_brightness);
  std::string cmd_reboot_cmd();

  // ***************************************** //
  // Commands only for esatto
  // ***************************************** //
  std::string get_vin_usb_cmd();

  // ***************************************** //
  // Commands common to esatto and sestosenso2
  // ***************************************** //
  std::string cmd_get_mot1_cmd();
  std::string cmd_get_mot1_status_cmd();
  // Deprecated command...won't implement
  // std::string cmd_goto_cmd(const int &);

  // Not sure I want / need to implement these or not...
  // std::string cmd_fast_inward_mot1_cmd();
  // std::string cmd_fast_outward_mot1_cmd();
  // std::string cmd_slow_inward_mot1_cmd();
  // std::string cmd_slow_outward_mot1_cmd();

  // Without deceleration
  std::string cmd_abort_mot1_cmd();

  // With previously set deceleration
  std::string cmd_stop_mot1_cmd();

  // Moves the focuser to a new position like for the GOTO command,
  // but it write, in the serial bus, a feedback of the current
  // position. This command print every 1 sec, the current position
  // and inform you when it finished your request.
  std::string cmd_verbose_time_goto_mot1_cmd(const uint32_t &);

  // Motor position, allowing for any sync offset
  // POSITION = POSITION_STEP + COMPENSATION_POS_STEP
  // This parameter supports only the "get" operation
  std::string get_position_mot1_cmd();

  // Absolute (mechanical) Motor position
  std::string get_position_step_mot1_cmd();

  // Absolute (mechanical) Motor position
  std::string get_abs_position_step_mot1_cmd();

  // Syncs the motor to the specified position without moving it.This
  // command changes the COMPENSATION_POS_STEP's value
  std::string cmd_sync_position_mot1_cmd(const uint32_t &);

  // Move the motor to the specified absolute position This command is
  // similar to the GOTO command, except it works with the absolute
  // position
  std::string cmd_move_abs_mot1_cmd(const uint32_t &);

  // Same as MOVE_ABS command, but writes logs in the shell at every
  // second
  std::string verbose_move_abs_mot1_cmd(const uint32_t &);
  // Causes the motor to move Position relative to the
  // current Position value
  std::string cmd_move_mot1_cmd(const uint32_t &);

  std::string get_backlash_cmd();

  // The Motor's backlash is corrected in the factory, with a specific
  // calibration value.  The user could customize this motor's
  // parameter using this command, increasing the calibration setup.
  // The range of possible values is from 0 to 5000.  The default
  // value is zero.
  std::string set_backlash_cmd(const uint32_t &);
};

#endif
