#ifndef ONSTEP_COMMANDS_HPP
#define ONSTEP_COMMANDS_HPP

#include "common/alpaca_exception.hpp"
#include "fmt/format.h"
#include <catch2/catch_test_macros.hpp>
#include <regex>
#include <string>
#include <vector>
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bundled/format.h"

namespace onstep_commands {

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

const std::string cmd_get_version();
// Removing undocumented commands
// const std::string cmd_switch_to_eq_mode();
// const std::string cmd_switch_to_alt_az_mode();
const std::string cmd_get_date();
const std::string cmd_set_date(const int &mm, const int &dd, const int &yy);
const std::string cmd_set_time(const int &hh, const int &mm, const int &ss);
const std::string cmd_get_time();
const std::string cmd_get_time_12h();
const std::string cmd_get_sidereal_time();
const std::string cmd_set_sidereal_time(const int &hh, const int &mm, const int &ss);
const std::string cmd_get_daylight_savings();
const std::string cmd_set_daylight_savings(const int &on_or_off);

const std::string cmd_set_timezone(const char &plus_or_minus,
                                   const int &h_offset, int m_offset = 0);

const std::string cmd_get_timezone();

const std::string cmd_set_latitude(const char &plus_or_minus, const int &dd,
                                   const int &mm, int ss);

const std::string cmd_get_latitude();

const std::string cmd_set_longitude(const char &plus_or_minus, const int &ddd,
                                    const int &mm, const int &ss);

const std::string cmd_get_longitude();
const std::string cmd_set_site_0_name(const std::string &site_name);
const std::string cmd_set_site_1_name(const std::string &site_name);
const std::string cmd_set_site_2_name(const std::string &site_name);
const std::string cmd_set_site_3_name(const std::string &site_name);
const std::string cmd_get_site_0_name();
const std::string cmd_get_site_1_name();
const std::string cmd_get_site_2_name();
const std::string cmd_get_site_3_name();
const std::string cmd_select_site(const int &site_number);
const std::string cmd_get_current_cardinal_direction();
const std::string cmd_get_target_ra();

const std::string cmd_set_target_ra(const int &hh, const int &mm,
                                    const int &ss);

const std::string cmd_get_target_dec();
const std::string cmd_set_target_dec(const char &plus_or_minus, const int &dd,
                                     const int &mm, const int &ss);
const std::string cmd_set_target_azm(const int &ddd, const int &mm, const int &ss);
const std::string cmd_set_target_alt(const char &plus_or_minus, const int &dd,
                                     const int &mm, const int &ss);
const std::string cmd_set_horizon_limit(const char &plus_or_minus, const int &dd);
const std::string cmd_get_horizon_limit();
const std::string cmd_set_overhead_limit(const int &dd);
const std::string cmd_get_overhead_limit();

const std::string cmd_get_current_ra();
const std::string cmd_get_current_dec();
const std::string cmd_get_azimuth();
const std::string cmd_get_altitude();
const std::string cmd_goto();
const std::string cmd_goto_horizontal();
const std::string cmd_stop_moving();

auto format_as(move_speed_enum s);
const std::string cmd_set_moving_speed(move_speed_enum move_speed);
const std::string cmd_set_0_5x_sidereal_rate();
const std::string cmd_set_1x_sidereal_rate();
const std::string cmd_set_720x_sidereal_rate();
const std::string cmd_set_1440x_sidereal_rate();
const std::string cmd_set_moving_speed_precise(const double &move_speed);
const std::string cmd_move_towards_east();
const std::string cmd_stop_moving_towards_east();
const std::string cmd_move_towards_west();
const std::string cmd_stop_moving_towards_west();
const std::string cmd_move_towards_north();
const std::string cmd_stop_moving_towards_north();
const std::string cmd_move_towards_south();
const std::string cmd_stop_moving_towards_south();
const std::string cmd_set_tracking_rate_to_sidereal();
const std::string cmd_set_sidereal_rate_ra(const double &rate);
const std::string cmd_get_sidereal_rate_ra();
const std::string cmd_track_sidereal_rate_reset();
const std::string cmd_track_rate_increase();
const std::string cmd_track_rate_decrease();
const std::string cmd_set_tracking_rate_to_solar();
const std::string cmd_set_tracking_rate_to_lunar();
const std::string cmd_set_tracking_rate_to_king();
const std::string cmd_get_tracking_rate();
const std::string cmd_start_tracking();
const std::string cmd_stop_tracking();
const std::string cmd_enable_refraction_rate_tracking();
const std::string cmd_disable_refraction_rate_tracking();
const std::string cmd_get_tracking_status();
const std::string cmd_guide(const char &direction, const int &rate);
const std::string cmd_set_guide_rate(const double &guide_rate);
const std::string cmd_get_guide_rate();

const std::string
cmd_set_act_of_crossing_meridian(const int &perform_meridian_flip,
                                 const int &continue_to_track_after_meridian,
                                 const char &plus_or_minus,
                                 const int &limit_angle_after_meridian);

const std::string cmd_get_act_of_crossing_meridian();
const std::string cmd_sync();
const std::string cmd_home_position();
const std::string cmd_set_home();
const std::string cmd_get_status();
const std::string cmd_park();
const std::string cmd_restore_parked_telescope();
const std::string cmd_get_distance_bars();
const std::string cmd_reset_controller();
const std::string cmd_reset_eeprom();
const std::string cmd_set_baud_rate(const int &rate);
const std::string cmd_precision_toggle();
const std::string cmd_get_firmware_date();
const std::string cmd_get_firmware_time();
const std::string cmd_get_firmware_number();
const std::string cmd_get_firmware_name();
// Removing duplicate status command
// const std::string cmd_get_general_status();

// Anti-backlash commands
const std::string cmd_set_ra_backlash(const int &backlash);
const std::string cmd_set_dec_backlash(const int &backlash);

// Basic focuser commands
const std::string cmd_is_focuser1_active();
const std::string cmd_is_focuser2_active();
const std::string cmd_select_primary_focuser(const int &n);
const std::string cmd_get_primary_focuser();
const std::string cmd_get_focuser_status();
const std::string cmd_get_focuser_mode();
const std::string cmd_get_focuser_full_in_position();
const std::string cmd_get_focuser_max_position();
const std::string cmd_stop_focuser();
const std::string cmd_set_focuser_fast_motion();
const std::string cmd_set_focuser_slow_motion();
const std::string cmd_move_focuser_in();
const std::string cmd_move_focuser_out();
const std::string cmd_get_focuser_position();
const std::string cmd_set_focuser_position_zero();
const std::string cmd_set_focuser_position_half_travel();
const std::string cmd_set_focuser_target_half_travel();

// PEC Commands
const std::string cmd_turn_pec_on();
const std::string cmd_turn_pec_off();
const std::string cmd_clear_pec_data();
const std::string cmd_start_recording_pec();
const std::string cmd_save_pec_data();
const std::string cmd_get_pec_status();
const std::string cmd_readout_pec_data(const int &index);
const std::string cmd_readout_pec_data_at_current_index();
const std::string cmd_write_pec_data(const int &index, const int &steps);

// Alignment Commands
const std::string cmd_align_write_model();
const std::string cmd_align_one_star();
const std::string cmd_align_two_star();
const std::string cmd_align_three_star();
const std::string cmd_align_accept();

// Reticle Control
const std::string cmd_increase_reticule_brightness();
const std::string cmd_decrease_reticule_brightness();

// Removing undocumented commands
// const std::string cmd_set_lat_and_long(const char &plus_or_minus_lat,
//                                     const int &lat_dd, const int lat_mm,
//                                     const int &lat_ss,
//                                     const char &plus_or_minus_long,
//                                     const int &long_ddd, const int &long_mm,
//                                     const int &long_ss);

// const std::string cmd_get_lat_and_long();

// const std::string cmd_set_date_time_and_tz(
//     const int &date_mm, const int &date_dd, const int &date_yy,
//     const int &time_hh, const int &time_mm, const int &time_ss,
//     const char &plus_or_minus_tz, const int tz_hh, int tz_mm = 0);

// const std::string cmd_get_date_and_time_and_tz();
// const std::string cmd_get_target_ra_and_dec();
// const std::string cmd_get_current_ra_and_dec();
// const std::string cmd_get_az_and_alt();

// const std::string cmd_set_target_ra_and_dec_and_goto(
//     const int &ra_hh, const int &ra_mm, const int &ra_ss,
//     const char &plus_or_minus_dec, const int &dec_dd, const int &dec_mm,
//     const int &dec_ss);

// const std::string cmd_set_target_ra_and_dec_and_sync(
//     const int &ra_hh, const int &ra_mm, const int &ra_ss,
//     const char &plus_or_minus_dec, const int &dec_dd, const int &dec_mm,
//     const int &dec_ss);

}; // namespace onstep_commands

