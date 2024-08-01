#include "asio/io_context.hpp"
#include "common/alpaca_hub_serial.hpp"
#include "interfaces/i_alpaca_switch.hpp"

enum ppba_switches {
  // case 0 : return "Input Voltage";
  // case 1 : return "Current in Amps";
  // case 2 : return "Quad 12V Current";
  // case 3 : return "Power in Watts";
  // case 4 : return "Temperature in Celsius";
  // case 5 : return "Humidity %";
  // case 6 : return "Dew Point in Celsius";
  // case 7 : return "Quad 12V Output On/Off";
  // case 8 : return "Adjustable Power Output On/Off";
  // case 9 : return "Adjustable Power Set Voltage";
  // case 10 : return "Dew Heater A PWM Set Point";
  // case 11 : return "Dew Heater B PWM Set Point";
  // case 12 : return "Dew Heater A Power";
  // case 13 : return "Dew Heater B Power";
  // case 14 : return "AutoDew";
  // case 15 : return "Power Warning";
  // case 16 : return "Uptime in ms";
  // case 17 : return "Autodew Aggressiveness";
  // case 18 : return "USB2 Ports On/Off";
  INPUT_VOLTAGE = 0,
  CURRENT,
  CURRENT_12V,
  POWER,
  TEMP,
  HUMIDITY,
  DEWPOINT,
  QUAD12V_ON_OFF,
  ADJPOW_ON_OFF,
  ADJVOLTAGE,
  DEWA_PWM,
  DEWB_PWM,
  DEWA_CURRENT,
  DEWB_CURRENT,
  AUTODEW_ON_OFF,
  POWERWARN,
  UPTIME_MS,
  DEW_AGGRESSIVENESS,
  USB2_ON_OFF
};

class pegasus_alpaca_ppba : public i_alpaca_switch {
public:
  static std::vector<std::string> serial_devices();
  pegasus_alpaca_ppba();
  ~pegasus_alpaca_ppba();
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

  uint32_t max_switch();

  bool can_write(const uint32_t &switch_idx);
  bool get_switch(const uint32_t &switch_idx);
  std::string get_switch_description(const uint32_t &switch_idx);
  std::string get_switch_name(const uint32_t &switch_idx);
  double get_switch_value(const uint32_t &switch_idx);
  double min_switch_value(const uint32_t &switch_idx);
  double max_switch_value(const uint32_t &switch_idx);
  int set_switch(const uint32_t &switch_idx, const bool &switch_state);
  int set_switch_name(const uint32_t &switch_idx,
                      const std::string &switch_name);
  int set_switch_value(const uint32_t &switch_idx, const double &switch_value);
  double switch_step(const uint32_t &switch_idx);

private:
  void throw_if_not_connected();
  std::string send_command_to_switch(const std::string &cmd,
                                     bool read_response = true,
                                     char stop_on_char = '\n');
  void update_properties();
  void update_properties_proc();
  std::thread _ppba_update_thread;
  std::string _serial_device_path;
  bool _connected;
  asio::io_context _io_context;
  asio::serial_port _serial_port;
  std::mutex _ppba_mtx;

  double _voltage;
  double _power;
  double _current_of_12v_outputs;
  double _total_current;
  double _temp;
  uint32_t _humidity;
  bool _quadport_on;
  uint32_t _uptime_in_mins;

  double _dew_point;

  uint32_t _dew_a_pwm;
  double _current_of_dewA;
  double _current_of_dewB;

  // uint32_t _dew_a_pwm_max;
  // uint32_t _dew_a_pwm_min;
  uint32_t _dew_b_pwm;
  // uint32_t _dew_b_pwm_max;
  // uint32_t _dew_b_pwm_min;

  // uint32_t _dew_a_power;
  // uint32_t _dew_b_power;
  bool _usb2_on_off;
  bool _autodew;
  bool _power_warning;
  bool _adj_power_on;
  uint32_t _adj_power_voltage;

  // uint32_t _adj_power_voltage_min;
  // uint32_t _adj_power_voltage_max;
  uint32_t _dew_aggressiveness;
};
