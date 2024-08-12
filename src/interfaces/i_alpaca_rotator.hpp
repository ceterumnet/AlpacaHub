#ifndef I_ALPACA_ROTATOR_HPP
#define I_ALPACA_ROTATOR_HPP

#include "i_alpaca_device.hpp"

class i_alpaca_rotator : public i_alpaca_device {
public:
  virtual ~i_alpaca_rotator() {};
  virtual bool can_reverse() = 0;
  virtual bool is_moving() = 0;
  virtual double mechanical_position() = 0;
  virtual bool reverse() = 0;
  virtual int set_reverse() = 0;
  virtual double step_size() = 0;
  virtual double target_position() = 0;
  virtual int halt() = 0;
  virtual int move(const double &position) = 0;
  virtual int moveabsolute(const double &absolute_position) = 0;
  virtual int movemechanical(const double &mechanical_position) = 0;
  virtual int sync(const double &sync_position) = 0;
  virtual std::map<std::string, device_variant_t> details() = 0;
};

#endif
