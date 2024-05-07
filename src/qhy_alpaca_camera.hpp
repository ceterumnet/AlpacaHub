#ifndef QHY_ALPACA_CAMERA_HPP
#define QHY_ALPACA_CAMERA_HPP

#include "alpaca_exception.hpp"
#include "fmt/format.h"
#include "i_alpaca_camera.hpp"
#include "qhy_alpaca_filterwheel.hpp"
#include "spdlog/spdlog.h"
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <qhyccd.h>
#include <qhyccdcamdef.h>
#include <qhyccderr.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <thread>
#include <vector>

class qhy_alpaca_camera : public i_alpaca_camera {
public:
  static int InitializeQHYSDK();
  static int ReleaseQHYSDK();
  static u_int32_t camera_count();
  static std::vector<std::string> get_connected_cameras();
  static void on_camera_unplugged(char *);
  static void on_camera_plugged(char *);

  bool connected();

  std::string unique_id();
  int set_connected(bool);
  qhy_alpaca_camera(std::string &);
  ~qhy_alpaca_camera();

  uint32_t interface_version();
  std::string driver_version();
  short bin_x();
  short bin_y();
  int set_bin_x(short);
  int set_bin_y(short);
  camera_state_enum camera_state();
  long camera_x_size();
  long camera_y_size();

  bool can_abort_exposure();
  bool can_asymmetric_bin();
  bool can_get_cooler_power();
  bool can_pulse_guide();
  bool can_set_ccd_temperature();
  bool can_stop_exposure();
  bool has_shutter();

  double ccd_temperature();

  bool cooler_on();
  int set_cooler_on(bool);
  int set_cooler_power(double);
  double cooler_power();

  double electrons_per_adu();
  double full_well_capacity();
  double heat_sink_temperature();

  int image_array(std::vector<uint8_t> &theImage);

  // template <typename T>
  // std::vector<std::vector<T>> image_2d();
  std::vector<std::vector<uint32_t>> image_2d();
  std::vector<std::vector<uint32_t>> image_2d_8bpp();
  std::vector<std::vector<uint32_t>> image_2d_16bpp();

  // template <> std::vector<std::vector<uint16_t>> image_2d();

  // virtual Array< Array<long> > ImageArrayVariant() = 0;

  bool image_ready();
  bool is_pulse_guiding();
  std::string last_error();
  double last_exposure_duration();

  // Reports the actual exposure start in the FITS-standard
  // CCYY-MM-DDThh:mm:ss[.sss...] format.
  std::string last_exposure_start_time();
  long max_adu();
  short max_bin_x();
  short max_bin_y();
  long num_x();
  long num_y();
  int set_num_x(long);
  int set_num_y(long);
  double pixel_size_x();
  double pixel_size_y();
  int set_ccd_temperature(double);
  double get_set_ccd_temperature();
  long start_x();
  int set_start_x(long);
  long start_y();
  int set_start_y(long);

  int abort_exposure();
  int pulse_guide(guide_direction, long duration);
  int start_exposure(double, bool is_light = true);
  int stop_exposure();

  uint32_t gain();
  uint32_t gain_max();
  uint32_t gain_min();
  std::vector<std::string> gains();
  int set_gain(uint32_t);

  // 0 = Monochrome,
  // 1 = Colour not requiring Bayer decoding
  // 2 = RGGB Bayer encoding
  // 3 = CMYG Bayer encoding
  // 4 = CMYG2 Bayer encoding
  // 5 = LRGB TRUESENSE Bayer encoding.
  int sensor_type();

  std::string sensor_name();

  std::string get_camera_model_name();
  std::string description();
  std::string driverinfo();
  std::string name();

  double exposure_max();
  double exposure_min();
  double exposure_resolution();
  double subexposure_duration();
  int set_subexposure_duration(double);
  bool can_fast_readout();
  bool fast_readout();
  int set_fast_readout(bool);
  int readout_mode();
  int set_readout_mode(int);
  std::vector<std::string> readout_modes();

  int set_offset(int);
  int offset();
  int offset_max();
  int offset_min();
  std::vector<std::string> offsets();

  uint8_t bpp();

  int percent_complete();
  int bayer_offset_x();
  int bayer_offset_y();

  std::vector<std::string> supported_actions();

  friend class qhy_alpaca_filterwheel;

  bool has_filter_wheel();
  std::shared_ptr<qhy_alpaca_filterwheel> filter_wheel();

private:
  void initialize_camera_by_camera_id(std::string &camera_id);
  int get_is_qhy_control_available(CONTROL_ID);
  std::string _camera_id;
  std::string _unique_id;
  bool _connected;
  static u_int32_t _num_of_connected_cameras;
  qhyccd_handle *_cam_handle;
  uint32_t _num_modes;
  camera_state_enum _camera_state;
  std::mutex _cam_mutex;
  // This allows us to get the camera id from the camera name conveniently
  static std::map<std::string, int> _camera_map;
  short _bin_x;
  short _bin_y;
  double _chip_w;
  double _chip_h;
  uint32_t _image_w;
  uint32_t _image_h;

  double _pixel_w;
  double _pixel_h;
  uint32_t _bpp;
  double _gain;
  bool _has_shutter;
  uint32_t _num_x;
  uint32_t _num_y;
  uint32_t _effective_num_x;
  uint32_t _effective_num_y;

  uint32_t _max_num_x;
  uint32_t _max_num_y;

  uint32_t _start_x;
  uint32_t _start_y;
  uint32_t _effective_start_x;
  uint32_t _effective_start_y;

  // Leaving this so that later I can add options around this
  bool _include_overscan;

  double _last_exposure_duration;
  std::vector<uint8_t> _img_data;
  std::thread _img_read_thread;
  std::thread _cooler_thread;
  bool _run_cooler_thread;
  void cooler_proc();
  bool should_run_cooler_proc();
  void ensure_temp_is_set();
  void read_image_from_camera();
  int start_exposure_proc();
  asio::io_context _io;
  void set_reading_state();
  std::string _qhy_model_name;
  double _current_set_temp;

  std::vector<std::string> _read_mode_names;
  int set_resolution(const uint32_t start_x, const uint32_t start_y,
                     const uint32_t read_width, const uint32_t read_height);
  std::string _last_exposure_start_time_fits;

  std::string _sensor_name;

  double _exposure_max;
  double _exposure_min;
  double _exposure_step_size;
  double _subexposure_duration;

  int _readout_mode;
  bool _fast_readout;

  double _offset;
  double _offset_max;
  double _offset_min;
  double _offset_step_size;
  std::vector<std::string> _offsets;

  double _gain_max;
  double _gain_min;
  double _gain_step_size;
  std::vector<std::string> _gains;

  bool _can_control_ccd_temp;
  bool _can_control_cooler_power;
  short _max_bin;

  std::string _last_cam_error;
  int _percent_complete;

  int _bayer_offset_x;
  int _bayer_offset_y;

  double _set_cooler_power;

  bool _has_filter_wheel;
  std::shared_ptr<qhy_alpaca_filterwheel> _filter_wheel;

};

#endif
