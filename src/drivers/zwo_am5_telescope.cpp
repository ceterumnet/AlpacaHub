#include "alpaca_exception.hpp"
#include "zwo_am5_telescope.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace zwo_commands {

// Time and location commands
const std::string cmd_get_date() { return ":GC#"; }

// I wonder if I should create a date type and ensure it is a valid date?
const std::string cmd_set_date(const int &mm, const int &dd, const int &yy) {
  if (mm < 1 || mm > 12)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "month must be 1 through 12");
  if (dd < 1 || dd > 31)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "day must be 1 through 31");
  if (yy < 0 || yy > 99)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "year must be 0 through 99");

  return fmt::format(":SC{:#02d}/{:#02d}/{:02d}#", mm, dd, yy);
}
const std::string cmd_switch_to_eq_mode() { return ":AP#"; }
const std::string cmd_switch_to_alt_az_mode() { return ":AA#"; }
const std::string cmd_get_time() { return ":GL#"; }
const std::string cmd_set_time(const int &hh, const int &mm, const int &ss) {
  if (hh < 0 || hh > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hour must be 0 through 23");
  if (mm < 0 || mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ss < 0 || ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 99");

  return fmt::format(":SL{0:#02d}:{1:#02d}:{2:#02d}#", hh, mm, ss);
}
const std::string cmd_get_sidereal_time() { return ":GS#"; }
const std::string cmd_get_daylight_savings() { return ":GH#"; }
const std::string cmd_set_daylight_savings(const int &on_or_off) {
  return fmt::format(":SH{0}#", on_or_off);
}

const std::string cmd_set_timezone(const char &plus_or_minus,
                                   const int &h_offset, int m_offset = 0) {
  return fmt::format(":SG{0}{1:#02d}:{2:#02d}", plus_or_minus, h_offset,
                     m_offset);
}

const std::string cmd_get_timezone() { return ":GG#"; }

const std::string cmd_set_latitude(const char &plus_or_minus, const int &dd,
                                   const int &mm, int ss) {
  return fmt::format(":St{0}{1:#02d}*{2:#02d}:{3:#02d}#", plus_or_minus, dd, mm,
                     ss);
}

const std::string cmd_get_latitude() { return ":Gt#"; }

const std::string cmd_set_longitude(const int &ddd, const int &mm,
                                    const int &ss) {
  return fmt::format(":Sgs{0:#03d}*{1:#02d}:{2:#02d}#", ddd, mm, ss);
}

const std::string cmd_get_longitude() { return ":Gg#"; }

const std::string cmd_get_current_cardinal_direction() { return ":Gm#"; }

// Moving commands

const std::string cmd_set_target_ra(const int &hh, const int &mm,
                                    const int &ss) {
  return fmt::format(":Sr{0:#02d}:{1:#02d}:{2:#02d}#", hh, mm, ss);
}
}; // namespace zwo_commands

TEST_CASE("ZWO commands are correctly formatted and padded with happy values",
          "[zwo_commands]") {
  using namespace zwo_commands;

  REQUIRE(cmd_get_date() == ":GC#");
  REQUIRE(cmd_set_date(1.0, 2.0, 24.0) == ":SC01/02/24#");
  REQUIRE(cmd_switch_to_eq_mode() == ":AP#");
  REQUIRE(cmd_switch_to_alt_az_mode() == ":AA#");
  REQUIRE(cmd_get_time() == ":GL#");
  REQUIRE(cmd_set_time(13, 54, 59) == ":SL13:54:59#");
}

TEST_CASE("ZWO commands throw exception with invalid values",
          "[zwo_commands]") {
  using namespace zwo_commands;

  REQUIRE_THROWS_AS(cmd_set_date(14.0, 2.0, 24.0), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_date(11.0, 40.0, 24.0), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_date(11.0, 10.0, 100.0), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_time(25, 54, 59), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_time(23, 61, 59), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_time(23, 54, 61), alpaca_exception);
}

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

zwo_am5_telescope::equatorial_system_enum
zwo_am5_telescope::equatorial_system() {
  return equatorial_system_enum::j2000;
}
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

std::vector<zwo_am5_telescope::drive_rate_enum>
zwo_am5_telescope::tracking_rates() {
  return std::vector<drive_rate_enum>();
}
std::string zwo_am5_telescope::utc_time() { return ""; }

int zwo_am5_telescope::set_utc_time(std::string &) { return 0; }

int zwo_am5_telescope::abort_slew() { return 0; }

std::vector<zwo_am5_telescope::axis_rate>
zwo_am5_telescope::axis_rates(telescope_axes_enum) {
  return std::vector<axis_rate>();
}
bool zwo_am5_telescope::can_move_axis(telescope_axes_enum) { return false; }

zwo_am5_telescope::pier_side_enum
zwo_am5_telescope::destination_side_of_pier(double ra, double dec) {
  return pier_side_enum::unknown;
}

int zwo_am5_telescope::find_home() { return 0; }

int zwo_am5_telescope::move_axis(telescope_axes_enum, axis_rate) { return 0; }

int zwo_am5_telescope::park() { return 0; }

int zwo_am5_telescope::pulse_guide(guide_direction_enum direction,
                                   uint32_t duration_ms) {
  return 0;
}

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
