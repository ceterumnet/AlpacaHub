#include "zwo_am5_telescope.hpp"
#include "common/alpaca_exception.hpp"
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
                           "seconds must 0 through 59");

  return fmt::format(":SL{0:#02d}:{1:#02d}:{2:#02d}#", hh, mm, ss);
}

const std::string cmd_get_sidereal_time() { return ":GS#"; }

const std::string cmd_get_daylight_savings() { return ":GH#"; }

const std::string cmd_set_daylight_savings(const int &on_or_off) {
  if (on_or_off < 0 || on_or_off > 1)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE, "must be 0 or 1");
  return fmt::format(":SH{0}#", on_or_off);
}

const std::string cmd_set_timezone(const char &plus_or_minus,
                                   const int &h_offset, int m_offset = 0) {
  if (plus_or_minus != '+' && plus_or_minus != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "first param must be '+' or '-'");
  if (h_offset < 0 || h_offset > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hour offest must be 0 through 23");
  if (m_offset != 0 && m_offset != 30)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes offset must be 0 or 30");

  return fmt::format(":SG{0}{1:#02d}:{2:#02d}#", plus_or_minus, h_offset,
                     m_offset);
}

const std::string cmd_get_timezone() { return ":GG#"; }

const std::string cmd_set_latitude(const char &plus_or_minus, const int &dd,
                                   const int &mm, int ss) {
  if (plus_or_minus != '+' && plus_or_minus != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "first param must be '+' or '-'");
  if (dd < 0 || dd > 90)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "latitude degrees must be 0 through 90");
  if (mm < 0 || mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ss < 0 || ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");
  return fmt::format(":St{0}{1:#02d}*{2:#02d}:{3:#02d}#", plus_or_minus, dd, mm,
                     ss);
}

const std::string cmd_get_latitude() { return ":Gt#"; }

const std::string cmd_set_longitude(const char &plus_or_minus, const int &ddd,
                                    const int &mm, const int &ss) {
  if (plus_or_minus != '+' && plus_or_minus != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "first param must be '+' or '-'");
  if (ddd < 0 || ddd > 180)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "longitude degrees must be 0 through 180");
  if (mm < 0 || mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ss < 0 || ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");
  return fmt::format(":Sg{0}{1:#03d}*{2:#02d}:{3:#02d}#", plus_or_minus, ddd,
                     mm, ss);
}

const std::string cmd_get_longitude() { return ":Gg#"; }

const std::string cmd_get_current_cardinal_direction() { return ":Gm#"; }

// Moving commands

const std::string cmd_set_target_ra(const int &hh, const int &mm,
                                    const int &ss) {
  if (hh < 0 || hh > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hours must 0 through 23");
  if (mm < 0 || mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ss < 0 || ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");

  return fmt::format(":Sr{0:#02d}:{1:#02d}:{2:#02d}#", hh, mm, ss);
}

const std::string cmd_get_target_ra() { return ":Gr#"; }

const std::string cmd_set_target_dec(const char &plus_or_minus, const int &dd,
                                     const int &mm, const int &ss) {
  if (plus_or_minus != '+' && plus_or_minus != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "first param must be '+' or '-'");
  if (dd < 0 || dd > 90)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "degrees must 0 through 90");
  if (mm < 0 || mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ss < 0 || ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");

  return fmt::format(":Sd{0}{1:#02d}:{2:#02d}:{3:#02d}#", plus_or_minus, dd, mm,
                     ss);
}

const std::string cmd_get_target_dec() { return ":Gd#"; }

const std::string cmd_get_current_ra() { return ":GR#"; }

const std::string cmd_get_current_dec() { return ":GD#"; }

const std::string cmd_get_azimuth() { return ":GZ#"; }

const std::string cmd_get_altitude() { return ":GA#"; }

const std::string cmd_goto() { return ":MS#"; }

const std::string cmd_stop_moving() { return ":Q#"; }

enum move_speed_enum : int {
  speed_0_25x = 0,
  speed_0_5x = 1,
  speed_1x = 2,
  speed_2x = 3,
  speed_4x = 4,
  speed_8x = 5,
  speed_20x = 6,
  speed_60x = 7,
  speed_720x = 8,
  speed_1440x = 9
};

auto format_as(move_speed_enum s) { return fmt::underlying(s); }

// 0 - 9 move speed corresponds with:
// .25, .5, 1, 2, 4, 8, 20, 60, 720, 1440
const std::string cmd_set_moving_speed(move_speed_enum move_speed) {
  if (move_speed < 0 || move_speed > 9)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "Move speed must be 0 through 9");

  return fmt::format(":R{0}#", move_speed);
}

const std::string cmd_set_0_5x_sidereal_rate() { return ":RG#"; }

const std::string cmd_set_1x_sidereal_rate() { return ":RC#"; }

const std::string cmd_set_720x_sidereal_rate() { return ":RM#"; }

const std::string cmd_set_1440x_sidereal_rate() { return ":RS#"; }

const std::string cmd_set_moving_speed_precise(const double &move_speed) {
  if (move_speed < 0 || move_speed > 1440)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "move speed must be between 0 and 1440.00");
  return fmt::format(":Rv{0:#04.2f}#", move_speed);
}

const std::string cmd_move_towards_east() { return ":Me#"; }

const std::string cmd_stop_moving_towards_east() { return ":Qe#"; }

const std::string cmd_move_towards_west() { return ":Mw#"; }

const std::string cmd_stop_moving_towards_west() { return ":Qw#"; }

const std::string cmd_move_towards_north() { return ":Mn#"; }

const std::string cmd_stop_moving_towards_north() { return ":Qn#"; }

const std::string cmd_move_towards_south() { return ":Ms#"; }

const std::string cmd_stop_moving_towards_south() { return ":Qs#"; }

const std::string cmd_set_guide_rate_to_sidereal() { return ":TQ#"; }

const std::string cmd_set_guide_rate_to_solar() { return ":TS#"; }

const std::string cmd_set_guide_rate_to_lunar() { return ":TL#"; }

const std::string cmd_get_tracking_rate() { return ":GT#"; }

const std::string cmd_start_tracking() { return ":Te#"; }

const std::string cmd_stop_tracking() { return ":Td#"; }

const std::string cmd_get_tracking_status() { return ":GAT#"; }

const std::string cmd_guide(const char &direction, const int &rate) {
  if (direction != 'e' && direction != 'w' && direction != 'n' &&
      direction != 's')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "direction must be e, w, n, or s");
  if (rate < 0 || rate > 3000)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "rate must be between 0 and 3000");
  return fmt::format(":Mg{0}{1:#04d}#", direction, rate);
}

