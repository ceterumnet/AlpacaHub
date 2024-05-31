#include "zwo_am5_telescope.hpp"
#include "asio/io_context.hpp"
#include "common/alpaca_exception.hpp"
#include "common/alpaca_hub_serial.hpp"
#include "spdlog/fmt/bundled/format.h"
#include "spdlog/spdlog.h"
#include <asio/steady_timer.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>
#include <regex>
#include <stdexcept>
#include <thread>
#include <vector>

auto format_as(pier_side_enum s) { return fmt::underlying(s); }

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

// I've implemented these with std::regex initially
// I'm not a huge fan of how I've done this, but it should
// work pretty well and be "safe"
namespace zwo_responses {

// 1#, 0# or e+error_code+#
int parse_standard_response(const std::string &resp) {
  std::string response = resp;
  std::regex resp_regex("^e?([0-9])#");
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));
  std::string matched_str = res[1].str();
  return atoi(matched_str.c_str());
}

struct hh_mm_ss {
  int hh;
  int mm;
  int ss;
  double as_decimal() { return hh + (mm / 60.0) + (ss / 3600.0); }
};

hh_mm_ss parse_hh_mm_ss_response(const std::string &resp) {
  hh_mm_ss data;
  std::string response = resp;
  auto expression = R"(^([0-9]{2}):([0-9]{2}):([0-9]{2})#)";
  std::regex resp_regex(expression);
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));

  std::string hh_matched_str = res[1].str();
  data.hh = atoi(hh_matched_str.c_str());
  std::string mm_matched_str = res[2].str();
  data.mm = atoi(mm_matched_str.c_str());
  std::string ss_matched_str = res[3].str();
  data.ss = atoi(ss_matched_str.c_str());
  return data;
}

struct dd_mm_ss {
  int dd;
  int mm;
  int ss;

  double as_decimal() { return dd + (mm / 60.0) + (ss / 3600.0); }
};

dd_mm_ss parse_dd_mm_ss_response(const std::string &resp) {
  dd_mm_ss data;
  std::string response = resp;
  auto expression = R"(^([0-9]{2})\*([0-9]{2}):([0-9]{2})#)";
  std::regex resp_regex(expression);
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));
  std::string dd_matched_str = res[1].str();
  data.dd = atoi(dd_matched_str.c_str());
  std::string mm_matched_str = res[2].str();
  data.mm = atoi(mm_matched_str.c_str());
  std::string ss_matched_str = res[3].str();
  data.ss = atoi(ss_matched_str.c_str());
  return data;
}

struct sdd_mm_ss {
  char plus_or_minus;
  int dd;
  int mm;
  int ss;
  double as_decimal() {
    double value = dd + (mm / 60.0) + (ss / 3600.0);
    if (plus_or_minus == '-')
      value = value * -1;
    return value;
  }
};

sdd_mm_ss parse_sdd_mm_ss_response(const std::string &resp) {
  sdd_mm_ss data;
  std::string response = resp;
  auto expression = R"(^([+-])([0-9]{2})\*([0-9]{2}):([0-9]{2})#)";
  std::regex resp_regex(expression);
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));
  std::string s_matched_str = res[1].str();
  data.plus_or_minus = s_matched_str.c_str()[0];
  std::string dd_matched_str = res[2].str();
  data.dd = atoi(dd_matched_str.c_str());
  std::string mm_matched_str = res[3].str();
  data.mm = atoi(mm_matched_str.c_str());
  std::string ss_matched_str = res[4].str();
  data.ss = atoi(ss_matched_str.c_str());
  return data;
}

struct mm_dd_yy {
  int mm;
  int dd;
  int yy;
};

mm_dd_yy parse_mm_dd_yy_response(const std::string &resp) {
  mm_dd_yy data;
  std::string response = resp;
  auto expression = R"(^([0-9]{2})/([0-9]{2})/([0-9]{2})#)";
  std::regex resp_regex(expression);
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));
  std::string mm_matched_str = res[1].str();
  data.mm = atoi(mm_matched_str.c_str());
  std::string dd_matched_str = res[2].str();
  data.dd = atoi(dd_matched_str.c_str());
  std::string yy_matched_str = res[3].str();
  data.yy = atoi(yy_matched_str.c_str());
  return data;
}

struct shh_mm {
  char plus_or_minus;
  int hh;
  int mm;
};

shh_mm parse_shh_mm_response(const std::string &resp) {
  shh_mm data;
  std::string response = resp;
  auto expression = R"(^([+-]{1})([0-9]{2}):([0-9]{2})#)";
  std::regex resp_regex(expression);
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));
  std::string s_matched_str = res[1].str();
  data.plus_or_minus = s_matched_str.c_str()[0];
  std::string hh_matched_str = res[2].str();
  data.hh = atoi(hh_matched_str.c_str());
  std::string mm_matched_str = res[3].str();
  data.mm = atoi(mm_matched_str.c_str());
  return data;
}

