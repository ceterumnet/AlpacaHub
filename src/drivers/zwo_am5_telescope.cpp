#include "zwo_am5_telescope.hpp"
#include "common/alpaca_exception.hpp"
#include "spdlog/spdlog.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

namespace zwo_commands {


  // This doesn't respond with anything
const std::string cmd_switch_to_eq_mode() { return ":AP#"; }

// This doesn't respond with anything
const std::string cmd_switch_to_alt_az_mode() { return ":AA#"; }

// response should be "MM/DD/YY#"
const std::string cmd_get_date() { return ":GC#"; }

// I wonder if I should create a date type and ensure it is a valid date?
  // response should be 1 for success or 0 for failure
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

  // Response should be 1 for success and 0 for failure
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

  // response should be "HH:MM:SS#"
const std::string cmd_get_time() { return ":GL#"; }

// response should be "HH:MM:SS#"
const std::string cmd_get_sidereal_time() { return ":GS#"; }

  // response is 1 for daylight savings on and 0 for off
const std::string cmd_get_daylight_savings() { return ":GH#"; }

  // response should be 1
const std::string cmd_set_daylight_savings(const int &on_or_off) {
  if (on_or_off < 0 || on_or_off > 1)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE, "must be 0 or 1");
  return fmt::format(":SH{0}#", on_or_off);
}

  // response should be 1 for success and 0 for failure
const std::string cmd_set_timezone(const char &plus_or_minus,
                                   const int &h_offset, int m_offset = 0) {
  if (plus_or_minus != '+' && plus_or_minus != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "first param must be '+' or '-'");
  if (h_offset < 0 || h_offset > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hour offset must be 0 through 23");
  if (m_offset != 0 && m_offset != 30)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes offset must be 0 or 30");

  return fmt::format(":SG{0}{1:#02d}:{2:#02d}#", plus_or_minus, h_offset,
                     m_offset);
}

  // Response should be "sHH:MM#"
const std::string cmd_get_timezone() { return ":GG#"; }

  // Response should be 1 for success and 0 for failure
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

// Response should be "sDD*MM#"
const std::string cmd_get_latitude() { return ":Gt#"; }

  // Response should be 1 for success and 0 for failure
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

// Response should be "sDDD*MM#"
const std::string cmd_get_longitude() { return ":Gg#"; }

  // Response should be "E or W or N for home / zero position"
const std::string cmd_get_current_cardinal_direction() { return ":Gm#"; }

// Response should be "HH:MM:SS#"
const std::string cmd_get_target_ra() { return ":Gr#"; }

// Response should be 1 for success or 0 for failure
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

// Response should be "DD:MM:SS#"
// TODO: verify that this doesn't return a * after DD
const std::string cmd_get_target_dec() { return ":Gd#"; }

// Response should be 1 for success 0 or for failure
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

  // Response should be "HH:MM:SS#"
const std::string cmd_get_current_ra() { return ":GR#"; }

  // Response should be "sDD*MM:SS#"
const std::string cmd_get_current_dec() { return ":GD#"; }

  // Response should be "DDD*MM:SS#"
const std::string cmd_get_azimuth() { return ":GZ#"; }

  // Response should be "sDD*MM:SS#"
const std::string cmd_get_altitude() { return ":GA#"; }

  // Response should be 1 for success or "e2#"
const std::string cmd_goto() { return ":MS#"; }

  // Response should be none
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
// Response should be none
const std::string cmd_set_moving_speed(move_speed_enum move_speed) {
  if (move_speed < 0 || move_speed > 9)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "Move speed must be 0 through 9");

  return fmt::format(":R{0}#", move_speed);
}

  // Response should be none
const std::string cmd_set_0_5x_sidereal_rate() { return ":RG#"; }

// Response should be none
const std::string cmd_set_1x_sidereal_rate() { return ":RC#"; }

// Response should be none
const std::string cmd_set_720x_sidereal_rate() { return ":RM#"; }

// Response should be none
const std::string cmd_set_1440x_sidereal_rate() { return ":RS#"; }

// Response should be none
const std::string cmd_set_moving_speed_precise(const double &move_speed) {
  if (move_speed < 0 || move_speed > 1440)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "move speed must be between 0 and 1440.00");
  return fmt::format(":Rv{0:#04.2f}#", move_speed);
}

// Response should be none
const std::string cmd_move_towards_east() { return ":Me#"; }

// Response should be none
const std::string cmd_stop_moving_towards_east() { return ":Qe#"; }

// Response should be none
const std::string cmd_move_towards_west() { return ":Mw#"; }

