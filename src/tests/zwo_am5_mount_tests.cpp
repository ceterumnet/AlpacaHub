#include "drivers/zwo_am5_telescope.hpp"

// auto format_as(pier_side_enum s) { return fmt::underlying(s); }
// auto format_as(drive_rate_enum s) { return fmt::underlying(s); }
// auto format_as(telescope_axes_enum s) { return fmt::underlying(s); }
// auto format_as(guide_direction_enum s) { return fmt::underlying(s); }

// template <typename Duration>
// inline auto a_localtime(date::local_time<Duration> time) -> std::tm {
//   return fmt::localtime(
//       std::chrono::system_clock::to_time_t(date::current_zone()->to_sys(time)));
// }

namespace zwoc = zwo_commands;

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

TEST_CASE("Test get version", "[cmd_get_version]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  auto resp = telescope.send_command_to_mount(zwoc::cmd_get_version());
  spdlog::debug("Version data returned: {0}", resp);
  // REQUIRE(telescope.at_home() == true);
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
    if (telescope.slewing())
      spdlog::debug("slewing...");

    if (telescope.at_home())
      break;

    // spdlog::debug("waiting 1 second for mount to settle");
    // I'm not sure I need this anymore
    std::this_thread::sleep_for(1000ms);
  }
  REQUIRE(telescope.at_home() == true);
}

TEST_CASE("Test get azimuth", "[azimuth]") {
  using namespace std::chrono_literals;
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  double val = telescope.azimuth();
  spdlog::debug("Current azimuth: {0}", val);
  // REQUIRE(telescope.at_home() == true);
}

TEST_CASE("Test get declination", "[declination]") {
  using namespace std::chrono_literals;
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  double val = telescope.declination();
  spdlog::debug("Current declination: {0}", val);
  // REQUIRE(telescope.at_home() == true);
}

TEST_CASE("Test get right_ascension", "[right_ascension]") {
  using namespace std::chrono_literals;
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  double val = telescope.right_ascension();
  spdlog::debug("Current right ascension: {0}", val);
  // REQUIRE(telescope.at_home() == true);
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

TEST_CASE("Test set and get UTC", "[get_and_set_utc_time]") {
  spdlog::set_level(spdlog::level::trace);
  using namespace std::chrono_literals;
  auto cur_tz = date::current_zone();
  spdlog::trace("cur_tz name {}", cur_tz->name());
  date::sys_info sys_info;

  // Chrono System Clock
  auto tp = std::chrono::system_clock::now();
  spdlog::trace("tp raw: {}", tp.time_since_epoch());
  spdlog::trace("tp:     {:%F %T %Z} ", tp);

  auto tp_zoned_time = date::make_zoned(cur_tz, tp);
  spdlog::trace("tp_zoned_time: {} ",
                tp_zoned_time.get_local_time().time_since_epoch());
  spdlog::trace("tp_zoned_time: {:%F %T %Z}", tp_zoned_time.get_local_time());

  auto l_tp = date::floor<std::chrono::seconds>(cur_tz->to_local(tp));
  spdlog::trace("l_tp:            {} ", l_tp.time_since_epoch());

  auto l_tp_zoned_time = date::make_zoned(cur_tz, l_tp);
  spdlog::trace("l_tp_zoned_time: {} ",
                l_tp_zoned_time.get_local_time().time_since_epoch());
  spdlog::trace("l_tp_zoned_time: {:%F %T %Z}",
                l_tp_zoned_time.get_local_time());

  // Date UTC Clock
  auto u_tp = date::utc_clock::now();
  auto u_tp_cast = date::clock_cast<date::utc_clock>(u_tp);

  spdlog::trace("u_tp raw:  {}", u_tp.time_since_epoch());
  spdlog::trace("u_tp_cast: {:%F %T %Z}",
                fmt::gmtime(date::utc_clock::to_sys(u_tp_cast)));

  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);

  // Clear sync data
  telescope.send_command_to_mount(":NSC#", true, '\0');

  telescope.set_site_latitude(30.33333333333);
  telescope.set_site_longitude(-98.0);

  auto utc_date_str = telescope.utc_date();
  spdlog::debug("TIME ********* ");
  spdlog::debug(utc_date_str);
  spdlog::debug("TIME ********* ");

  auto set_utc_date_str = fmt::format("{0:%F}T{0:%T}.0000000Z", tp);
  spdlog::debug("calling set_utc_date with: {}", set_utc_date_str);
  telescope.set_utc_date(set_utc_date_str);

  utc_date_str = telescope.utc_date();
  spdlog::debug("TIME ********* ");
  spdlog::debug(utc_date_str);
  spdlog::debug("TIME ********* ");

  spdlog::debug("SIDEREAL TIME ********* ");
  auto sidereal_tm = telescope.sidereal_time();
  spdlog::debug("sidereal_tm: {}", sidereal_tm);
  spdlog::debug("SIDEREAL TIME ********* ");
}

TEST_CASE("Test mount get time", "[get_time]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);
  spdlog::trace("sending cmd_get_time()");
  std::string status_str =
      telescope.send_command_to_mount(zwoc::cmd_get_time());
  spdlog::debug("cmd_get_time() status_str: {0}", status_str);
}

TEST_CASE("Test mount get sidereal time", "[get_sidereal_time]") {
  spdlog::set_level(spdlog::level::trace);
  zwo_am5_telescope telescope;
  telescope.set_serial_device("/dev/ttyACM0");
  telescope.set_connected(true);
  spdlog::trace("sending cmd_get_sidereal_time()");
  std::string status_str =
      telescope.send_command_to_mount(zwoc::cmd_get_sidereal_time());
  spdlog::trace("sending cmd_get_timezone()");
  status_str = telescope.send_command_to_mount(zwoc::cmd_get_timezone());
  spdlog::trace("sending cmd_get_date()");
  status_str = telescope.send_command_to_mount(zwoc::cmd_get_date());
  spdlog::trace("sending cmd_get_time()");
  status_str = telescope.send_command_to_mount(zwoc::cmd_get_time());
  spdlog::trace("sending cmd_get_daylight_savings()");
  status_str =
      telescope.send_command_to_mount(zwoc::cmd_get_daylight_savings());
  spdlog::trace("sending cmd_get_lat_and_long()");
  status_str = telescope.send_command_to_mount(zwoc::cmd_get_lat_and_long());

  // spdlog::trace("sending cmd_set_timezone()");
  // status_str = telescope.send_command_to_mount(
  //   zwoc::cmd_set_timezone('-', 6), true, true);

  spdlog::trace("fetching sidereal time");
  telescope.send_command_to_mount(zwoc::cmd_get_sidereal_time());

  // telescope.set_site_longitude(-98.0);
  // telescope.set_site_latitude(30.33);

  spdlog::trace("fetching sidereal time");
  telescope.send_command_to_mount(zwoc::cmd_get_sidereal_time());

  spdlog::trace("fetching longitude");
  telescope.send_command_to_mount(zwoc::cmd_get_longitude());

  // status_str = telescope.utc_date();
  // spdlog::debug("UTC returned from scope: {}", status_str);
  // telescope.set_utc_date()
  // spdlog::debug("cmd_get_sidereal_time() status_str: {0}", status_str);
}

TEST_CASE("Test east/west/north/south", "[move_axis]") {
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

// TODO: finish writing this case
// TEST_CASE("Get tracking rate", "[tracking_rate]") {
//   spdlog::set_level(spdlog::level::trace);
//   zwo_am5_telescope telescope;
//   telescope.set_serial_device("/dev/ttyACM0");
//   telescope.set_connected(true);
//   telescope.tracking_rate();
// }