struct sdd_mm {
  char plus_or_minus;
  int dd;
  int mm;
};

sdd_mm parse_sdd_mm_response(const std::string &resp) {
  sdd_mm data;
  std::string response = resp;
  auto expression = R"(^([+-]{1})([0-9]{2})\*([0-9]{2})#)";
  std::regex resp_regex(expression);
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));
  std::string s_matched_str = res[1].str();
  data.plus_or_minus = s_matched_str.c_str()[0];
  std::string dd_matched_str = res[2].str();
  data.dd = atoi(dd_matched_str.c_str());
  std::string mm_matched_str = res[3].str();
  data.mm = atoi(mm_matched_str.c_str());
  return data;
}

struct sddd_mm {
  char plus_or_minus;
  int ddd;
  int mm;
};

sddd_mm parse_sddd_mm_response(const std::string &resp) {
  sddd_mm data;
  std::string response = resp;
  auto expression = R"(^([+-]{1})([0-9]{3})\*([0-9]{2})#)";
  std::regex resp_regex(expression);
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));
  std::string s_matched_str = res[1].str();
  data.plus_or_minus = s_matched_str.c_str()[0];
  std::string ddd_matched_str = res[2].str();
  data.ddd = atoi(ddd_matched_str.c_str());
  std::string mm_matched_str = res[3].str();
  data.mm = atoi(mm_matched_str.c_str());
  return data;
}

std::vector<std::string> split_on(const std::string &string_to_split,
                                  const std::string &separator) {
  std::string copy_of_str = string_to_split;
  std::vector<std::string> results;
  size_t pos = 0;
  while (pos != std::string::npos) {
    pos = copy_of_str.find(separator);
    int end_of_chunk = pos;
    std::string chunk;
    if (end_of_chunk != std::string::npos)
      chunk = copy_of_str.substr(0, pos);
    else
      chunk = copy_of_str;
    results.push_back(chunk);
    copy_of_str = copy_of_str.substr(pos + 1, copy_of_str.size());
  }

  return results;
}

struct sdd_mm_sddd_mm {
  sdd_mm sdd_mm_data;
  sddd_mm sddd_mm_data;
};

sdd_mm_sddd_mm parse_sdd_mm_and_sddd_mm_response(const std::string &resp) {
  sdd_mm_sddd_mm data;
  std::string sdd_mm_resp_str = split_on(resp, "&")[0];
  sdd_mm_resp_str.append("#");
  std::string sddd_mm_resp_str = split_on(resp, "&")[1];
  data.sdd_mm_data = parse_sdd_mm_response(sdd_mm_resp_str);
  data.sddd_mm_data = parse_sddd_mm_response(sddd_mm_resp_str);
  return data;
}

struct ddd_mm_ss {
  int ddd;
  int mm;
  int ss;
  double as_decimal() {
    double value = ddd + (mm / 60.0) + (ss / 3600.0);
    return value;
  }

  ddd_mm_ss(){};
  // CTOR to support easy initialization
  ddd_mm_ss(const double &val) {
    ddd = val;
    mm = (val - ddd) * 60.0;
    ss = (val - ddd - (mm / 60.0)) * 3600.0;
  }

  // format_as()
};

TEST_CASE("Construction of ddd_mm_ss from double",
          "[zwo_responses_ddd_mm_ss_ctor]") {
  using namespace zwo_responses;
  double val = 100.55;
  ddd_mm_ss converted(val);
  spdlog::info("converted: {0} to {1:#03d}*{2:#02d}:{3:#02d}", val,
               converted.ddd, converted.mm, converted.ss);

  spdlog::info("using formatter converted: {0} to {1}", val, converted);
}

ddd_mm_ss parse_ddd_mm_ss_response(const std::string &resp) {
  ddd_mm_ss data;
  std::string response = resp;
  auto expression = R"(^([0-9]{3})\*([0-9]{2}):([0-9]{2})#)";
  std::regex resp_regex(expression);
  std::match_results<std::string::iterator> res;
  auto is_matched =
      std::regex_search(response.begin(), response.end(), res, resp_regex);
  if (!is_matched)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("problem parsing response {0}", resp));
  std::string ddd_matched_str = res[1].str();
  data.ddd = atoi(ddd_matched_str.c_str());
  std::string mm_matched_str = res[2].str();
  data.mm = atoi(mm_matched_str.c_str());
  std::string ss_matched_str = res[3].str();
  data.ss = atoi(ss_matched_str.c_str());

  return data;
}

}; // namespace zwo_responses

// namespace zwo_responses
namespace zwor = zwo_responses;