namespace onstep_responses {

// 1#, 0# or e+error_code+#
  int parse_standard_response(const std::string &resp);

struct hh_mm_ss {
  int hh;
  int mm;
  int ss;
  double as_decimal();
  hh_mm_ss();
  hh_mm_ss(const double &val);
};

  hh_mm_ss parse_hh_mm_ss_response(const std::string &resp);

struct dd_mm_ss {
  int dd;
  int mm;
  int ss;

  double as_decimal();
};

  dd_mm_ss parse_dd_mm_ss_response(const std::string &resp);

struct sdd_mm_ss {
  char plus_or_minus;
  int dd;
  int mm;
  int ss;
  double as_decimal();
  sdd_mm_ss();
  sdd_mm_ss(const double &val);
  };

sdd_mm_ss parse_sdd_mm_ss_response(const std::string &resp);

struct sddd_mm_ss {
  char plus_or_minus;
  int ddd;
  int mm;
  int ss;
  double as_decimal();
  sddd_mm_ss();
  sddd_mm_ss(const double &val);
  };

// -098*00:00#
sddd_mm_ss parse_sddd_mm_ss_response(const std::string &resp);

struct mm_dd_yy {
  int mm;
  int dd;
  int yy;
};

mm_dd_yy parse_mm_dd_yy_response(const std::string &resp);

struct shh_mm {
  char plus_or_minus;
  int hh;
  int mm;
};

shh_mm parse_shh_mm_response(const std::string &resp);

struct sdd_mm {
  char plus_or_minus;
  int dd;
  int mm;

