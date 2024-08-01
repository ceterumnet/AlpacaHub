#ifndef I_ALPACA_SWITCH_HPP
#define I_ALPACA_SWITCH_HPP

#include "i_alpaca_device.hpp"

class i_alpaca_switch : public i_alpaca_device {
public:
  virtual ~i_alpaca_switch(){};
  virtual uint32_t max_switch() = 0;
  virtual bool can_write(const uint32_t &switch_idx) = 0;
  virtual bool get_switch(const uint32_t &switch_idx) = 0;
  virtual std::string get_switch_description(const uint32_t &switch_idx) = 0;
  virtual std::string get_switch_name(const uint32_t &switch_idx) = 0;
  virtual double get_switch_value(const uint32_t &switch_idx) = 0;
  virtual double min_switch_value(const uint32_t &switch_idx) = 0;
  virtual double max_switch_value(const uint32_t &switch_idx) = 0;
  virtual int set_switch(const uint32_t &switch_idx,
                         const bool &switch_state) = 0;
  virtual int set_switch_name(const uint32_t &switch_idx,
                              const std::string &switch_name) = 0;
  virtual int set_switch_value(const uint32_t &switch_idx,
                               const double &switch_value) = 0;
  virtual double switch_step(const uint32_t &switch_idx) = 0;

  virtual std::string send_command_to_switch(const std::string &, bool, char) = 0;
};

#endif