template <> struct fmt::formatter<zwor::hh_mm_ss> : formatter<string_view> {
  auto format(zwor::hh_mm_ss d, format_context &ctx) const {
    return format_to(ctx.out(), "{:#02d}:{:#02d}:{:#02d}", d.hh, d.mm, d.ss);
  };
};

template <> struct fmt::formatter<zwor::dd_mm_ss> : formatter<string_view> {
  auto format(zwor::dd_mm_ss d, format_context &ctx) const {
    return format_to(ctx.out(), "{:#02d}*{:#02d}:{:#02d}", d.dd, d.mm, d.ss);
  };
};

template <> struct fmt::formatter<zwor::sdd_mm_ss> : formatter<string_view> {
  auto format(zwor::sdd_mm_ss d, format_context &ctx) const {
    return format_to(ctx.out(), "{}{:#02d}*{:#02d}:{:#02d}", d.plus_or_minus,
                     d.dd, d.mm, d.ss);
  };
};

template <> struct fmt::formatter<zwor::mm_dd_yy> : formatter<string_view> {
  auto format(zwor::mm_dd_yy d, format_context &ctx) const {
    return format_to(ctx.out(), "{:#02d}/{:#02d}/{:#02d}", d.mm, d.dd, d.yy);
  };
};

template <> struct fmt::formatter<zwor::shh_mm> : formatter<string_view> {
  auto format(zwor::shh_mm d, format_context &ctx) const {
    return format_to(ctx.out(), "{}{:#02d}:{:#02d}", d.plus_or_minus, d.hh,
                     d.mm);
  };
};

template <> struct fmt::formatter<zwor::sdd_mm> : formatter<string_view> {
  auto format(zwor::sdd_mm d, format_context &ctx) const {
    return format_to(ctx.out(), "{}{:#02d}:{:#02d}", d.plus_or_minus, d.dd,
                     d.mm);
  };
};

template <> struct fmt::formatter<zwor::sddd_mm> : formatter<string_view> {
  auto format(zwor::sddd_mm d, format_context &ctx) const {
    return format_to(ctx.out(), "{}{:#03d}:{:#02d}", d.plus_or_minus, d.ddd,
                     d.mm);
  };
};

template <> struct fmt::formatter<zwor::ddd_mm_ss> : formatter<string_view> {
  auto format(zwor::ddd_mm_ss d, format_context &ctx) const {
    return format_to(ctx.out(), "{:#03d}*{:#02d}:{:#02d}", d.ddd, d.mm, d.ss);
  };
};

TEST_CASE("Split On", "[split_on]") {
  using namespace zwo_responses;
  spdlog::set_level(spdlog::level::trace);
  REQUIRE(split_on("foo&bar", "&").size() == 2);
  REQUIRE(split_on("foo&bar", "&")[0] == "foo");
  REQUIRE(split_on("foo&bar", "&")[1] == "bar");
  REQUIRE(split_on("foo&bar&baz", "&")[1] == "bar");
  REQUIRE(split_on("foo&bar&baz", "&")[2] == "baz");
  REQUIRE(split_on("foobar", "&").size() == 1);
}

TEST_CASE("ZWO single value responses", "[zwo_responses_single_value]") {
  using namespace zwo_responses;
  REQUIRE(parse_standard_response("1#") == 1);
  REQUIRE(parse_standard_response("0#") == 0);
  REQUIRE(parse_standard_response("e2#") == 2);
  REQUIRE_THROWS_AS(parse_standard_response("e2"), alpaca_exception);
}

TEST_CASE("ZWO hours, minutes, seconds responses", "[zwo_responses_time]") {
  using namespace zwo_responses;
  REQUIRE(parse_hh_mm_ss_response("12:10:34#").hh == 12);
  REQUIRE(parse_hh_mm_ss_response("12:10:34#").mm == 10);
  REQUIRE(parse_hh_mm_ss_response("12:10:34#").ss == 34);
  REQUIRE(parse_hh_mm_ss_response("02:10:34#").hh == 2);
  REQUIRE(parse_hh_mm_ss_response("02:10:04#").ss == 4);

  REQUIRE_THROWS_AS(parse_hh_mm_ss_response("02:99:123#"), alpaca_exception);
}

TEST_CASE("ZWO degrees, minutes, seconds responses", "[zwo_responses_dec]") {
  using namespace zwo_responses;
  REQUIRE(parse_dd_mm_ss_response("12*10:34#").dd == 12);
  REQUIRE(parse_dd_mm_ss_response("12*10:34#").mm == 10);
  REQUIRE(parse_dd_mm_ss_response("12*10:34#").ss == 34);
  REQUIRE(parse_dd_mm_ss_response("02*10:34#").dd == 2);
  REQUIRE(parse_dd_mm_ss_response("02*10:04#").ss == 4);
  REQUIRE_THROWS_AS(parse_dd_mm_ss_response("02*99:123#"), alpaca_exception);
}

