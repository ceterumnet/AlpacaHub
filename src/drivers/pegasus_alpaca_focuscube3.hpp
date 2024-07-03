#ifndef PEGASUS_ALPACA_FOCUSER_HPP
#define PEGASUS_ALPACA_FOCUSER_HPP

#include "interfaces/i_alpaca_focuser.hpp"
#include "asio/io_context.hpp"
#include "common/alpaca_hub_serial.hpp"

class pegasus_alpaca_focuscube3 : public i_alpaca_focuser {
public:
  static std::vector<std::string> serial_devices();

  pegasus_alpaca_focuscube3();
  ~pegasus_alpaca_focuscube3();
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
  bool _moving;
  asio::io_context _io_context;
  asio::serial_port _serial_port;
  std::mutex _focuser_mtx;
  uint32_t _position;
  int _backlash;
  double _temperature;
};

#endif
