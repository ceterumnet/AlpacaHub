#include <limits>
#include "onstep_commands.hpp"
namespace onstep_commands {

const std::string cmd_get_version() { return ":GVP#"; }

// This doesn't respond with anything
// Removing undocumented command
// const std::string cmd_switch_to_eq_mode() { return ":AP#"; }

// This doesn't respond with anything
// Removing undocumented command
// const std::string cmd_switch_to_alt_az_mode() { return ":AA#"; }

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

// NEW: Get time in 12hr format - response should be "HH:MM:SS#"
const std::string cmd_get_time_12h() { return ":Ga#"; }

// response should be "HH:MM:SS#"
const std::string cmd_get_sidereal_time() { return ":GS#"; }

// NEW: Set sidereal time - response should be 1 for success and 0 for failure
const std::string cmd_set_sidereal_time(const int &hh, const int &mm, const int &ss) {
  if (hh < 0 || hh > 23)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "hour must be 0 through 23");
  if (mm < 0 || mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ss < 0 || ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");

  return fmt::format(":SS{0:#02d}:{1:#02d}:{2:#02d}#", hh, mm, ss);
}

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

// NEW: Set site 0 name - response should be 0 or 1
const std::string cmd_set_site_0_name(const std::string &site_name) {
  return fmt::format(":SM{}#", site_name);
}

// NEW: Set site 1 name - response should be 0 or 1
const std::string cmd_set_site_1_name(const std::string &site_name) {
  return fmt::format(":SN{}#", site_name);
}

// NEW: Set site 2 name - response should be 0 or 1
const std::string cmd_set_site_2_name(const std::string &site_name) {
  return fmt::format(":SO{}#", site_name);
}

// NEW: Set site 3 name - response should be 0 or 1
const std::string cmd_set_site_3_name(const std::string &site_name) {
  return fmt::format(":SP{}#", site_name);
}

// NEW: Get site 0 name - response should be "sss...#"
const std::string cmd_get_site_0_name() { return ":GM#"; }

// NEW: Get site 1 name - response should be "sss...#"
const std::string cmd_get_site_1_name() { return ":GN#"; }

// NEW: Get site 2 name - response should be "sss...#"
const std::string cmd_get_site_2_name() { return ":GO#"; }

// NEW: Get site 3 name - response should be "sss...#"
const std::string cmd_get_site_3_name() { return ":GP#"; }

// NEW: Select site n (0-3) - response should be none
const std::string cmd_select_site(const int &site_number) {
  if (site_number < 0 || site_number > 3)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "site number must be 0 through 3");
  return fmt::format(":W{}#", site_number);
}

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

// NEW: Set target Azm - response should be 1 for success or 0 for failure
const std::string cmd_set_target_azm(const int &ddd, const int &mm, const int &ss) {
  if (ddd < 0 || ddd > 359)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "degrees must be 0 through 359");
  if (mm < 0 || mm > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "minutes must 0 through 59");
  if (ss < 0 || ss > 59)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "seconds must 0 through 59");

  return fmt::format(":Sz{0:#03d}:{1:#02d}:{2:#02d}#", ddd, mm, ss);
}

// NEW: Set target Alt - response should be 1 for success or 0 for failure
const std::string cmd_set_target_alt(const char &plus_or_minus, const int &dd,
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

  return fmt::format(":Sa{0}{1:#02d}:{2:#02d}:{3:#02d}#", plus_or_minus, dd, mm, ss);
}

// NEW: Set horizon limit - response should be 1 for success or 0 for failure
const std::string cmd_set_horizon_limit(const char &plus_or_minus, const int &dd) {
  if (plus_or_minus != '+' && plus_or_minus != '-')
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "first param must be '+' or '-'");
  if (dd < 0 || dd > 30)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "degrees must 0 through 30");

  return fmt::format(":Sh{0}{1:#02d}#", plus_or_minus, dd);
}

// NEW: Get horizon limit - response should be "sDD#"
const std::string cmd_get_horizon_limit() { return ":Gh#"; }

// NEW: Set overhead limit - response should be 1 for success or 0 for failure
const std::string cmd_set_overhead_limit(const int &dd) {
  if (dd < 60 || dd > 90)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "degrees must 60 through 90");

  return fmt::format(":So{0:#02d}#", dd);
}

// NEW: Get overhead limit - response should be "DD#"
const std::string cmd_get_overhead_limit() { return ":Go#"; }

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

// NEW: Move telescope to current Hor target - response should be e
const std::string cmd_goto_horizontal() { return ":MA#"; }

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

// NEW: Set sidereal rate RA - response should be 0 or 1
const std::string cmd_set_sidereal_rate_ra(const double &rate) {
  if (rate < 0.0 || rate > 100.0)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "rate must be between 0.0 and 100.0");
  return fmt::format(":ST{0:#.5f}#", rate);
}

// NEW: Get sidereal rate RA - response should be "dd.ddddd#"
const std::string cmd_get_sidereal_rate_ra() { return ":GT#"; }

// NEW: Track sidereal rate reset - response should be none
const std::string cmd_track_sidereal_rate_reset() { return ":TR#"; }

// NEW: Track rate increase 0.02Hz - response should be none
const std::string cmd_track_rate_increase() { return ":T+#"; }

// NEW: Track rate decrease 0.02Hz - response should be none
const std::string cmd_track_rate_decrease() { return ":T-#"; }

// NEW: Track king rate RA - response should be none
const std::string cmd_set_tracking_rate_to_king() { return ":TK#"; }

// Response will be one of 1# 2# or 3# corresponding with the tracking rate enum
const std::string cmd_get_tracking_rate() { return ":GT#"; }

// Returns 1 for success or 0 for failure
const std::string cmd_start_tracking() { return ":Te#"; }

// Returns 1 for success or 0 for failure
const std::string cmd_stop_tracking() { return ":Td#"; }

// NEW: Refraction rate tracking - response should be 0 or 1
const std::string cmd_enable_refraction_rate_tracking() { return ":Tr#"; }

// NEW: No refraction rate tracking - response should be 0 or 1
const std::string cmd_disable_refraction_rate_tracking() { return ":Tn#"; }

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

// NEW: Restore parked telescope to operation - response should be 0 or 1
const std::string cmd_restore_parked_telescope() { return ":hR#"; }

// NEW: Set home (CWD) - response should be none
const std::string cmd_set_home() { return ":hF#"; }

// Response should be 1 for success or 0 for failure
// :SMGEsDD*MM:SS&sDDD*MM:SS#
// Removing undocumented command
// const std::string cmd_set_lat_and_long(const char &plus_or_minus_lat,
//                                       const int &lat_dd, const int lat_mm,
//                                       const int &lat_ss,
//                                       const char &plus_or_minus_long,
//                                       const int &long_ddd, const int &long_mm,
//                                       const int &long_ss) {
//   ... [code implementation] ...
// }

// Removing undocumented command
// const std::string cmd_get_lat_and_long() { return ":GMGE#"; }

// Removing undocumented command
// const std::string cmd_set_date_time_and_tz(
//    const int &date_mm, const int &date_dd, const int &date_yy,
//    const int &time_hh, const int &time_mm, const int &time_ss,
//    const char &plus_or_minus_tz, const int tz_hh, int tz_mm) {
//    ... [code implementation] ...
// }

// Removing undocumented command
// const std::string cmd_get_date_and_time_and_tz() { return ":GMTI#"; }

// NEW: Get status - response should be "sss#"
// Removing duplicate status command
// const std::string cmd_get_general_status() { return ":GU#"; }

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