const std::string cmd_set_guide_rate(const double &guide_rate) {
  if (guide_rate < .1 || guide_rate > .9)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "guide rate must be between .1 and .9");
  return fmt::format(":Rg{0:#01.2f}#", guide_rate);
}

const std::string cmd_get_guide_rate() { return ":Ggr#"; }

const std::string
cmd_set_act_of_crossing_meridian(const int &perform_meridian_flip,
                                 const int &continue_to_track_after_meridian,
                                 const char &plus_or_minus,
                                 const int &limit_angle_after_meridian) {

  if (perform_meridian_flip != 0 && perform_meridian_flip != 1)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "perform_meridian_flip must be 0 or 1");
  if (continue_to_track_after_meridian != 0 && continue_to_track_after_meridian != 1)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "continue_to_track_after_meridian must be 0 or 1");
  if (plus_or_minus != '+' && plus_or_minus != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "sign of limit angle be '+' or '-'");

  if (limit_angle_after_meridian < 0 || limit_angle_after_meridian > 15)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE, "limit_angle_after_meridian must be between 0 and 15");
  return fmt::format(":STa{0:#01d}{1:#01d}{2}{3:#02d}#", perform_meridian_flip,
                     continue_to_track_after_meridian,
                     plus_or_minus,
                     limit_angle_after_meridian);
}

}; // namespace zwo_commands

