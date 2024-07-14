#ifndef QHY_ALPACA_FILTERWHEEL_STANDALONE_HPP
#define QHY_ALPACA_FILTERWHEEL_STANDALONE_HPP

#include "asio/io_context.hpp"
#include "common/alpaca_exception.hpp"
#include "common/alpaca_hub_serial.hpp"
#include "interfaces/i_alpaca_filterwheel.hpp"
#include <vector>

class qhy_alpaca_filterwheel_standalone : public i_alpaca_filterwheel {
public:
  static std::vector<std::string> get_connected_filterwheels();
  std::map<std::string, device_variant_t> details();
  bool connected();
  int set_connected(bool);
  std::string description();
  std::string driverinfo();
  std::string name();
  uint32_t interface_version();
  std::string driver_version();
  std::vector<std::string> supported_actions();
  std::string unique_id();

  // Pass in the path to the serial device
  qhy_alpaca_filterwheel_standalone(const std::string &);
  ~qhy_alpaca_filterwheel_standalone();

  int position();
  std::vector<std::string> names();
  int set_names(std::vector<std::string>);
  int set_position(uint32_t);
  std::vector<int> focus_offsets();

private:
  void initialize();
  void update_properties_proc();
  std::string send_command_to_filterwheel(
    const std::string &cmd, int n_chars_to_read);
  bool _connected;
  std::string _serial_device_path;
  std::string _name;
  std::string _description;
  std::string _driver_info;
  std::string _driver_version;
  std::string _firmware_version;
  std::string _unique_id;
  std::vector<int> _focus_offsets;
  std::vector<std::string> _names;
  asio::io_context _io_context;
  asio::serial_port _serial_port;
  std::mutex _filterwheel_mtx;
  int _position;
  bool _busy;
  std::thread _filterwheel_update_thread;
};

#endif