// Response should be none
const std::string cmd_stop_moving_towards_west() { return ":Qw#"; }

// Response should be none
const std::string cmd_move_towards_north() { return ":Mn#"; }

// Response should be none
const std::string cmd_stop_moving_towards_north() { return ":Qn#"; }

// Response should be none
const std::string cmd_move_towards_south() { return ":Ms#"; }

// Response should be none
const std::string cmd_stop_moving_towards_south() { return ":Qs#"; }

// Response should be none
const std::string cmd_set_guide_rate_to_sidereal() { return ":TQ#"; }

// Response should be none
const std::string cmd_set_guide_rate_to_solar() { return ":TS#"; }

// Response should be none
const std::string cmd_set_guide_rate_to_lunar() { return ":TL#"; }

// Response will be one of 1# 2# or 3# corresponding with the tracking rate enum
const std::string cmd_get_tracking_rate() { return ":GT#"; }

  // Returns 1 for success or 0 for failure
const std::string cmd_start_tracking() { return ":Te#"; }

// Returns 1 for success or 0 for failure
const std::string cmd_stop_tracking() { return ":Td#"; }

// Returns 0# for tracking off, 1# for tracking on, e+error code+#
const std::string cmd_get_tracking_status() { return ":GAT#"; }

// Response should be none
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

  // Response should be none
const std::string cmd_set_guide_rate(const double &guide_rate) {
  if (guide_rate < .1 || guide_rate > .9)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "guide rate must be between .1 and .9");
  return fmt::format(":Rg{0:#01.2f}#", guide_rate);
}

  // Response is 0.nn#
const std::string cmd_get_guide_rate() { return ":Ggr#"; }

  // Response is 1 for success and 0 for failure
const std::string
cmd_set_act_of_crossing_meridian(const int &perform_meridian_flip,
                                 const int &continue_to_track_after_meridian,
                                 const char &plus_or_minus,
                                 const int &limit_angle_after_meridian) {

  if (perform_meridian_flip != 0 && perform_meridian_flip != 1)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "perform_meridian_flip must be 0 or 1");
  if (continue_to_track_after_meridian != 0 &&
      continue_to_track_after_meridian != 1)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "continue_to_track_after_meridian must be 0 or 1");
  if (plus_or_minus != '+' && plus_or_minus != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "sign of limit angle be '+' or '-'");

  if (limit_angle_after_meridian < 0 || limit_angle_after_meridian > 15)
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        "limit_angle_after_meridian must be between 0 and 15");
  return fmt::format(":STa{0:#01d}{1:#01d}{2}{3:#02d}#", perform_meridian_flip,
                     continue_to_track_after_meridian, plus_or_minus,
                     limit_angle_after_meridian);
}

  // TODO: create structure to interpret this
// Response should be nnsnn#
const std::string cmd_get_act_of_crossing_meridian() { return ":GTa#"; }

// Response is N/A# for success and e2# for error
const std::string cmd_sync() { return ":CM#"; }

  // Response should be none
const std::string cmd_home_position() { return ":hC#"; }

// This is a complicated response...I'm not sure I have this one right yet
// nNG000000000#
// ^^^^^^^^^^^^
// |||||| | |||
// |||||| | || -> state
// |||||| | | --> DEC rate
// |||||| |  ---> RA rate
// ||||||  -----> flags of dec axis
// ||||| -------> flags of ra axis
//
// n N G 0 0 00 00 0 0 state 0# <- sample response from cmd_get_status
const std::string cmd_get_status() { return ":GU#"; }

  // Response should be none
const std::string cmd_park() { return ":hP#"; }

  // Response should be 1 for success or 0 for failure
