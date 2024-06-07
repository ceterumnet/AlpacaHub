#ifndef ZWO_AM5_TELESCOPE_HPP
#define ZWO_AM5_TELESCOPE_HPP

#include "asio/io_context.hpp"
#include "asio/io_service.hpp"
#include "asio/serial_port.hpp"
#include "common/alpaca_exception.hpp"
#include "common/alpaca_hub_serial.hpp"
#include "date/date.h"
#include "date/tz.h"
#include "fmt/chrono.h"
#include "interfaces/i_alpaca_telescope.hpp"
#include "spdlog/fmt/bundled/format.h"
#include "spdlog/spdlog.h"
#include <asio/steady_timer.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <initializer_list>
#include <memory>
#include <regex>
#include <stdexcept>
#include <thread>
#include <vector>

class zwo_am5_telescope : public i_alpaca_telescope {
public:
  static std::vector<std::string> serial_devices();
  bool connected();
  int set_connected(bool);
  zwo_am5_telescope();
  ~zwo_am5_telescope();
  uint32_t interface_version();
  std::string driver_version();
  std::vector<std::string> supported_actions();
  std::string description();
  std::string driverinfo();
  std::string name();
  std::string unique_id();

  alignment_mode_enum alignment_mode();
  double altitude();
  double aperture_diameter();
  double aperture_area();
  int set_aperture_diameter(const double &);
  bool at_home();
  bool at_park();
  double azimuth();
  bool can_find_home();
  bool can_park();
  bool can_pulse_guide();
  bool can_set_declination_rate();
  bool can_set_guide_rates();
  bool can_set_park();
  bool can_set_pier_side();
  bool can_set_right_ascension_rate();
  bool can_set_tracking();
  bool can_slew();
  bool can_slew_async();
  bool can_slew_alt_az();
  bool can_slew_alt_az_async();
  bool can_sync();
  bool can_sync_alt_az();
  bool can_unpark();
  double declination();
  double declination_rate();
  int set_declination_rate(const double &);
  bool does_refraction();
  int set_does_refraction(bool);
  equatorial_system_enum equatorial_system();
  double focal_length();
  int set_focal_length(const double &);
  double guide_rate_declination();
  int set_guide_rate_declination(const double &);
  double guide_rate_ascension();
  int set_guide_rate_ascension(const double &);
  bool is_pulse_guiding();
  double right_ascension();
  double right_ascension_rate();
  int set_right_ascension_rate(const double &);
  pier_side_enum side_of_pier();
  int set_side_of_pier(const pier_side_enum &);
  double sidereal_time();
  double site_elevation();
  int set_site_elevation(const double &);
  double site_latitude();
  int set_site_latitude(const double &);
  double site_longitude();
  int set_site_longitude(const double &);
  bool slewing();
  int slew_settle_time();
  int set_slew_settle_time(const int &);
  double target_declination();
  int set_target_declination(const double &);
  double target_right_ascension();
  int set_target_right_ascension(const double &);

  bool tracking();
  int set_tracking(const bool &);
  drive_rate_enum tracking_rate();
  int set_tracking_rate(const drive_rate_enum &);
  std::vector<drive_rate_enum> tracking_rates();
  std::string utc_date();
  int set_utc_date(const std::string &);
  int abort_slew();
  std::vector<axis_rate> axis_rates(const telescope_axes_enum &);
  bool can_move_axis(const telescope_axes_enum &);
  pier_side_enum destination_side_of_pier(const double &ra, const double &dec);
  int find_home();
  int move_axis(const telescope_axes_enum &, const double &);
  int park();
  int pulse_guide(const guide_direction_enum &direction,
                  const uint32_t &duration_ms);
  int set_park();
  int slew_to_alt_az(const double &alt, const double &az);
  int slew_to_alt_az_async(const double &alt, const double &az);
  int slew_to_coordinates(const double &ra, const double &dec);
  int slew_to_coordinates_async(const double &ra, const double &dec);

  int slew_to_target();
  int slew_to_target_async();
  int sync_to_alt_az(const double &alt, const double &az);
  int sync_to_coordinates(const double &ra, const double &dec);
  int sync_to_target();
  int unpark();

  int set_serial_device(const std::string &);
  std::string get_serial_device_path();

  std::string send_command_to_mount(const std::string &cmd,
                                    bool read_response = true,
                                    char stop_on_char = '#');

  std::string get_serial_number();
private:
  asio::io_context _io_ctx;
  std::mutex _telescope_mtx;
  std::mutex _moving_mtx;
  std::string _serial_device_path;
  std::thread _guiding_thread;
  // std::unique_ptr<asio::serial_port> _serial_port;
  // std::unique_ptr<asio::io_service> _io_service;
  asio::io_context _io_context;
  asio::serial_port _serial_port;
  bool _connected;
  double _aperture_diameter;
  double _focal_length;
  void throw_if_not_connected();
  void throw_if_parked();
  void block_while_moving();
  void set_is_moving(bool);
  void pulse_guide_proc(int duration_ms, char cardinal_direction);
  double _site_elevation;
  double _site_latitude;
  double _site_longitude;
  double _guide_rate;
  double _slew_settle_time;
  bool _parked;
  bool _moving;
  bool _is_pulse_guiding;
  bool _ra_target_set;
  bool _dec_target_set;
  bool _tracking_enabled;
};

#endif
