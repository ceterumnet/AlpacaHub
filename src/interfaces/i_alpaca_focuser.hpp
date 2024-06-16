#ifndef I_ALPACA_FOCUSER_HPP
#define I_ALPACA_FOCUSER_HPP

#include "i_alpaca_device.hpp"

class i_alpaca_focuser : public i_alpaca_device {
public:
  virtual ~i_alpaca_focuser() {};
  virtual bool absolute() = 0;
  virtual bool is_moving() = 0;
  virtual uint32_t max_increment() = 0;
  virtual uint32_t max_step() = 0;
  virtual uint32_t position() = 0;
  virtual uint32_t step_size() = 0;
  virtual bool temp_comp() = 0;
  virtual int set_temp_comp(bool) = 0;
  virtual bool temp_comp_available() = 0;
  virtual double temperature() = 0;
  virtual int halt() = 0;
  virtual int move(const int &) = 0;
};

#endif