// :SMGEsDD*MM:SS&sDDD*MM:SS#
const std::string cmd_set_lat_and_long(const char &plus_or_minus_lat,
                                       const int &lat_dd, const int lat_mm,
                                       const int &lat_ss,
                                       const char &plus_or_minus_long,
                                       const int &long_ddd, const int &long_mm,
                                       const int &long_ss) {
  if (plus_or_minus_lat != '+' && plus_or_minus_lat != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "lat +/- param must be '+' or '-'");
  if (lat_dd < 0 || lat_dd > 90)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "latitude degrees must be 0 through 90");
  if (lat_mm < 0 || lat_mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (lat_ss < 0 || lat_ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");
  if (plus_or_minus_long != '+' && plus_or_minus_long != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "longitude +/- param must be '+' or '-'");
  if (long_ddd < 0 || long_ddd > 180)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "longitude degrees must be 0 through 90");
  if (long_mm < 0 || long_mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (long_ss < 0 || long_ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");

  return fmt::format(
      ":SMGE{0}{1:#02d}*{2:#02d}:{3:#02d}&{4}{5:#03d}*{6:#02d}:{7:#02d}#",
      plus_or_minus_lat, lat_dd, lat_mm, lat_ss, plus_or_minus_long, long_ddd,
      long_mm, long_ss);
}

// Response should be "sDD*MM&sDDD*MM#"
const std::string cmd_get_lat_and_long() { return ":GMGE#"; }

// Response should be 1 for success and 0 for failure
// :SMTIMM/DD/YY&HH:MM:SS&sHH:MM#
const std::string cmd_set_date_time_and_tz(
    const int &date_mm, const int &date_dd, const int &date_yy,
    const int &time_hh, const int &time_mm, const int &time_ss,
    const char &plus_or_minus_tz, const int tz_hh, int tz_mm = 0) {
  if (date_mm < 1 || date_mm > 12)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "month must be 1 through 12");
  if (date_dd < 1 || date_dd > 31)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "day must be 1 through 31");
  if (date_yy < 0 || date_yy > 99)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "year must be 0 through 99");
  if (time_hh < 0 || time_hh > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hour must be 0 through 23");
  if (time_mm < 0 || time_mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (time_ss < 0 || time_ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");

  if (plus_or_minus_tz != '+' && plus_or_minus_tz != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "plus_or_minus_tz must be '+' or '-'");
  if (tz_hh < 0 || tz_hh > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hour offset must be 0 through 23");
  if (tz_mm != 0 && tz_mm != 30)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes offset must be 0 or 30");

  return fmt::format(
      ":SMTI{0:#02d}/{1:#02d}/"
      "{2:#02d}&{3:#02d}:{4:#02d}:{5:#02d}&{6}{7:#02d}:{8:#02d}#",
      date_mm, date_dd, date_yy, time_hh, time_mm, time_ss, plus_or_minus_tz,
      tz_hh, tz_mm);
}

// Should return "MM/DD/YY&HH:MM:SS&sHH:MM#"
const std::string cmd_get_date_and_time_and_tz() { return ":GMTI#"; }

// Should return "HH:MM:SS&sDD*MM:SS#"
const std::string cmd_get_target_ra_and_dec() { return ":GMeq#"; }

// Should return "HH:MM:SS&sDD*MM:SS#"
const std::string cmd_get_current_ra_and_dec() { return ":GMEQ#"; }

// Should return "DDD*MM:SS&sDD*MM:SS#"
const std::string cmd_get_az_and_alt() { return ":GMZA#"; }

  // Should return 1 for success and e+error_code+#
// :SMeqHH:MM:SS&sDD*MM:SS#
const std::string cmd_set_target_ra_and_dec_and_goto(
    const int &ra_hh, const int &ra_mm, const int &ra_ss,
    const char &plus_or_minus_dec, const int &dec_dd, const int &dec_mm,
    const int &dec_ss) {

  if (ra_hh < 0 || ra_hh > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hours must 0 through 23");
  if (ra_mm < 0 || ra_mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ra_ss < 0 || ra_ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");
  if (plus_or_minus_dec != '+' && plus_or_minus_dec != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "plus_or_minus_dec must be '+' or '-'");
  if (dec_dd < 0 || dec_dd > 90)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "degrees must 0 through 90");
  if (dec_mm < 0 || dec_mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (dec_ss < 0 || dec_ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");

  return fmt::format(
      ":SMeq{0:#02d}:{1:#02d}:{2:#02d}&{3}{4:#02d}*{5:#02d}:{6:#02d}#", ra_hh,
      ra_mm, ra_ss, plus_or_minus_dec, dec_dd, dec_mm, dec_ss);
}

// Should return N/A#: Success, e+ error code+#
// :SMMCHH:MM:SS&sDD*MM:SS#
const std::string cmd_set_target_ra_and_dec_and_sync(
    const int &ra_hh, const int &ra_mm, const int &ra_ss,
    const char &plus_or_minus_dec, const int &dec_dd, const int &dec_mm,
    const int &dec_ss) {

  if (ra_hh < 0 || ra_hh > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hours must 0 through 23");
  if (ra_mm < 0 || ra_mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ra_ss < 0 || ra_ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");
  if (plus_or_minus_dec != '+' && plus_or_minus_dec != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "plus_or_minus_dec must be '+' or '-'");
  if (dec_dd < 0 || dec_dd > 90)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "degrees must 0 through 90");
  if (dec_mm < 0 || dec_mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (dec_ss < 0 || dec_ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");

  return fmt::format(
      ":SMMC{0:#02d}:{1:#02d}:{2:#02d}&{3}{4:#02d}*{5:#02d}:{6:#02d}#", ra_hh,
      ra_mm, ra_ss, plus_or_minus_dec, dec_dd, dec_mm, dec_ss);
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

  REQUIRE(cmd_get_act_of_crossing_meridian() == ":GTa#");

  REQUIRE(cmd_sync() == ":CM#");

  REQUIRE(cmd_home_position() == ":hC#");

  REQUIRE(cmd_get_status() == ":GU#");

  REQUIRE(cmd_park() == ":hP#");

  REQUIRE(cmd_set_lat_and_long('+', 14, 44, 51, '-', 12, 32, 0) ==
          ":SMGE+14*44:51&-012*32:00#");

  REQUIRE(cmd_get_lat_and_long() == ":GMGE#");

  REQUIRE(cmd_set_date_time_and_tz(11, 4, 24, 5, 1, 0, '-', 5) ==
          ":SMTI11/04/24&05:01:00&-05:00#");

  REQUIRE(cmd_get_date_and_time_and_tz() == ":GMTI#");

  REQUIRE(cmd_get_target_ra_and_dec() == ":GMeq#");

  REQUIRE(cmd_get_current_ra_and_dec() == ":GMEQ#");

  REQUIRE(cmd_get_az_and_alt() == ":GMZA#");

  REQUIRE(cmd_set_target_ra_and_dec_and_goto(1, 2, 1, '+', 9, 1, 2) ==
          ":SMeq01:02:01&+09*01:02#");
  REQUIRE(cmd_set_target_ra_and_dec_and_sync(1, 2, 1, '+', 9, 1, 2) ==
          ":SMMC01:02:01&+09*01:02#");
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

  REQUIRE_THROWS_AS(cmd_set_act_of_crossing_meridian(2, 1, '+', 14),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_act_of_crossing_meridian(1, 2, '+', 14),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_act_of_crossing_meridian(1, 1, '?', 14),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_act_of_crossing_meridian(1, 1, '+', 16),
                    alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_lat_and_long('z', 14, 44, 51, '-', 12, 32, 0),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_lat_and_long('-', 14, 44, 51, 'z', 12, 32, 0),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_lat_and_long('+', 91, 44, 51, '-', 12, 32, 0),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_lat_and_long('+', 89, 44, 51, '-', 181, 32, 0),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_lat_and_long('+', 89, 44, 51, '-', 179, 60, 0),
                    alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_date_time_and_tz(13, 4, 24, 5, 1, 0, '-', 5),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_date_time_and_tz(11, 41, 24, 5, 1, 0, '-', 5),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_date_time_and_tz(11, 1, 124, 5, 1, 0, '-', 5),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_date_time_and_tz(11, 1, 24, 5, 1, 0, 'z', 5),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_date_time_and_tz(11, 1, 24, 5, 1, 0, '-', 5, 45),
                    alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_target_ra_and_dec_and_goto(24, 2, 1, '+', 9, 1, 2),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_target_ra_and_dec_and_goto(23, 60, 1, '+', 9, 1, 2),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_target_ra_and_dec_and_goto(23, 2, 60, '+', 9, 1, 2),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_target_ra_and_dec_and_goto(23, 2, 59, 'z', 9, 1, 2),
                    alpaca_exception);
  REQUIRE_THROWS_AS(
      cmd_set_target_ra_and_dec_and_goto(23, 2, 59, '+', 91, 1, 2),
      alpaca_exception);
  REQUIRE_THROWS_AS(
      cmd_set_target_ra_and_dec_and_goto(23, 2, 59, '+', 89, 60, 2),
      alpaca_exception);
  REQUIRE_THROWS_AS(
      cmd_set_target_ra_and_dec_and_goto(23, 2, 59, '+', 89, 59, 60),
      alpaca_exception);

  REQUIRE_THROWS_AS(cmd_set_target_ra_and_dec_and_sync(24, 2, 1, '+', 9, 1, 2),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_target_ra_and_dec_and_sync(23, 60, 1, '+', 9, 1, 2),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_target_ra_and_dec_and_sync(23, 2, 60, '+', 9, 1, 2),
                    alpaca_exception);
  REQUIRE_THROWS_AS(cmd_set_target_ra_and_dec_and_sync(23, 2, 59, 'z', 9, 1, 2),
                    alpaca_exception);
  REQUIRE_THROWS_AS(
      cmd_set_target_ra_and_dec_and_sync(23, 2, 59, '+', 91, 1, 2),
      alpaca_exception);
  REQUIRE_THROWS_AS(
      cmd_set_target_ra_and_dec_and_sync(23, 2, 59, '+', 89, 60, 2),
      alpaca_exception);
  REQUIRE_THROWS_AS(
      cmd_set_target_ra_and_dec_and_sync(23, 2, 59, '+', 89, 59, 60),
      alpaca_exception);
}
namespace zwoc = zwo_commands;

std::string zwo_am5_telescope::unique_id() { return ""; }

bool zwo_am5_telescope::connected() { return _connected; }



int zwo_am5_telescope::set_connected(bool connected) {
  // covers already connected with connect request and
  // not connected with disconnect request
  if (_connected && connected)
    return 0;

  if (connected) {
    try {
      printf("Opening serial device\n");
      spdlog::debug("Opening serial device at {0}", _serial_device_path);
      _serial_port.open(_serial_device_path);
      _serial_port.set_option(asio::serial_port_base::baud_rate(9600));
      _serial_port.set_option(asio::serial_port_base::character_size(8));
      _serial_port.set_option(asio::serial_port_base::flow_control(
          asio::serial_port_base::flow_control::none));
      _serial_port.set_option(
          asio::serial_port_base::parity(asio::serial_port_base::parity::none));
      _serial_port.set_option(asio::serial_port_base::stop_bits(
          asio::serial_port_base::stop_bits::one));

      char buf[512] = {0};

      _serial_port.write_some(asio::buffer(zwoc::cmd_get_status()));
      _serial_port.read_some(asio::buffer(buf));

      spdlog::debug("mount returned {0}", buf);
      _connected = true;
      return 0;
    } catch (asio::system_error &e) {
      throw alpaca_exception(
          alpaca_exception::DRIVER_ERROR,
          fmt::format(
              "Problem opening serial connection at {0} with error: {1}",
              _serial_device_path, e.what()));
    }
  }
  return -1;
}

TEST_CASE("Serial connection attempt",
          "[set_connected]") {
  spdlog::set_level(spdlog::level::debug);
  zwo_am5_telescope telescope;
  SECTION("Invalid serial device path") {
    telescope.set_serial_device("/dev/ttyARGGWTF");
    REQUIRE_THROWS_AS(telescope.set_connected(true), alpaca_exception);
  }

  SECTION("Valid serial device path") {
    telescope.set_serial_device("/dev/ttyACM0");
    telescope.set_connected(true);
    REQUIRE(telescope.connected());
  }
}

zwo_am5_telescope::zwo_am5_telescope() : _io_context(1), _serial_port(_io_context) {
  // _io_service = std::make_unique<asio::io_service>();
  // _serial_port = std::make_unique<asio::serial_port>(_io_service.get());
};

zwo_am5_telescope::~zwo_am5_telescope(){};

void zwo_am5_telescope::throw_if_not_connected() {
  if ( !_connected )
    throw alpaca_exception(alpaca_exception::NOT_CONNECTED, "Mount not connected");
}

std::string zwo_am5_telescope::send_command_to_mount(const std::string &cmd) {
  char buf[512] = {0};
  _serial_port.write_some(asio::buffer(cmd));
  _serial_port.read_some(asio::buffer(buf));
  return std::string(buf);
}

uint32_t zwo_am5_telescope::interface_version() { return 3; }

std::string zwo_am5_telescope::driver_version() { return "v0.1"; }

std::string zwo_am5_telescope::description() {
  return "ZWO AM5 Telescope Driver";
}

std::string zwo_am5_telescope::driverinfo() {
  return "ZWO AM5 Telescop Driver Information";
}

std::string zwo_am5_telescope::name() { return "ZWO AM5 Mount"; };

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
  spdlog::trace("getting tracking rate from mount");
  auto result = send_command_to_mount(zwoc::cmd_get_tracking_rate());
  spdlog::trace("mount returned: {0}", result[0]);

  return drive_rate_enum::sidereal;
}

TEST_CASE("Get tracking rate", "[tracking_rate]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);
  telescope.tracking_rate();
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

int zwo_am5_telescope::find_home() {
  throw_if_not_connected();
  send_command_to_mount(zwo_commands::cmd_home_position());
  return 0;
}

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

int zwo_am5_telescope::set_serial_device(
    const std::string &serial_device_path) {
  _serial_device_path = serial_device_path;
  return 0;
}

std::string zwo_am5_telescope::get_serial_device_path() {
  return _serial_device_path;
}