TEST_CASE("ZWO commands are correctly formatted and padded with happy values",
          "[zwo_commands_happy]") {
  using namespace zwo_commands;

  REQUIRE(cmd_get_date() == ":GC#");
  REQUIRE(cmd_set_date(1.0, 2.0, 24.0) == ":SC01/02/24#");

  REQUIRE(cmd_switch_to_eq_mode() == ":AP#");
  REQUIRE(cmd_switch_to_alt_az_mode() == ":AA#");

  REQUIRE(cmd_get_time() == ":GL#");
  REQUIRE(cmd_set_time(13, 54, 59) == ":SL13:54:59#");

  REQUIRE(cmd_get_sidereal_time() == ":GS#");

  REQUIRE(cmd_get_daylight_savings() == ":GH#");

  REQUIRE(cmd_set_daylight_savings(0) == ":SH0#");
  REQUIRE(cmd_set_daylight_savings(1) == ":SH1#");

  REQUIRE(cmd_set_timezone('-', 5) == ":SG-05:00#");
  REQUIRE(cmd_set_timezone('-', 5, 30) == ":SG-05:30#");

  REQUIRE(cmd_get_timezone() == ":GG#");

  REQUIRE(cmd_set_latitude('+', 30, 03, 48) == ":St+30*03:48#");
  REQUIRE(cmd_set_latitude('-', 30, 03, 48) == ":St-30*03:48#");

  REQUIRE(cmd_get_latitude() == ":Gt#");

  REQUIRE(cmd_set_longitude('+', 23, 25, 21) == ":Sg+023*25:21#");
  REQUIRE(cmd_set_longitude('-', 180, 59, 59) == ":Sg-180*59:59#");

  REQUIRE(cmd_get_longitude() == ":Gg#");

  REQUIRE(cmd_get_current_cardinal_direction() == ":Gm#");

  REQUIRE(cmd_set_target_ra(5, 45, 43) == ":Sr05:45:43#");

  REQUIRE(cmd_get_target_ra() == ":Gr#");

  REQUIRE(cmd_set_target_dec('+', 89, 4, 4) == ":Sd+89:04:04#");
  REQUIRE(cmd_set_target_dec('-', 0, 14, 4) == ":Sd-00:14:04#");

  REQUIRE(cmd_get_target_dec() == ":Gd#");

  REQUIRE(cmd_get_current_ra() == ":GR#");

  REQUIRE(cmd_get_current_dec() == ":GD#");

  REQUIRE(cmd_get_azimuth() == ":GZ#");

  REQUIRE(cmd_get_altitude() == ":GA#");

  REQUIRE(cmd_goto() == ":MS#");

  REQUIRE(cmd_stop_moving() == ":Q#");

  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_0_25x) == ":R0#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_0_5x) == ":R1#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_1x) == ":R2#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_2x) == ":R3#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_4x) == ":R4#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_8x) == ":R5#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_20x) == ":R6#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_60x) == ":R7#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_720x) == ":R8#");
  REQUIRE(cmd_set_moving_speed(move_speed_enum::speed_1440x) == ":R9#");

  REQUIRE(cmd_set_0_5x_sidereal_rate() == ":RG#");
  REQUIRE(cmd_set_1x_sidereal_rate() == ":RC#");
  REQUIRE(cmd_set_720x_sidereal_rate() == ":RM#");
  REQUIRE(cmd_set_1440x_sidereal_rate() == ":RS#");

  REQUIRE(cmd_set_moving_speed_precise(1211.1) == ":Rv1211.10#");

  REQUIRE(cmd_move_towards_east() == ":Me#");

  REQUIRE(cmd_stop_moving_towards_east() == ":Qe#");

  REQUIRE(cmd_move_towards_west() == ":Mw#");

  REQUIRE(cmd_stop_moving_towards_west() == ":Qw#");

  REQUIRE(cmd_move_towards_north() == ":Mn#");

  REQUIRE(cmd_stop_moving_towards_north() == ":Qn#");

  REQUIRE(cmd_move_towards_south() == ":Ms#");

  REQUIRE(cmd_stop_moving_towards_south() == ":Qs#");

  REQUIRE(cmd_set_guide_rate_to_sidereal() == ":TQ#");

  REQUIRE(cmd_set_guide_rate_to_solar() == ":TS#");

  REQUIRE(cmd_set_guide_rate_to_lunar() == ":TL#");

  REQUIRE(cmd_get_tracking_rate() == ":GT#");

  REQUIRE(cmd_start_tracking() == ":Te#");

  REQUIRE(cmd_stop_tracking() == ":Td#");

  REQUIRE(cmd_get_tracking_status() == ":GAT#");

  REQUIRE(cmd_guide('e', 5) == ":Mge0005#");

  REQUIRE(cmd_set_guide_rate(.1) == ":Rg0.10#");

  REQUIRE(cmd_get_guide_rate() == ":Ggr#");

  REQUIRE(cmd_set_act_of_crossing_meridian(1, 1, '+', 14) == ":STa11+14#");
}

TEST_CASE("ZWO commands throw exception with invalid values",
          "[zwo_commands_sad]") {
  using namespace zwo_commands;

  REQUIRE_THROWS_AS(cmd_set_date(14.0, 2.0, 24.0), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_date(11.0, 40.0, 24.0), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_date(11.0, 10.0, 100.0), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_time(25, 54, 59), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_time(23, 61, 59), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_time(23, 54, 61), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_daylight_savings(2), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_daylight_savings(-1), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_timezone('0', 5), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_timezone('+', 5, 29), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_timezone('+', 25), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_timezone('+', -1), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_latitude('0', 30, 03, 48), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_latitude('+', 91, 03, 48), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_latitude('+', 89, 61, 48), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_latitude('+', 89, 59, 60), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_longitude('?', 23, 25, 21), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_longitude('+', 181, 25, 21), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_longitude('+', 180, 60, 21), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_longitude('+', 179, 60, 21), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_longitude('+', 179, 58, 60), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_target_ra(24, 45, 43), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_target_ra(23, 60, 43), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_target_ra(23, 59, 60), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_moving_speed_precise(1441.00), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_moving_speed_precise(-41.00), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_guide('z', 2000), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_guide('e', 3001), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_guide('e', -1), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_guide_rate(.01), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_guide_rate(-0.1), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_guide_rate(.91), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_guide_rate(1.91), alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_act_of_crossing_meridian(2, 1, '+', 14), alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_act_of_crossing_meridian(1, 2, '+', 14),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_act_of_crossing_meridian(1, 1, '?', 14),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_act_of_crossing_meridian(1, 1, '+', 16),
                    alpaca_exception);
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
