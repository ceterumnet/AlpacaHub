#ifndef I_ALPACA_FILTERWHEEL_HPP
#define I_ALPACA_FILTERWHEEL_HPP

#include "i_alpaca_device.hpp"

class i_alpaca_filterwheel : public i_alpaca_device {
public:
  virtual int position() = 0;
  virtual std::vector<std::string> names() = 0;
  virtual int set_names(std::vector<std::string>) = 0;
  virtual int set_position(uint32_t) = 0;
  virtual std::vector<int> focus_offsets() = 0;
  virtual ~i_alpaca_filterwheel(){};
};

#endif