TEST_CASE("ZWO month, day, year responses", "[zwo_responses_date]") {
  using namespace zwo_responses;
  REQUIRE(parse_mm_dd_yy_response("12/10/34#").mm == 12);
  REQUIRE(parse_mm_dd_yy_response("12/10/34#").dd == 10);
  REQUIRE(parse_mm_dd_yy_response("12/10/34#").yy == 34);
  REQUIRE(parse_mm_dd_yy_response("02/10/34#").mm == 2);
  REQUIRE(parse_mm_dd_yy_response("02/10/04#").yy == 4);
  REQUIRE_THROWS_AS(parse_dd_mm_ss_response("02/99/123#"), alpaca_exception);
}

TEST_CASE("ZWO timezone format", "[zwo_responses_tz]") {
  using namespace zwo_responses;
  REQUIRE(parse_shh_mm_response("+05:30#").plus_or_minus == '+');
  REQUIRE(parse_shh_mm_response("+05:30#").hh == 5);
  REQUIRE(parse_shh_mm_response("+05:30#").mm == 30);

  REQUIRE_THROWS_AS(parse_shh_mm_response("+05/30#"), alpaca_exception);
  REQUIRE_THROWS_AS(parse_shh_mm_response("+05:30"), alpaca_exception);
}

TEST_CASE("ZWO latitude format", "[zwo_responses_latitude]") {
  using namespace zwo_responses;
  REQUIRE(parse_sdd_mm_response("+05*30#").plus_or_minus == '+');
  REQUIRE(parse_sdd_mm_response("+05*30#").dd == 5);
  REQUIRE(parse_sdd_mm_response("+05*30#").mm == 30);

  REQUIRE_THROWS_AS(parse_sdd_mm_response("+05*30"), alpaca_exception);
  REQUIRE_THROWS_AS(parse_sdd_mm_response("+05/30#"), alpaca_exception);
}

TEST_CASE("ZWO longitude format", "[zwo_responses_longitude]") {
  using namespace zwo_responses;
  REQUIRE(parse_sddd_mm_response("+105*30#").plus_or_minus == '+');
  REQUIRE(parse_sddd_mm_response("+105*30#").ddd == 105);
  REQUIRE(parse_sddd_mm_response("+005*30#").mm == 30);

  REQUIRE_THROWS_AS(parse_sddd_mm_response("+05*30"), alpaca_exception);
  REQUIRE_THROWS_AS(parse_sddd_mm_response("+05/30#"), alpaca_exception);
}

TEST_CASE("ZWO altitude format", "[zwo_responses_altitude]") {
  using namespace zwo_responses;
  REQUIRE(parse_sdd_mm_ss_response("+34*11:23#").plus_or_minus == '+');
  REQUIRE(parse_sdd_mm_ss_response("+34*11:23#").dd == 34);
  REQUIRE(parse_sdd_mm_ss_response("+34*11:23#").mm == 11);
  REQUIRE(parse_sdd_mm_ss_response("+34*11:23#").ss == 23);
  REQUIRE_THROWS_AS(parse_sdd_mm_ss_response("+34*11:23"), alpaca_exception);
}

TEST_CASE("Degrees, hours, minutes, seconds conversions",
          "[zwo_responses_conversions]") {
  using namespace zwo_responses;

  sdd_mm_ss data{.plus_or_minus = '-', .dd = 81, .mm = 30, .ss = 30};
  REQUIRE(data.as_decimal() < -81.508);
  REQUIRE(data.as_decimal() > -81.509);
}

// TODO: Need to figure out how to have a unique identifier for this
std::string zwo_am5_telescope::unique_id() { return ""; }

bool zwo_am5_telescope::connected() { return _connected; }

