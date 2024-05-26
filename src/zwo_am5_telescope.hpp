#ifndef ZWO_AM5_TELESCOPE_HPP
#define ZWO_AM5_TELESCOPE_HPP

#include "i_alpaca_telescope.hpp"


class zwo_am5_telescope : public i_alpaca_telescope {
public:
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
  bool can_slew_alt_az();
  bool can_slew_alt_az_async();
  bool can_sync();
  bool can_sync_alt_az();
  bool can_unpark();
  double declination();
  double declination_rate();
  int set_declination_rate(double);
  bool does_refraction();
  int set_does_refraction(bool);
  equatorial_system_enum equatorial_system();
  double focal_length();
  double guide_rate_declination();
  int set_guide_rate_declination(double);
  double guide_rate_ascension();
  int set_guide_rate_ascension(double);
  bool is_pulse_guiding();
  double right_ascension();
  double right_ascension_rate();
  int set_right_ascension_rate();
  pier_side_enum side_of_pier();
  int set_side_of_pier(pier_side_enum);
  double sidereal_time();
  double site_elevation();
  int set_site_elevation(double);
  double site_latitude();
  int set_site_latitude(double);
  double site_longitude();
  int set_site_longitude(double);
  bool slewing();
  double slew_settle_time();
  int set_slew_settle_time(double);
  double target_declination();
  int set_target_declination(double);
  bool tracking();
  int set_tracking(bool);
  drive_rate_enum tracking_rate();
  int tracking_rate(drive_rate_enum);
  std::vector<drive_rate_enum> tracking_rates();
  std::string utc_time();
  int set_utc_time(std::string &);
  int abort_slew();
  std::vector<axis_rate> axis_rates(telescope_axes_enum);
  bool can_move_axis(telescope_axes_enum);
  pier_side_enum destination_side_of_pier(double ra, double dec);
  int find_home();
  int move_axis(telescope_axes_enum, axis_rate);
  int park();
  int pulse_guide(guide_direction_enum direction,
                  uint32_t duration_ms);
  int set_park();
  int slew_to_alt_az(double alt, double az);
  int slew_to_alt_az_async(double alt, double az);
  int slew_to_coordinates(double ra, double dec);
  int slew_to_target();
  int slew_to_target_async();
  int sync_to_alt_az(double alt, double az);
  int sync_to_coordinates(double ra, double dec);
  int sync_to_target();
  int unpark();

};


#endif
