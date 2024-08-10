#ifndef I_ALPACA_CAMERA_HPP
#define I_ALPACA_CAMERA_HPP

#include "i_alpaca_device.hpp"
#include <cstdint>

class i_alpaca_camera : public i_alpaca_device {


public:
  enum camera_type { TYPE_CCD, TYPE_CMOS, TYPE_DSLR };
  enum camera_state_enum {
    CAMERA_IDLE,
    CAMERA_WAITING,
    CAMERA_EXPOSING,
    CAMERA_READING,
    CAMERA_DOWNLOAD,
    CAMERA_ERROR
  };

  enum guide_direction { GUIDE_NORTH, GUIDE_SOUTH, GUIDE_EAST, GUIDE_WEST };

  virtual ~i_alpaca_camera(){};

  virtual short bin_x() = 0;
  virtual short bin_y() = 0;
  virtual int set_bin_x(short) = 0;
  virtual int set_bin_y(short) = 0;
  virtual camera_state_enum camera_state() = 0;
  virtual long camera_x_size() = 0;
  virtual long camera_y_size() = 0;
  virtual bool can_abort_exposure() = 0;
  virtual bool can_asymmetric_bin() = 0;
  virtual bool can_get_cooler_power() = 0;
  virtual bool can_pulse_guide() = 0;
  virtual bool can_set_ccd_temperature() = 0;
  virtual bool can_stop_exposure() = 0;
  virtual double ccd_temperature() = 0;
  virtual bool cooler_on() = 0;
  virtual int set_cooler_on(bool) = 0;
  virtual int set_cooler_power(double) = 0;
  virtual double cooler_power() = 0;

  virtual double electrons_per_adu() = 0;
  virtual double full_well_capacity() = 0;
  virtual bool has_shutter() = 0;
  virtual double heat_sink_temperature() = 0;

  virtual int image_array(std::vector<uint8_t> &theImage) = 0;
  // template <typename T> std::vector<std::vector<T>> image_2d();
  // template <> std::vector<std::vector<uint8_t>> image_2d();
  virtual std::vector<std::vector<uint32_t>> image_2d() = 0;
  virtual bool image_ready() = 0;
  virtual bool is_pulse_guiding() = 0;
  virtual std::string last_error() = 0;
  virtual double last_exposure_duration() = 0;
  // Reports the actual exposure start in the FITS-standard
  // CCYY-MM-DDThh:mm:ss[.sss...] format.
  virtual std::string last_exposure_start_time() = 0;
  virtual long max_adu() = 0;
  virtual short max_bin_x() = 0;
  virtual short max_bin_y() = 0;
  virtual long num_x() = 0;
  virtual long num_y() = 0;
  virtual int set_num_x(long) = 0;
  virtual int set_num_y(long) = 0;
  virtual double pixel_size_x() = 0;
  virtual double pixel_size_y() = 0;
  virtual int set_ccd_temperature(double) = 0;
  virtual double get_set_ccd_temperature() = 0;
  virtual long start_x() = 0;
  virtual int set_start_x(long) = 0;
  virtual long start_y() = 0;
  virtual int set_start_y(long) = 0;

  virtual int abort_exposure() = 0;
  virtual int pulse_guide(guide_direction, long duration) = 0;
  virtual int start_exposure(double, bool light_frame = true) = 0;
  virtual int stop_exposure() = 0;

  virtual double exposure_max() = 0;
  virtual double exposure_min() = 0;
  virtual double exposure_resolution() = 0;
  virtual double subexposure_duration() = 0;
  virtual int set_subexposure_duration(double) = 0;

  virtual bool can_fast_readout() = 0;
  virtual bool fast_readout() = 0;
  virtual int readout_mode() = 0;
  virtual int set_fast_readout(bool) = 0;
  virtual int set_readout_mode(int) = 0;
  virtual std::vector<std::string> readout_modes() = 0;

  virtual uint32_t gain() = 0;
  virtual uint32_t gain_max() = 0;
  virtual uint32_t gain_min() = 0;
  virtual std::vector<std::string> gains() = 0;
  virtual int set_gain(uint32_t) = 0;
  // 0 = Monochrome,
  // 1 = Colour not requiring Bayer decoding
  // 2 = RGGB Bayer encoding
  // 3 = CMYG Bayer encoding
  // 4 = CMYG2 Bayer encoding
  // 5 = LRGB TRUESENSE Bayer encoding.
  virtual int sensor_type() = 0;
  virtual std::string sensor_name() = 0;

  virtual int set_offset(int) = 0;
  virtual int offset() = 0;
  virtual int offset_max() = 0;
  virtual int offset_min() = 0;
  virtual std::vector<std::string> offsets() = 0;
  virtual std::string get_camera_model_name() = 0;
  virtual uint8_t bpp() = 0;

  virtual int percent_complete() = 0;
  virtual int bayer_offset_x() = 0;
  virtual int bayer_offset_y() = 0;

  // To support generic actions supported by driver implementations:
  // I probably need to move this to i_alpaca_device...but I'm not
  // sure this is the way I want to implement it
  virtual std::string
  invoke_action(const std::string &action_name,
                const std::map<std::string, std::string> &action_params) = 0;
};

#endif
