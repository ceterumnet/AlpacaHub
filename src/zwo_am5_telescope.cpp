#include "zwo_am5_telescope.hpp"
#include "i_alpaca_telescope.hpp"
#include <vector>

std::string zwo_am5_telescope::unique_id() { return ""; }

bool zwo_am5_telescope::connected() { return false; }

int zwo_am5_telescope::set_connected(bool) { return 0; }

zwo_am5_telescope::zwo_am5_telescope(){};

zwo_am5_telescope::~zwo_am5_telescope(){};

uint32_t zwo_am5_telescope::interface_version() { return 3; }

std::string zwo_am5_telescope::driver_version() { return "v0.1"; }

std::string zwo_am5_telescope::description() {
  return "ZWO AM5 Telescope Driver";
}

std::string zwo_am5_telescope::driverinfo() {
  return "ZWO AM5 Telescop Driver Information";
}

std::string zwo_am5_telescope::name() { return ""; };

std::vector<std::string> zwo_am5_telescope::supported_actions() {
  return std::vector<std::string>();
}

zwo_am5_telescope::alignment_mode_enum zwo_am5_telescope::alignment_mode() {
  return alignment_mode_enum::german_polar;
}

double zwo_am5_telescope::altitude() { return 0; }

double zwo_am5_telescope::aperture_diameter() { return 0; }

bool zwo_am5_telescope::at_home() { return false; }

bool zwo_am5_telescope::at_park() { return false; }

double zwo_am5_telescope::azimuth() { return 0; }

bool zwo_am5_telescope::can_find_home() { return false; }

bool zwo_am5_telescope::can_park() { return false; }

bool zwo_am5_telescope::can_pulse_guide() { return false; }

bool zwo_am5_telescope::can_set_declination_rate() { return false; }

bool zwo_am5_telescope::can_set_guide_rates() { return false; }

bool zwo_am5_telescope::can_set_park() { return false; }

bool zwo_am5_telescope::can_set_pier_side() { return false; }

bool zwo_am5_telescope::can_set_right_ascension_rate() { return false; }

bool zwo_am5_telescope::can_set_tracking() { return false; }

bool zwo_am5_telescope::can_slew() { return false; }

bool zwo_am5_telescope::can_slew_alt_az() { return false; }

bool zwo_am5_telescope::can_slew_alt_az_async() { return false; }

bool zwo_am5_telescope::can_sync() { return false; }

bool zwo_am5_telescope::can_sync_alt_az() { return false; }

bool zwo_am5_telescope::can_unpark() { return false; }

double zwo_am5_telescope::declination() { return 0; }

double zwo_am5_telescope::declination_rate() { return 0; }

int zwo_am5_telescope::set_declination_rate(double) { return 0; }

bool zwo_am5_telescope::does_refraction() { return 0; }

int zwo_am5_telescope::set_does_refraction(bool) { return 0; }

zwo_am5_telescope::equatorial_system_enum zwo_am5_telescope::equatorial_system() { return equatorial_system_enum::j2000
    ; }
double zwo_am5_telescope::focal_length() { return 0; }

double zwo_am5_telescope::guide_rate_declination() { return 0; }

int zwo_am5_telescope::set_guide_rate_declination(double) { return 0; }

double zwo_am5_telescope::guide_rate_ascension() { return 0; }

int zwo_am5_telescope::set_guide_rate_ascension(double) { return 0; }

bool zwo_am5_telescope::is_pulse_guiding() { return 0; }

double zwo_am5_telescope::right_ascension() { return 0; }

double zwo_am5_telescope::right_ascension_rate() { return 0; }

int zwo_am5_telescope::set_right_ascension_rate() { return 0; }

zwo_am5_telescope::pier_side_enum zwo_am5_telescope::side_of_pier() {
  return pier_side_enum::unknown;
}

int zwo_am5_telescope::set_side_of_pier(pier_side_enum) { return 0; }

double zwo_am5_telescope::sidereal_time() { return 0; }

double zwo_am5_telescope::site_elevation() { return 0; }

int zwo_am5_telescope::set_site_elevation(double) { return 0; }

double zwo_am5_telescope::site_latitude() { return 0; }

int zwo_am5_telescope::set_site_latitude(double) { return 0; }

double zwo_am5_telescope::site_longitude() { return 0; }

int zwo_am5_telescope::set_site_longitude(double) { return 0; }

bool zwo_am5_telescope::slewing() { return 0; }

double zwo_am5_telescope::slew_settle_time() { return 0; }

int zwo_am5_telescope::set_slew_settle_time(double) { return 0; }

double zwo_am5_telescope::target_declination() { return 0; }

int zwo_am5_telescope::set_target_declination(double) { return 0; }

bool zwo_am5_telescope::tracking() { return 0; }

int zwo_am5_telescope::set_tracking(bool) { return 0; }

zwo_am5_telescope::drive_rate_enum zwo_am5_telescope::tracking_rate() {
  return drive_rate_enum::sidereal;
}

int zwo_am5_telescope::tracking_rate(drive_rate_enum) { return 0; }

std::vector<zwo_am5_telescope::drive_rate_enum> zwo_am5_telescope::tracking_rates() {
  return std::vector<drive_rate_enum>();
}
std::string zwo_am5_telescope::utc_time() { return ""; }

int zwo_am5_telescope::set_utc_time(std::string &) { return 0; }

int zwo_am5_telescope::abort_slew() { return 0; }

std::vector<zwo_am5_telescope::axis_rate> zwo_am5_telescope::axis_rates(telescope_axes_enum) {
  return std::vector<axis_rate>();
}
bool zwo_am5_telescope::can_move_axis(telescope_axes_enum) { return false; }

zwo_am5_telescope::pier_side_enum zwo_am5_telescope::destination_side_of_pier(double ra,
                                                           double dec) {
  return pier_side_enum::unknown;
}

int zwo_am5_telescope::find_home() { return 0; }

int zwo_am5_telescope::move_axis(telescope_axes_enum, axis_rate) { return 0; }

int zwo_am5_telescope::park() { return 0; }

int zwo_am5_telescope::pulse_guide(guide_direction_enum direction,
                                   uint32_t duration_ms) { return 0; }

int zwo_am5_telescope::set_park() { return 0; }

int zwo_am5_telescope::slew_to_alt_az(double alt, double az) { return 0; }

int zwo_am5_telescope::slew_to_alt_az_async(double alt, double az) { return 0; }

int zwo_am5_telescope::slew_to_coordinates(double ra, double dec) { return 0; }

int zwo_am5_telescope::slew_to_target() { return 0; }

int zwo_am5_telescope::slew_to_target_async() { return 0; }

int zwo_am5_telescope::sync_to_alt_az(double alt, double az) { return 0; }

int zwo_am5_telescope::sync_to_coordinates(double ra, double dec) { return 0; }

int zwo_am5_telescope::sync_to_target() { return 0; }

int zwo_am5_telescope::unpark() { return 0; }