int zwo_am5_telescope::set_connected(bool connected) {
  // covers already connected with connect request and
  // not connected with disconnect request
  if (_connected && connected) {
    spdlog::warn(
        "set_connected called with connected:{0} but is already in that state");
    return 0;
  }

  if (connected) {
    try {
      spdlog::debug("Attempting to open serial device at {0}",
                    _serial_device_path);
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
  } else {
    try {
      _serial_port.close();
    } catch (asio::system_error &e) {
      throw alpaca_exception(
          alpaca_exception::DRIVER_ERROR,
          fmt::format(
              "Problem closing serial connection at {0} with error: {1}",
              _serial_device_path, e.what()));
    }
  }
  return -1;
}

TEST_CASE("Serial connection attempt", "[set_connected]") {
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

zwo_am5_telescope::zwo_am5_telescope()
    : _site_longitude(0), _site_latitude(0), _site_elevation(0),
      _aperture_diameter(0), _io_context(1), _serial_port(_io_context){};

zwo_am5_telescope::~zwo_am5_telescope() {
  spdlog::debug("Closing serial connection");
  _serial_port.close();
};

void zwo_am5_telescope::throw_if_not_connected() {
  if (!_connected)
    throw alpaca_exception(alpaca_exception::NOT_CONNECTED,
                           "Mount not connected");
}

std::string zwo_am5_telescope::send_command_to_mount(const std::string &cmd,
                                                     bool read_response) {
  char buf[512] = {0};
  _serial_port.write_some(asio::buffer(cmd));

  std::string rsp;

  if (read_response) {
    _io_context.reset();

    // TODO: we may need to make the read timeout configurable here
    alpaca_hub_serial::blocking_reader reader(_serial_port, 100, _io_context);
    char c;
    while (reader.read_char(c)) {
      rsp += c;
      if (c == '#') {
        break;
      }
    }
  }

  return rsp;
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

// I'm going to hard code this. Based on the development documentation for the
// AM5, it seems that the mount must be rebooted when the mode is switched.
// Given that the use case for this is imaging - alt / az mode doesn't really
// sense.
zwo_am5_telescope::alignment_mode_enum zwo_am5_telescope::alignment_mode() {
  return alignment_mode_enum::polar;
}

double zwo_am5_telescope::altitude() {
  throw_if_not_connected();

  std::string resp = send_command_to_mount(zwoc::cmd_get_altitude());
  zwor::sdd_mm_ss result = zwor::parse_sdd_mm_ss_response(resp);

  return result.as_decimal();
}

double zwo_am5_telescope::aperture_diameter() { return _aperture_diameter; }

int zwo_am5_telescope::set_aperture_diameter(const double &diameter) {
  _aperture_diameter = diameter;
  return 0;
};

bool zwo_am5_telescope::is_slewing() {
  auto resp = send_command_to_mount(zwoc::cmd_get_status());
  auto last_2_chars = resp.substr(resp.size() - 2, resp.size());
  // 4 is moving status
  if (last_2_chars.find("4") != std::string::npos &&
      last_2_chars.find("#") != std::string::npos)
    return true;
  return false;
}

bool zwo_am5_telescope::at_home() {
  throw_if_not_connected();
  std::string resp = send_command_to_mount(zwoc::cmd_get_status());
  if (resp.find('H') != std::string::npos)
    return true;
  return false;
}

// Commenting this out because it moves the scope...
//  - I need to figure out how to run moving tests separately
TEST_CASE("Test mount At home", "[at_home]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);
  spdlog::trace("sending cmd_home_position()");

  // telescope.send_command_to_mount(zwoc::cmd_home_position());
  telescope.find_home();
  using namespace std::chrono_literals;
  // Wait for the mount to home...
  for (int i = 0; i < 30; i++) {
    if (telescope.is_slewing())
      spdlog::debug("slewing...");

    if (telescope.at_home())
      break;

    // spdlog::debug("waiting 1 second for mount to settle");
    std::this_thread::sleep_for(1000ms);
  }
  REQUIRE(telescope.at_home() == true);
}

// TODO: implement
bool zwo_am5_telescope::at_park() {
  throw_if_not_connected();
  return false;
}

// TEST_CASE("Test mount At park", "[at_park]") {
//   spdlog::set_level(spdlog::level::trace);
//   zwo_am5_telescope telescope;
//   telescope.set_serial_device("/dev/ttyACM0");
//   telescope.set_connected(true);
//   spdlog::trace("sending cmd_park()");
//   telescope.send_command_to_mount(zwoc::cmd_park());
//   using namespace std::chrono_literals;
//   // Wait for the mount to home...
//   std::this_thread::sleep_for(1000ms);
//   std::string status_str =
//   telescope.send_command_to_mount(zwoc::cmd_get_status());
//   spdlog::debug("status_str: {0}", status_str);
//   // REQUIRE(telescope.at_home() == true);
// }

// TODO: implement
double zwo_am5_telescope::azimuth() {
  throw_if_not_connected();
  std::string resp = send_command_to_mount(zwoc::cmd_get_azimuth());
  spdlog::trace("raw value returned from mount for cmd_get_azimuth(): {0}",
                resp);
  return zwor::parse_ddd_mm_ss_response(resp).as_decimal();
}

TEST_CASE("Test get azimuth", "[azimuth]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  using namespace std::chrono_literals;
  // Wait for the mount to home...
  double val = telescope.azimuth();
  spdlog::debug("Current azimuth: {0}", val);
  // REQUIRE(telescope.at_home() == true);
}

// AM5 supports finding home
bool zwo_am5_telescope::can_find_home() {
  throw_if_not_connected();
  return true;
}

// AM5 supports park
bool zwo_am5_telescope::can_park() {
  throw_if_not_connected();
  return true;
}

// AM5 supports pulse guiding
bool zwo_am5_telescope::can_pulse_guide() {
  throw_if_not_connected();
  return true;
}

// AM5 supports setting rates
bool zwo_am5_telescope::can_set_declination_rate() {
  throw_if_not_connected();
  return true;
}

// AM5 supports setting guide rates
bool zwo_am5_telescope::can_set_guide_rates() {
  throw_if_not_connected();
  return true;
}

// AM5 doesn't appear to support a custom park
bool zwo_am5_telescope::can_set_park() {
  throw_if_not_connected();
  return false;
}

// I'm not sure this will be supported or not...
//
bool zwo_am5_telescope::can_set_pier_side() {
  throw_if_not_connected();
  return false;
}

// AM5 supports setting rates
bool zwo_am5_telescope::can_set_right_ascension_rate() {
  throw_if_not_connected();
  return true;
}

// AM5 supports turning tracking on and off
bool zwo_am5_telescope::can_set_tracking() {
  throw_if_not_connected();
  return true;
}

bool zwo_am5_telescope::can_slew() {
  throw_if_not_connected();
  return true;
}

// TODO: decide whether or not I'm gonna implement alt az mode
bool zwo_am5_telescope::can_slew_alt_az() {
  throw_if_not_connected();
  return false;
}

// TODO: decide whether or not I'm gonna implement alt az mode
bool zwo_am5_telescope::can_slew_alt_az_async() {
  throw_if_not_connected();
  return false;
}

bool zwo_am5_telescope::can_sync() {
  throw_if_not_connected();
  return true;
}

// I don't believe this is supported according to AM5 documentation
bool zwo_am5_telescope::can_sync_alt_az() {
  throw_if_not_connected();
  return false;
}

// I'm not sure if this means just going to home position or what...
// for now I'm gonna say false
bool zwo_am5_telescope::can_unpark() {
  throw_if_not_connected();
  return false;
}

double zwo_am5_telescope::declination() {
  throw_if_not_connected();
  std::string resp = send_command_to_mount(zwoc::cmd_get_current_dec());
  spdlog::trace("declination returned: {0}", resp);
  auto parsed_resp = zwor::parse_sdd_mm_ss_response(resp);
  return parsed_resp.as_decimal();
}

TEST_CASE("Test get declination", "[declination]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  using namespace std::chrono_literals;
  // Wait for the mount to home...
  double val = telescope.declination();
  spdlog::debug("Current declination: {0}", val);
  // REQUIRE(telescope.at_home() == true);
}

// Returning 0 because we aren't supporting setting a dec rate
double zwo_am5_telescope::declination_rate() {
  throw_if_not_connected();
  return 0;
}

// not supported
int zwo_am5_telescope::set_declination_rate(double) {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Cannot set declination rate");
  return 0;
}

bool zwo_am5_telescope::does_refraction() {
  throw_if_not_connected();
  return false;
}

int zwo_am5_telescope::set_does_refraction(bool) {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Setting refraction not supported");
  return 0;
}

zwo_am5_telescope::equatorial_system_enum
zwo_am5_telescope::equatorial_system() {
  throw_if_not_connected();
  return equatorial_system_enum::j2000;
}

double zwo_am5_telescope::focal_length() {
  throw_if_not_connected();
  return _focal_length;
}

int zwo_am5_telescope::set_focal_length(double focal_length) {
  throw_if_not_connected();
  _focal_length = focal_length;
  return 0;
}

// TODO: clean this up so it is safe
double zwo_am5_telescope::guide_rate_declination() {
  throw_if_not_connected();
  auto resp = send_command_to_mount(zwoc::cmd_get_guide_rate());
  std::string value = resp.substr(0, 3);
  return atof(&value[0]);
}

int zwo_am5_telescope::set_guide_rate_declination(double rate) {
  throw_if_not_connected();
  send_command_to_mount(zwoc::cmd_set_guide_rate(rate));
  return 0;
}

// We just use the same method as declination
double zwo_am5_telescope::guide_rate_ascension() {
  return guide_rate_declination();
}

// We just use the same method as declination
int zwo_am5_telescope::set_guide_rate_ascension(double rate) {
  return set_guide_rate_declination(rate);
}

bool zwo_am5_telescope::is_pulse_guiding() { return 0; }

double zwo_am5_telescope::right_ascension() {
  throw_if_not_connected();
  std::string resp = send_command_to_mount(zwoc::cmd_get_current_ra());
  spdlog::trace("ra returned: {0}", resp);
  auto parsed_resp = zwor::parse_hh_mm_ss_response(resp);
  return parsed_resp.as_decimal();
}

TEST_CASE("Test get right_ascension", "[right_ascension]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  using namespace std::chrono_literals;
  // Wait for the mount to home...
  double val = telescope.right_ascension();
  spdlog::debug("Current right ascension: {0}", val);
  // REQUIRE(telescope.at_home() == true);
}

// I believe this is just 0 as I don't believe the AM5 supports a
// separte RA rate
double zwo_am5_telescope::right_ascension_rate() { return 0; }

int zwo_am5_telescope::set_right_ascension_rate() {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Setting a RA rate is not supported on this mount");
  return 0;
}

// After testing this, I'm a little bit concerned that the
// accuracy of this is a little bit questionable...
pier_side_enum zwo_am5_telescope::side_of_pier() {
  spdlog::trace("side_of_pier invoked");
  std::string resp =
      send_command_to_mount(zwoc::cmd_get_current_cardinal_direction());
  spdlog::trace("cmd_get_current_cardinal_direction() returned {0}", resp);
  if (resp.find("W"))
    return pier_side_enum::west;
  if (resp.find("E"))
    return pier_side_enum::east;
  return pier_side_enum::unknown;
}

TEST_CASE("Test get side_of_pier", "[side_of_pier]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  using namespace std::chrono_literals;
  // Wait for the mount to home...
  pier_side_enum val = telescope.side_of_pier();
  spdlog::debug("Current side of pier: {0}", val);
  // REQUIRE(telescope.at_home() == true);
}

int zwo_am5_telescope::set_side_of_pier(pier_side_enum) {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Setting side of pier not supported on this mount");
  return 0;
}

double zwo_am5_telescope::sidereal_time() {
  throw_if_not_connected();
  auto resp = send_command_to_mount(zwoc::cmd_get_sidereal_time());
  zwor::hh_mm_ss data = zwor::parse_hh_mm_ss_response(resp);
  return data.as_decimal();
}

double zwo_am5_telescope::site_elevation() {
  throw_if_not_connected();
  return _site_elevation;
}

int zwo_am5_telescope::set_site_elevation(double site_elevation) {
  throw_if_not_connected();
  _site_elevation = site_elevation;
  return 0;
}

double zwo_am5_telescope::site_latitude() {
  throw_if_not_connected();
  return _site_latitude;
}

int zwo_am5_telescope::set_site_latitude(double site_latitude) {
  throw_if_not_connected();
  _site_latitude = site_latitude;
  return 0;
}

double zwo_am5_telescope::site_longitude() {
  throw_if_not_connected();
  return _site_longitude;
}

int zwo_am5_telescope::set_site_longitude(double) { return 0; }

bool zwo_am5_telescope::slewing() { return 0; }

double zwo_am5_telescope::slew_settle_time() { return 0; }

int zwo_am5_telescope::set_slew_settle_time(double) { return 0; }

double zwo_am5_telescope::target_declination() { return 0; }

int zwo_am5_telescope::set_target_declination(double) { return 0; }

bool zwo_am5_telescope::tracking() { return 0; }

int zwo_am5_telescope::set_tracking(bool) { return 0; }

zwo_am5_telescope::drive_rate_enum zwo_am5_telescope::tracking_rate() {
  spdlog::set_level(spdlog::level::trace);
  spdlog::trace("getting tracking rate from mount");
  auto result = send_command_to_mount(zwoc::cmd_get_tracking_rate());
  spdlog::trace("mount returned: {0}", result[0]);

  return drive_rate_enum::sidereal;
}

// TODO: finish writing this case
// TEST_CASE("Get tracking rate", "[tracking_rate]") {
//   spdlog::set_level(spdlog::level::trace);
//   zwo_am5_telescope telescope;
//   telescope.set_serial_device("/dev/ttyACM0");
//   telescope.set_connected(true);
//   telescope.tracking_rate();
// }

int zwo_am5_telescope::tracking_rate(drive_rate_enum) { return 0; }

std::vector<zwo_am5_telescope::drive_rate_enum>
zwo_am5_telescope::tracking_rates() {
  return std::vector<drive_rate_enum>();
}
std::string zwo_am5_telescope::utc_time() { return ""; }

int zwo_am5_telescope::set_utc_time(std::string &) { return 0; }

TEST_CASE("Test mount get time", "[get_time]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);
  spdlog::trace("sending cmd_get_time()");
  std::string status_str =
      telescope.send_command_to_mount(zwoc::cmd_get_time());
  // using namespace std::chrono_literals;
  // // Wait for the mount to home...
  // std::this_thread::sleep_for(1000ms);
  spdlog::debug("cmd_get_time() status_str: {0}", status_str);
  // REQUIRE(telescope.at_home() == true);
}

int zwo_am5_telescope::abort_slew() { return 0; }

std::vector<zwo_am5_telescope::axis_rate>
zwo_am5_telescope::axis_rates(telescope_axes_enum) {
  return std::vector<axis_rate>();
}
bool zwo_am5_telescope::can_move_axis(telescope_axes_enum axis) {
  return false;
}

pier_side_enum zwo_am5_telescope::destination_side_of_pier(double ra,
                                                           double dec) {
  return pier_side_enum::unknown;
}

class blocking_telescope_command {
  asio::io_context _io_ctx;
  asio::steady_timer _t;
  zwo_am5_telescope &_telescope;
  bool _is_done_moving;

public:
  blocking_telescope_command(zwo_am5_telescope &telescope)
      : _t(_io_ctx), _telescope(telescope), _is_done_moving(false) {}

  void check_is_slewing() {
    using namespace std::chrono_literals;
    _t.expires_from_now(100ms);
    if (_telescope.is_slewing()) {
      _t.expires_from_now(100ms);
    } else {
      _is_done_moving = true;
      _t.cancel();
    }
  }

  bool is_still_moving() {
    using namespace std::chrono_literals;
    _io_ctx.reset();
    _t.async_wait(
        std::bind(&blocking_telescope_command::check_is_slewing, this));
    _t.expires_from_now(100ms);
    _io_ctx.run();
    return !_is_done_moving;
  }

  void run() {
    while (is_still_moving())
      ;
  }
};

// block until not moving
void zwo_am5_telescope::block_while_moving() {
  using namespace std::chrono_literals;
  spdlog::trace("block_while_moving invoked");
  blocking_telescope_command cmd(*this);
  // while (cmd.is_still_moving());
  cmd.run();
}

int zwo_am5_telescope::find_home() {
  using namespace std::chrono_literals;
  throw_if_not_connected();
  send_command_to_mount(zwo_commands::cmd_home_position());
  block_while_moving();
  spdlog::debug("initial timer has ended");
  // block_while_moving();
  return 0;
}

// I'm making an assumption that east/west is mapped to RA...need to test
int zwo_am5_telescope::move_axis(telescope_axes_enum axis, double rate) {
  switch(axis) {
  // RA
  case telescope_axes_enum::primary:
    send_command_to_mount(zwoc::cmd_set_moving_speed_precise(std::abs(rate)), false);
    if(rate > 0)
      send_command_to_mount(zwoc::cmd_move_towards_east(), false);
    else if(rate < 0)
      send_command_to_mount(zwoc::cmd_move_towards_west(), false);
    else {
      send_command_to_mount(zwoc::cmd_stop_moving_towards_east(), false);
      send_command_to_mount(zwoc::cmd_stop_moving_towards_west(), false);
    }
    break;
  // Dec
  case telescope_axes_enum::secondary:
    send_command_to_mount(zwoc::cmd_set_moving_speed_precise(std::abs(rate)));
    if(rate > 0)
      send_command_to_mount(zwoc::cmd_move_towards_north(), false);
    else if(rate < 0)
      send_command_to_mount(zwoc::cmd_move_towards_south(), false);
    else {
      send_command_to_mount(zwoc::cmd_stop_moving_towards_south(), false);
      send_command_to_mount(zwoc::cmd_stop_moving_towards_north(), false);
    }
    break;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE, "unrecognized axis");
  }
  return 0;
}

TEST_CASE("Test east/west/north/souch", "[move_axis]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  using namespace std::chrono_literals;
  telescope.send_command_to_mount(zwoc::cmd_set_moving_speed_precise(1440.0),
                        false);
  telescope.send_command_to_mount(zwoc::cmd_move_towards_east(), false);
  spdlog::debug("moving east");
  std::this_thread::sleep_for(2s);
  spdlog::debug("moving stopping");
  telescope.send_command_to_mount(zwoc::cmd_stop_moving_towards_east(), false);
}

int zwo_am5_telescope::park() {
  throw_if_not_connected();
  spdlog::trace("sending cmd_park()");
  send_command_to_mount(zwoc::cmd_park());
  block_while_moving();
  return 0;
}

int zwo_am5_telescope::pulse_guide(guide_direction_enum direction,
                                   uint32_t duration_ms) {
  return 0;
}

int zwo_am5_telescope::set_park() {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "This mount does not support setting park position");
  return 0;
}

// TODO: implement
int zwo_am5_telescope::slew_to_alt_az(double alt, double az) {
  throw_if_not_connected();
  return 0;
}

// TODO: implement
int zwo_am5_telescope::slew_to_alt_az_async(double alt, double az) {
  throw_if_not_connected();
  return 0;
}

// TODO: implement
int zwo_am5_telescope::slew_to_coordinates(double ra, double dec) {
  throw_if_not_connected();
  char plus_or_minus = '+';
  if (dec < 0)
    plus_or_minus = '-';
  // TODO: implement from_double on structs
  // int ra_hh = 0;
  // int ra_mm = 0;
  // int ra_ss = 0;

  // send_command_to_mount(zwoc::cmd_set_target_ra_and_dec_and_goto())
  return 0;
}

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
