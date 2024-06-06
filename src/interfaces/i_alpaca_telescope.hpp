#ifndef I_ALPACA_TELESCOPE_HPP
#define I_ALPACA_TELESCOPE_HPP

#include "i_alpaca_device.hpp"

#include "fmt/format.h"

enum pier_side_enum : int { east = 0, west = 1, unknown = -1 };

struct axis_rate {
  double Max;
  double Min;
  axis_rate(const double &max_, const double &min_) : Max(max_), Min(min_){};

};

enum guide_direction_enum : int {
  guide_north = 0,
  guide_south = 1,
  guide_east = 2,
  guide_west = 3
};

enum drive_rate_enum : int { sidereal = 0, lunar = 1, solar = 2, king = 3 };

enum alignment_mode_enum : int { alt_az = 0, polar = 1, german_polar = 2 };

enum equatorial_system_enum : int {
  // Custom or unknown equinox and/or reference frame.
  other = 0,

  // Topocentric coordinates. Coordinates of the object at the current
  // date having allowed for annual aberration, precession and
  // nutation. This is the most common coordinate type for amateur
  // telescopes.
  topocentric = 1,

  // J2000 equator/equinox. Coordinates of the object at mid-day on 1st
  // January
  // 2000, ICRS reference frame.
  j2000 = 2,

  // J2050 equator/equinox, ICRS reference frame.
  j2050 = 3,

  // B1950 equinox, FK4 reference frame.
  b1950 = 4,

  // Obsolete. Please use equTopocentric instead - see Astronomical
  // Coordinates
  // for an explanation.
  local_topocentric = 1
};

enum telescope_axes_enum : int { primary = 0, secondary = 1, tertiary = 2 };

class i_alpaca_telescope : public i_alpaca_device {
public:


  virtual ~i_alpaca_telescope(){};
  virtual alignment_mode_enum alignment_mode() = 0;
  virtual double altitude() = 0;
  virtual double aperture_diameter() = 0;
  virtual double aperture_area() = 0;
  virtual int set_aperture_diameter(const double &) = 0;
  virtual bool at_home() = 0;
  virtual bool at_park() = 0;
  virtual double azimuth() = 0;
  virtual bool can_find_home() = 0;
  virtual bool can_park() = 0;
  virtual bool can_pulse_guide() = 0;
  virtual bool can_set_declination_rate() = 0;
  virtual bool can_set_guide_rates() = 0;
  virtual bool can_set_park() = 0;
  virtual bool can_set_pier_side() = 0;
  virtual bool can_set_right_ascension_rate() = 0;
  virtual bool can_set_tracking() = 0;
  virtual bool can_slew() = 0;
  virtual bool can_slew_async() = 0;
  virtual bool can_slew_alt_az() = 0;
  virtual bool can_slew_alt_az_async() = 0;
  virtual bool can_sync() = 0;
  virtual bool can_sync_alt_az() = 0;
  virtual bool can_unpark() = 0;
  virtual double declination() = 0;
  virtual double declination_rate() = 0;
  virtual int set_declination_rate(const double &) = 0;
  virtual bool does_refraction() = 0;
  virtual int set_does_refraction(bool) = 0;
  virtual equatorial_system_enum equatorial_system() = 0;
  virtual double focal_length() = 0;
  virtual double guide_rate_declination() = 0;
  virtual int set_guide_rate_declination(const double &) = 0;
  virtual double guide_rate_ascension() = 0;
  virtual int set_guide_rate_ascension(const double &) = 0;
  virtual bool is_pulse_guiding() = 0;
  virtual double right_ascension() = 0;
  virtual double right_ascension_rate() = 0;
  virtual int set_right_ascension_rate(const double &) = 0;
  virtual pier_side_enum side_of_pier() = 0;
  virtual int set_side_of_pier(const pier_side_enum &) = 0;
  virtual double sidereal_time() = 0;
  virtual double site_elevation() = 0;
  virtual int set_site_elevation(const double &) = 0;
  virtual double site_latitude() = 0;
  virtual int set_site_latitude(const double &) = 0;
  virtual double site_longitude() = 0;
  virtual int set_site_longitude(const double &) = 0;
  virtual bool slewing() = 0;
  virtual int slew_settle_time() = 0;
  virtual int set_slew_settle_time(const int &) = 0;
  virtual double target_declination() = 0;
  virtual int set_target_declination(const double &) = 0;
  virtual double target_right_ascension() = 0;
  virtual int set_target_right_ascension(const double &) = 0;

  virtual bool tracking() = 0;
  virtual int set_tracking(const bool &) = 0;
  virtual drive_rate_enum tracking_rate() = 0;
  virtual int set_tracking_rate(const drive_rate_enum &) = 0;
  virtual std::vector<drive_rate_enum> tracking_rates() = 0;
  virtual std::string utc_date() = 0;
  virtual int set_utc_date(const std::string &) = 0;
  virtual int abort_slew() = 0;
  virtual std::vector<axis_rate> axis_rates(const telescope_axes_enum &) = 0;
  virtual bool can_move_axis(const telescope_axes_enum &) = 0;
  virtual pier_side_enum destination_side_of_pier(const double &ra, const double &dec) = 0;
  virtual int find_home() = 0;
  virtual int move_axis(const telescope_axes_enum &, const double &) = 0;
  virtual int park() = 0;
  virtual int pulse_guide(const guide_direction_enum &direction,
                          const uint32_t &duration_ms) = 0;
  virtual int set_park() = 0;
  virtual int slew_to_alt_az(const double &alt, const double &az) = 0;
  virtual int slew_to_alt_az_async(const double &alt, const double &az) = 0;
  virtual int slew_to_coordinates(const double &ra, const double &dec) = 0;
  virtual int slew_to_coordinates_async(const double &ra, const double &dec) = 0;
  virtual int slew_to_target() = 0;
  virtual int slew_to_target_async() = 0;
  virtual int sync_to_alt_az(const double &alt, const double &az) = 0;
  virtual int sync_to_coordinates(const double &ra, const double &dec) = 0;
  virtual int sync_to_target() = 0;
  virtual int unpark() = 0;
};

#endif
