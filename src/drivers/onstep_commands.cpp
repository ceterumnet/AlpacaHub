#include <limits>
#include "onstep_commands.hpp"
namespace onstep_commands {

const std::string cmd_get_version() { return ":GV#"; }

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
                                   const int &h_offset, int m_offset) {
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

// Response should be "sDD*MM:SS#"
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

// Response should be "sDD:MM:SS#"
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
const std::string cmd_set_tracking_rate_to_sidereal() { return ":TQ#"; }

// Response should be none
const std::string cmd_set_tracking_rate_to_solar() { return ":TS#"; }

// Response should be none
const std::string cmd_set_tracking_rate_to_lunar() { return ":TL#"; }

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

// This is a response with variable length
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
    const char &plus_or_minus_tz, const int tz_hh, int tz_mm) {
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

}; // namespace onstep_commands

// I've implemented these with std::regex initially
// I'm not a huge fan of how I've done this, but it should
// work pretty well and be "safe"
namespace onstep_responses {

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

double hh_mm_ss::as_decimal() { return hh + (mm / 60.0) + (ss / 3600.0); }

hh_mm_ss::hh_mm_ss() : hh(0), mm(0), ss(0){};
// sdd_mm_ss(std::initializer_list<sdd_mm_ss>) {};
// CTOR to support easy initialization
hh_mm_ss::hh_mm_ss(const double &val) {
  hh = val;
  mm = (val - hh) * 60.0;
  ss = std::round((val - hh - (mm / 60.0)) * 3600.0);

  if (ss == 60) {
    mm += 1;
    ss = 0;
  }

  if (mm == 60) {
    hh += 1;
    mm = 0;
  }

}

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

double dd_mm_ss::as_decimal() { return dd + (mm / 60.0) + (ss / 3600.0); }

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

double sdd_mm_ss::as_decimal() {
  double value = dd + (mm / 60.0) + (ss / 3600.0);
  if (plus_or_minus == '-')
    value = value * -1;
  return value;
}

sdd_mm_ss::sdd_mm_ss() : plus_or_minus('+'), dd(0), mm(0), ss(0){};

// sdd_mm_ss(std::initializer_list<sdd_mm_ss>) {};
// CTOR to support easy initialization
sdd_mm_ss::sdd_mm_ss(const double &val) {
  // 40.33333333333333
  // 40.333333333333336
  double abs_val = std::abs(val);
  dd = abs_val;
  mm = (abs_val - dd) * 60.0;
  ss = std::round((abs_val - dd - (mm / 60.0)) * 3600.0);

  if (ss == 60) {
    mm += 1;
    ss = 0;
  }

  if (mm == 60) {
    dd += 1;
    mm = 0;
  }

  plus_or_minus = '+';
  if (val < 0) {
    plus_or_minus = '-';
  }
}

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

double sddd_mm_ss::as_decimal() {
  double value = ddd + (mm / 60.0) + (ss / 3600.0);
  if (plus_or_minus == '-')
    value = value * -1;
  return value;
}

sddd_mm_ss::sddd_mm_ss() : plus_or_minus('+'), ddd(0), mm(0), ss(0){};

// sdd_mm_ss(std::initializer_list<sdd_mm_ss>) {};
// CTOR to support easy initialization
sddd_mm_ss::sddd_mm_ss(const double &val) {
  double abs_val = std::abs(val);
  ddd = abs_val;
  mm = (abs_val - ddd) * 60.0;
  ss = std::round((abs_val - ddd - (mm / 60.0)) * 3600.0);
  if (ss == 60) {
    mm += 1;
    ss = 0;
  }

  if (mm == 60) {
    ddd += 1;
    mm = 0;
  }

  plus_or_minus = '+';
  if (val < 0) {
    plus_or_minus = '-';
  }
}

// -098*00:00#
sddd_mm_ss parse_sddd_mm_ss_response(const std::string &resp) {
  sddd_mm_ss data;
  std::string response = resp;
  auto expression = R"(^([+-])([0-9]{3})\*([0-9]{2}):([0-9]{2})#)";
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
  std::string ss_matched_str = res[4].str();
  data.ss = atoi(ss_matched_str.c_str());
  return data;
}

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

double sdd_mm::as_decimal() {
  double value = dd + (mm / 60.0);
  if (plus_or_minus == '-')
    value = value * -1;
  return value;
}

sdd_mm::sdd_mm() : plus_or_minus('+'), dd(0), mm(0){};

// sdd_mm_ss(std::initializer_list<sdd_mm_ss>) {};
// CTOR to support easy initialization
sdd_mm::sdd_mm(const double &val) {
  double abs_val = std::abs(val);
  dd = abs_val;
  mm = std::round((abs_val - dd) * 60.0);

  if (mm == 60) {
    dd += 1;
    mm = 0;
  }

  plus_or_minus = '+';
  if (val < 0) {
    plus_or_minus = '-';
  }
}

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

double sddd_mm::as_decimal() {
  double value = ddd + (mm / 60.0);
  if (plus_or_minus == '-')
    value = value * -1;
  return value;
}

sddd_mm::sddd_mm() : plus_or_minus('+'), ddd(0), mm(0){};

// sdd_mm_ss(std::initializer_list<sdd_mm_ss>) {};
// CTOR to support easy initialization
sddd_mm::sddd_mm(const double &val) {
  double abs_val = std::abs(val);
  ddd = abs_val;
  mm = std::round((abs_val - ddd) * 60.0);

  if (mm == 60) {
    ddd += 1;
    mm = 0;
  }

  plus_or_minus = '+';
  if (val < 0) {
    plus_or_minus = '-';
  }
}

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

sdd_mm_sddd_mm parse_sdd_mm_and_sddd_mm_response(const std::string &resp) {
  sdd_mm_sddd_mm data;
  std::string sdd_mm_resp_str = split_on(resp, "&")[0];
  sdd_mm_resp_str.append("#");
  std::string sddd_mm_resp_str = split_on(resp, "&")[1];
  data.sdd_mm_data = parse_sdd_mm_response(sdd_mm_resp_str);
  data.sddd_mm_data = parse_sddd_mm_response(sddd_mm_resp_str);
  return data;
}

double ddd_mm_ss::as_decimal() {
  double value = ddd + (mm / 60.0) + (ss / 3600.0);
  return value;
}

ddd_mm_ss::ddd_mm_ss(){};

// CTOR to support easy initialization
ddd_mm_ss::ddd_mm_ss(const double &val) {
  ddd = val;
  mm = (val - ddd) * 60.0;
  ss = std::round((val - ddd - (mm / 60.0)) * 3600.0);
  if(ss == 60) {
    mm += 1;
    ss = 0;
  }

  if (mm == 60) {
    ddd += 1;
    mm = 0;
  }
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

}; // namespace onstep_responses
