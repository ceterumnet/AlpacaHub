#ifndef I_ALPACA_DEVICE_HPP
#define I_ALPACA_DEVICE_HPP

#include "alpaca_hub_common.hpp"
#include <string>
#include <vector>

// #include <spdlog/spdlog.h>
// #include <spdlog/sinks/stdout_color_sinks.h>

// Basic interface for Alpaca devices

class i_alpaca_device {
public:

  virtual bool connected() = 0;
  virtual int set_connected(bool) = 0;
  virtual std::string description() = 0;
  virtual std::string driverinfo() = 0;
  virtual std::string name() = 0;
  virtual uint32_t interface_version() = 0;
  virtual std::string driver_version() = 0;
  // virtual std::vector<std::string> supported_actions() = 0;
  // i_alpaca_device()
  virtual ~i_alpaca_device() { printf("i_alpaca_device destructor called\n"); };

  virtual std::vector<std::string> supported_actions() = 0;

  virtual std::string unique_id() = 0;
};

#endif