  double as_decimal();
  sdd_mm();
  sdd_mm(const double &val);
};

sdd_mm parse_sdd_mm_response(const std::string &resp);

struct sddd_mm {
  char plus_or_minus;
  int ddd;
  int mm;

  double as_decimal();
  sddd_mm();
  sddd_mm(const double &val);
};

sddd_mm parse_sddd_mm_response(const std::string &resp);

std::vector<std::string> split_on(const std::string &string_to_split,
                                  const std::string &separator);

struct sdd_mm_sddd_mm {
  sdd_mm sdd_mm_data;
  sddd_mm sddd_mm_data;
};

sdd_mm_sddd_mm parse_sdd_mm_and_sddd_mm_response(const std::string &resp);

struct ddd_mm_ss {
  int ddd;
  int mm;
  int ss;
  double as_decimal();
  ddd_mm_ss();
  ddd_mm_ss(const double &val);
};

ddd_mm_ss parse_ddd_mm_ss_response(const std::string &resp);

}; // namespace onstep_responses

namespace onor = onstep_responses;

template <> struct fmt::formatter<onor::hh_mm_ss> : formatter<string_view> {
  auto format(onor::hh_mm_ss d, format_context &ctx) const {
    return format_to(ctx.out(), "{:#02d}:{:#02d}:{:#02d}", d.hh, d.mm, d.ss);
  };
};

template <> struct fmt::formatter<onor::dd_mm_ss> : formatter<string_view> {
  auto format(onor::dd_mm_ss d, format_context &ctx) const {
    return format_to(ctx.out(), "{:#02d}*{:#02d}:{:#02d}", d.dd, d.mm, d.ss);
  };
};

template <> struct fmt::formatter<onor::sdd_mm_ss> : formatter<string_view> {
  auto format(onor::sdd_mm_ss d, format_context &ctx) const {
    return format_to(ctx.out(), "{}{:#02d}*{:#02d}:{:#02d}", d.plus_or_minus,
                     d.dd, d.mm, d.ss);
  };
};

template <> struct fmt::formatter<onor::mm_dd_yy> : formatter<string_view> {
  auto format(onor::mm_dd_yy d, format_context &ctx) const {
    return format_to(ctx.out(), "{:#02d}/{:#02d}/{:#02d}", d.mm, d.dd, d.yy);
  };
};

template <> struct fmt::formatter<onor::shh_mm> : formatter<string_view> {
  auto format(onor::shh_mm d, format_context &ctx) const {
    return format_to(ctx.out(), "{}{:#02d}:{:#02d}", d.plus_or_minus, d.hh,
                     d.mm);
  };
};

template <> struct fmt::formatter<onor::sdd_mm> : formatter<string_view> {
  auto format(onor::sdd_mm d, format_context &ctx) const {
    return format_to(ctx.out(), "{}{:#02d}:{:#02d}", d.plus_or_minus, d.dd,
                     d.mm);
  };
};

template <> struct fmt::formatter<onor::sddd_mm> : formatter<string_view> {
  auto format(onor::sddd_mm d, format_context &ctx) const {
    return format_to(ctx.out(), "{}{:#03d}:{:#02d}", d.plus_or_minus, d.ddd,
                     d.mm);
  };
};

template <> struct fmt::formatter<onor::ddd_mm_ss> : formatter<string_view> {
  auto format(onor::ddd_mm_ss d, format_context &ctx) const {
    return format_to(ctx.out(), "{:#03d}*{:#02d}:{:#02d}", d.ddd, d.mm, d.ss);
  };
};

#endif
