#ifndef QHY_ALPACA_FILTERWHEEL_HPP
#define QHY_ALPACA_FILTERWHEEL_HPP

#include "common/alpaca_exception.hpp"
#include "interfaces/i_alpaca_filterwheel.hpp"

// #include "qhy_alpaca_camera.hpp"
#include <memory>
#include <qhyccd.h>

class qhy_alpaca_camera;

class qhy_alpaca_filterwheel : public i_alpaca_filterwheel {
public:
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

  qhy_alpaca_filterwheel(qhy_alpaca_camera &);
  ~qhy_alpaca_filterwheel();

  int position();
  std::vector<std::string> names();
  int set_names(std::vector<std::string>);
  int set_position(uint32_t);
  std::vector<int> focus_offsets();


private:
  void initialize();
  bool _connected;
  std::string _name;
  std::string _description;
  std::string _driver_info;
  std::string _driver_version;
  std::string _unique_id;
  std::vector<int> _focus_offsets;
  std::vector<std::string> _names;
  qhy_alpaca_camera  &_camera;
};

#endif
