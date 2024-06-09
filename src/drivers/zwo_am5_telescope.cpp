#include "zwo_am5_telescope.hpp"
#include "common/alpaca_exception.hpp"
#include "drivers/zwo_am5_commands.hpp"
#include "interfaces/i_alpaca_telescope.hpp"
#include <chrono>
#include <mutex>
#include <thread>

namespace zwoc = zwo_commands;
namespace zwor = zwo_responses;

std::vector<std::string> zwo_am5_telescope::serial_devices() {
  std::vector<std::string> serial_devices{
      "/dev/serial/by-id/usb-ZWO_Systems_ZWO_Device_123456-if00"};

  return serial_devices;
}

// TODO: Need to figure out how to have a unique identifier for this
std::string zwo_am5_telescope::unique_id() {
  return "usb-ZWO_Systems_ZWO_Device_123456-if00";
}

bool zwo_am5_telescope::connected() { return _connected; }

int zwo_am5_telescope::set_connected(bool connected) {
  // covers already connected with connect request and
  // not connected with disconnect request
  if (_connected && connected) {
    spdlog::warn(
        "set_connected called with connected:{0} but is already in that state");
    return 0;
  }

  // Reset state of the target being set
  _ra_target_set = false;
  _dec_target_set = false;
  if (connected) {
    try {
      spdlog::debug("Setting connected to true");
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

      _tracking_enabled = false;
      auto resp = send_command_to_mount(zwoc::cmd_get_tracking_status());
      if (resp == "1#")
        _tracking_enabled = true;

      return 0;
    } catch (asio::system_error &e) {
      spdlog::error("problem opening serial connection. {0}", e.what());
      throw alpaca_exception(
          alpaca_exception::DRIVER_ERROR,
          fmt::format(
              "Problem opening serial connection at {0} with error: {1}",
              _serial_device_path, e.what()));
    }
  } else {
    try {
      spdlog::debug("Setting connected to false");
      _serial_port.close();
      _connected = false;
      return 0;
    } catch (asio::system_error &e) {
      spdlog::error("problem closing serial connection {}", e.what());
      throw alpaca_exception(
          alpaca_exception::DRIVER_ERROR,
          fmt::format(
              "Problem closing serial connection at {0} with error: {1}",
              _serial_device_path, e.what()));
    }
  }
  return -1;
}

zwo_am5_telescope::zwo_am5_telescope()
    : _parked(false), _connected(false), _guide_rate(.8), _site_longitude(0),
      _site_latitude(0), _site_elevation(0), _aperture_diameter(0),
      _moving(false), _io_context(1), _serial_port(_io_context),
      _is_pulse_guiding(false), _ra_target_set(false), _dec_target_set(false){};

zwo_am5_telescope::~zwo_am5_telescope() {
  spdlog::debug("Closing serial connection");
  _serial_port.close();
};

void zwo_am5_telescope::throw_if_not_connected() {
  if (!_connected)
    throw alpaca_exception(alpaca_exception::NOT_CONNECTED,
                           "Mount not connected");
}

void zwo_am5_telescope::throw_if_parked() {
  if (_parked)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Mount is parked");
}

std::string zwo_am5_telescope::send_command_to_mount(const std::string &cmd,
                                                     bool read_response,
                                                     char stop_on_char) {
  spdlog::trace("sending: {} to mount", cmd);
  std::lock_guard lock(_telescope_mtx);
  char buf[512] = {0};
  _serial_port.write_some(asio::buffer(cmd));

  std::string rsp;

  if (read_response) {
    _io_context.reset();

    // TODO: we may need to make the read timeout configurable here
    alpaca_hub_serial::blocking_reader reader(cmd, _serial_port, 250,
                                              _io_context);
    char c;
    while (reader.read_char(c)) {
      rsp += c;
      if (c == stop_on_char || stop_on_char == '\0' || c == '#') {
        break;
      }
    }
  }

  spdlog::trace("mount returned: {}", rsp);
  return rsp;
}

uint32_t zwo_am5_telescope::interface_version() { return 3; }

std::string zwo_am5_telescope::driver_version() { return "v0.1"; }

std::string zwo_am5_telescope::description() {
  return "ZWO AM5 Telescope Driver";
}

std::string zwo_am5_telescope::driverinfo() {
  return "ZWO AM5 Telescope Driver Information";
}

std::string zwo_am5_telescope::name() { return "ZWO AM5 Mount"; };

std::vector<std::string> zwo_am5_telescope::supported_actions() {
  return std::vector<std::string>();
}

// I'm going to hard code this. Based on the development documentation for the
// AM5, it seems that the mount must be rebooted when the mode is switched.
// Given that the use case for this is imaging - alt / az mode doesn't really
// sense.
alignment_mode_enum zwo_am5_telescope::alignment_mode() {
  return alignment_mode_enum::german_polar;
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

double zwo_am5_telescope::aperture_area() {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Aperture area not available");
  return 0;
}

// I'm wondering if I need to implement a state machine
// to store / set states around movement...
bool zwo_am5_telescope::slewing() {
  if (_moving)
    return true;
  auto resp = send_command_to_mount(zwoc::cmd_get_status());
  auto last_2_chars = resp.substr(resp.size() - 2, resp.size());
  // 4 is moving status
  if (last_2_chars.find("4#") != std::string::npos ||
      last_2_chars.find("2#") != std::string::npos)
    return true;
  spdlog::trace("last_2_chars: {}", last_2_chars);
  spdlog::trace("resp: {}", resp);
  return false;
}

bool zwo_am5_telescope::at_home() {
  throw_if_not_connected();
  std::string resp = send_command_to_mount(zwoc::cmd_get_status());
  if (resp.find('H') != std::string::npos)
    return true;
  return false;
}

// TODO: implement
bool zwo_am5_telescope::at_park() {
  throw_if_not_connected();
  return _parked;
}

// TODO: implement
double zwo_am5_telescope::azimuth() {
  throw_if_not_connected();
  std::string resp = send_command_to_mount(zwoc::cmd_get_azimuth());
  spdlog::trace("raw value returned from mount for cmd_get_azimuth(): {0}",
                resp);
  return zwor::parse_ddd_mm_ss_response(resp).as_decimal();
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
  return false;
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
  return false;
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

bool zwo_am5_telescope::can_slew_async() {
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

// The mount doesn't support this natively, so we will emulate
bool zwo_am5_telescope::can_unpark() {
  throw_if_not_connected();
  return true;
}

double zwo_am5_telescope::declination() {
  throw_if_not_connected();
  std::string resp = send_command_to_mount(zwoc::cmd_get_current_dec());
  spdlog::trace("declination returned: {0}", resp);
  auto parsed_resp = zwor::parse_sdd_mm_ss_response(resp);
  // The ASCOM driver does this every time it calls :GD#
  send_command_to_mount(":GFD1#");

  return parsed_resp.as_decimal();
}

// Returning 0 because we aren't supporting setting a dec rate
double zwo_am5_telescope::declination_rate() {
  throw_if_not_connected();
  return 0;
}

// not supported
int zwo_am5_telescope::set_declination_rate(const double &) {
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

equatorial_system_enum zwo_am5_telescope::equatorial_system() {
  throw_if_not_connected();
  return equatorial_system_enum::topocentric;
}

double zwo_am5_telescope::focal_length() {
  throw_if_not_connected();
  return _focal_length;
}

int zwo_am5_telescope::set_focal_length(const double &focal_length) {
  throw_if_not_connected();
  _focal_length = focal_length;
  return 0;
}

// TODO: clean this up so it is safe
double zwo_am5_telescope::guide_rate_declination() {
  throw_if_not_connected();
  auto resp = send_command_to_mount(zwoc::cmd_get_guide_rate());
  std::string value = resp.substr(0, 3);
  auto rate_in_degrees_per_second = atof(&value[0]) * .0042;
  return rate_in_degrees_per_second;
}

int zwo_am5_telescope::set_guide_rate_declination(const double &rate) {
  throw_if_not_connected();
  spdlog::debug("set_guide_rate_declination called with {} converted to {}",
                rate, rate / .0042);
  send_command_to_mount(zwoc::cmd_set_guide_rate(rate / .0042), false);
  return 0;
}

// We just use the same method as declination
double zwo_am5_telescope::guide_rate_ascension() {
  return guide_rate_declination();
}

// We just use the same method as declination
int zwo_am5_telescope::set_guide_rate_ascension(const double &rate) {
  return set_guide_rate_declination(rate);
}

bool zwo_am5_telescope::is_pulse_guiding() {
  spdlog::debug("is_pulse_guiding() invoked");
  return _is_pulse_guiding;
}

double zwo_am5_telescope::right_ascension() {
  throw_if_not_connected();
  spdlog::debug("right_ascension() invoked");
  std::string resp = send_command_to_mount(zwoc::cmd_get_current_ra());
  spdlog::trace("ra returned: {0}", resp);
  auto parsed_resp = zwor::parse_hh_mm_ss_response(resp);
  // The ASCOM driver does this every time it calls :GR#
  send_command_to_mount(":GFR1#");
  return parsed_resp.as_decimal();
}

// I believe this is just 0 as I don't believe the AM5 supports a
// separte RA rate
double zwo_am5_telescope::right_ascension_rate() { return 0; }

int zwo_am5_telescope::set_right_ascension_rate(const double &) {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Setting a RA rate is not supported on this mount");
  return 0;
}

// After testing this, I'm a little bit concerned that the
// accuracy of this is a little bit questionable...
pier_side_enum zwo_am5_telescope::side_of_pier() {
  throw_if_not_connected();
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

int zwo_am5_telescope::set_side_of_pier(const pier_side_enum &) {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Setting side of pier not supported on this mount");
  return 0;
}

double zwo_am5_telescope::sidereal_time() {
  throw_if_not_connected();
  auto resp = send_command_to_mount(zwoc::cmd_get_sidereal_time());
  auto sidereal_data = zwor::parse_hh_mm_ss_response(resp);
  return sidereal_data.as_decimal();
}

double zwo_am5_telescope::site_elevation() {
  throw_if_not_connected();
  return _site_elevation;
}

// TODO: add some validation
int zwo_am5_telescope::set_site_elevation(const double &site_elevation) {
  throw_if_not_connected();
  if (site_elevation > 10000 || site_elevation < -300)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "Elevation must be between -300m and 10,000m");
  _site_elevation = site_elevation;
  return 0;
}

double zwo_am5_telescope::site_latitude() {
  throw_if_not_connected();
  auto resp = send_command_to_mount(zwoc::cmd_get_latitude());
  auto parsed_resp = zwor::parse_sdd_mm_ss_response(resp);
  spdlog::debug("returning site_latitude: {}", parsed_resp.as_decimal());
  return parsed_resp.as_decimal();
}

// TODO: add some validation
int zwo_am5_telescope::set_site_latitude(const double &site_latitude) {
  throw_if_not_connected();
  if (site_latitude < -90 || site_latitude > 90)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE, "Latitude invalid");
  spdlog::debug("set_site_latitude invoked with {}", site_latitude);

  zwor::sdd_mm_ss latitude_s(site_latitude);

  spdlog::debug(" converts to {}", latitude_s);
  auto resp = send_command_to_mount(
      zwoc::cmd_set_latitude(latitude_s.plus_or_minus, latitude_s.dd,
                             latitude_s.mm, latitude_s.ss),
      true, '\0');
  _site_latitude = site_latitude;
  return 0;
}

double zwo_am5_telescope::site_longitude() {
  throw_if_not_connected();
  auto resp = send_command_to_mount(zwoc::cmd_get_longitude());
  auto parsed_resp = zwor::parse_sddd_mm_ss_response(resp);


  // This is a weird behavior from the ASCOM driver I'm mimicking
  if (parsed_resp.plus_or_minus == '+')
    parsed_resp.plus_or_minus = '-';
  else
    parsed_resp.plus_or_minus = '+';
  spdlog::debug("returning site_longitude: {}", parsed_resp.as_decimal());

  return parsed_resp.as_decimal();
}

// TODO: add some validation
int zwo_am5_telescope::set_site_longitude(const double &site_longitude) {
  throw_if_not_connected();
  if (site_longitude < -180 || site_longitude > 180)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "Longitude invalid");
  spdlog::debug("set_site_longitude invoked with {}", site_longitude);

  zwor::sddd_mm_ss longitude_s(site_longitude);

  // This is a weird behavior from the ASCOM driver I'm mimicking
  if (longitude_s.plus_or_minus == '+')
    longitude_s.plus_or_minus = '-';
  else
    longitude_s.plus_or_minus = '+';
  auto resp = send_command_to_mount(
      zwoc::cmd_set_longitude(longitude_s.plus_or_minus, longitude_s.ddd,
                              longitude_s.mm, longitude_s.ss),
      true, '\0');

  _site_longitude = site_longitude;
  return 0;
}

int zwo_am5_telescope::slew_settle_time() {
  throw_if_not_connected();
  return _slew_settle_time;
}

// TODO: add some validation
int zwo_am5_telescope::set_slew_settle_time(const int &slew_settle_time) {
  throw_if_not_connected();
  if (slew_settle_time < 0)
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "slew settle time must be a positive value");
  _slew_settle_time = slew_settle_time;
  return 0;
}

// sdd_mm_ss
double zwo_am5_telescope::target_declination() {
  throw_if_not_connected();
  if (!_dec_target_set)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Must set DEC target before reading");

  auto resp = send_command_to_mount(zwoc::cmd_get_target_dec());
  double val = zwor::parse_sdd_mm_ss_response(resp).as_decimal();
  return val;
}

// sdd_mm_ss
int zwo_am5_telescope::set_target_declination(const double &dec) {
  throw_if_not_connected();
  zwor::sdd_mm_ss converted(dec);
  auto resp = send_command_to_mount(
      zwoc::cmd_set_target_dec(converted.plus_or_minus, converted.dd,
                               converted.mm, converted.ss),
      true, '\0');
  if (resp == "1") {
    _dec_target_set = true;
    return 0;
  } else {
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           "Failed to set target declination");
    return -1;
  }
}

// hh_mm_ss
double zwo_am5_telescope::target_right_ascension() {
  throw_if_not_connected();
  if (!_ra_target_set)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Must set RA target before reading");
  auto resp = send_command_to_mount(zwoc::cmd_get_target_ra());
  double val = zwor::parse_hh_mm_ss_response(resp).as_decimal();
  return val;
}

// hh_mm_ss
int zwo_am5_telescope::set_target_right_ascension(const double &ra) {
  throw_if_not_connected();
  zwor::hh_mm_ss converted(ra);
  auto resp = send_command_to_mount(
      zwoc::cmd_set_target_ra(converted.hh, converted.mm, converted.ss), true,
      '\0');
  if (resp == "1") {
    _ra_target_set = true;
    return 0;
  } else {
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           "Failed to set target right ascension");
    return -1;
  }
  return 0;
}

// TODO: double check alpaca / ASCOM docs for correct behavior if error code
// is returned

// I'm going to store the state and manage behind the scenes...
bool zwo_am5_telescope::tracking() {
  throw_if_not_connected();
  return _tracking_enabled;
  // if(slewing())
  //   return _tracking_enabled;
  // auto resp = send_command_to_mount(zwoc::cmd_get_tracking_status());
  // auto val = zwor::parse_standard_response(resp);
  // if (val == 1)
  //   return true;
  // return false;
}

int zwo_am5_telescope::set_tracking(const bool &tracking) {
  using namespace std::chrono_literals;
  int val = 0;
  std::string resp;
  throw_if_not_connected();
  spdlog::debug("set_tracking invoked with {}", tracking);
  if (tracking)
    resp = send_command_to_mount(zwoc::cmd_start_tracking(), true, '\0');
  else
    resp = send_command_to_mount(zwoc::cmd_stop_tracking(), true, '\0');
  if (resp == "1") {
    _tracking_enabled = tracking;
    return 0;
  } else {
    auto gat_resp = send_command_to_mount(zwoc::cmd_get_tracking_status());
    spdlog::warn("failed to set tracking status to {}, error: {}", tracking,
                 gat_resp);
    if (gat_resp == "1#" && tracking) {
      spdlog::info("ignore warning, mount is tracking.");
      _tracking_enabled = tracking;
      return 0;
    }

    if (gat_resp == "0#" && !tracking) {
      spdlog::info("ignore warning, mount is not tracking.");
      _tracking_enabled = tracking;
      return 0;
    }

    if (gat_resp == "0#" && tracking) {
      spdlog::debug("is mount moving? {}", slewing());
      spdlog::debug("waiting 2000ms and retrying tracking");
      std::this_thread::sleep_for(2000ms);
      resp = send_command_to_mount(zwoc::cmd_start_tracking(), true, '\0');
      if (resp == "1") {
        spdlog::info("ignore previous warning, tracking retry successful");
        _tracking_enabled = tracking;
        return 0;
      } else {
        gat_resp = send_command_to_mount(zwoc::cmd_get_tracking_status());
        if (gat_resp == "1#") {
          spdlog::info("ignore previous warning");
          _tracking_enabled = tracking;
          return 0;
        }
        spdlog::warn("tracking retry failed: {}", resp);
      }
    }

    if (gat_resp == "1#" && !tracking) {
      spdlog::debug("is mount moving? {}", slewing());
      spdlog::debug("waiting 2000ms and retrying tracking");
      std::this_thread::sleep_for(2000ms);
      resp = send_command_to_mount(zwoc::cmd_stop_tracking(), true, '\0');
      if (resp == "1") {
        spdlog::info("ignore previous warning, tracking retry successful");
        _tracking_enabled = tracking;
        return 0;
      } else {
        gat_resp = send_command_to_mount(zwoc::cmd_get_tracking_status());
        if (gat_resp == "0#") {
          spdlog::info("ignore previous warning");
          _tracking_enabled = tracking;
          return 0;
        }
        spdlog::warn("tracking retry failed: {}", resp);
      }
    }

    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("Failed to set tracking status: to {}, error: {}", tracking,
                    gat_resp));
  }
  return -1;
}

drive_rate_enum zwo_am5_telescope::tracking_rate() {
  auto resp = send_command_to_mount(zwoc::cmd_get_tracking_rate());
  return static_cast<drive_rate_enum>(zwor::parse_standard_response(resp));
}

int zwo_am5_telescope::set_tracking_rate(const drive_rate_enum &tracking_rate) {
  throw_if_not_connected();
  switch (tracking_rate) {
  case drive_rate_enum::sidereal:
    send_command_to_mount(zwoc::cmd_set_tracking_rate_to_sidereal(), false);
    break;
  case drive_rate_enum::solar:
    send_command_to_mount(zwoc::cmd_set_tracking_rate_to_solar(), false);
    break;
  case drive_rate_enum::lunar:
    send_command_to_mount(zwoc::cmd_set_tracking_rate_to_lunar(), false);
    break;
  default:
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        fmt::format("Unsupported tracking rate: {}", tracking_rate));
  }
  return 0;
}

std::vector<drive_rate_enum> zwo_am5_telescope::tracking_rates() {
  throw_if_not_connected();
  return std::vector<drive_rate_enum>{drive_rate_enum::sidereal,
                                      drive_rate_enum::solar,
                                      drive_rate_enum::lunar};
}

std::string zwo_am5_telescope::utc_date() {
  throw_if_not_connected();

  using namespace std::chrono_literals;

  auto raw_resp_t = send_command_to_mount(zwoc::cmd_get_date_and_time_and_tz());
  spdlog::trace("raw data from scope: {}", raw_resp_t);

  auto tz_resp = send_command_to_mount(zwoc::cmd_get_timezone());
  zwor::shh_mm tz_data = zwor::parse_shh_mm_response(tz_resp);

  // This is recreating a strange behavior that the ASCOM driver does
  if (tz_data.plus_or_minus == '+')
    tz_data.plus_or_minus = '-';
  else
    tz_data.plus_or_minus = '+';

  auto dst_resp = send_command_to_mount(zwoc::cmd_get_daylight_savings());
  int d_savings = zwor::parse_standard_response(dst_resp);

  spdlog::trace("mount believes daylight savings is: {}", d_savings);
  auto date_resp = send_command_to_mount(zwoc::cmd_get_date());
  zwor::mm_dd_yy date_data = zwor::parse_mm_dd_yy_response(date_resp);

  auto time_resp = send_command_to_mount(zwoc::cmd_get_time());
  zwor::hh_mm_ss time_data = zwor::parse_hh_mm_ss_response(time_resp);

  // This particular section is using the older time / date stuff
  // I think this should be refactored to use the date stuff
  std::tm tm{}; // Zero initialise

  tm.tm_year = date_data.yy + 100;
  tm.tm_mon = date_data.mm - 1;
  tm.tm_mday = date_data.dd;
  tm.tm_hour = time_data.hh;
  tm.tm_min = time_data.mm;
  tm.tm_sec = time_data.ss;

  spdlog::trace("tm.tm_gmtoff: {}", tm.tm_gmtoff);

  std::time_t tt = std::mktime(&tm);

  spdlog::trace("local mount time: {0:%F} {0:%T} {0:%Z}", tm);
  spdlog::trace("tm.tm_gmtoff: {}", tm.tm_gmtoff);

  auto g_t = std::gmtime(&tt);
  // auto g_tt = std::mktime(g_t);
  auto gm_time = std::asctime(g_t);
  spdlog::trace("utc mount time: {0:%F} {0:%T} UTC", *g_t);

  //  2016-03-04T17:45:31.1234567Z
  return fmt::format("{0:%F}T{0:%T}.000000Z", *g_t);
}

// Expects format like:
//  2016-03-04T17:45:31.1234567Z
int zwo_am5_telescope::set_utc_date(const std::string &utc_date_str) {
  throw_if_not_connected();
  using namespace std::chrono_literals;
  std::istringstream stream{utc_date_str};
  date::sys_time<std::chrono::milliseconds> t;
  stream >> date::parse("%FT%T%Z", t);
  spdlog::debug("Parsed date: {0:%F} {0:%T} {0:%Z}", t);

  auto cur_tz = date::current_zone();
  auto zoned_time = date::make_zoned(cur_tz, t);

  spdlog::debug("Local date: {0:%F} {0:%T} {0:%Z}",
                zoned_time.get_local_time());

  auto info = zoned_time.get_info();

  int daylight_savings = 0;
  if (info.save > 0min)
    daylight_savings = 1;

  auto offset_minutes = date::floor<std::chrono::minutes>(
      zoned_time.get_local_time().time_since_epoch() - t.time_since_epoch());

  if (daylight_savings == 1) {
    offset_minutes -= 60min;
  }

  // Set DST to 0
  send_command_to_mount(zwoc::cmd_set_daylight_savings(0), true, '\0');

  spdlog::debug("calculated offset: {}", offset_minutes);

  // I believe that ZWO wants the time set as the time without DST
  // and then wants the daylight savings flag set accordingly
  if (daylight_savings == 1)
    zoned_time = date::make_zoned(cur_tz, t - 1h);

  spdlog::debug("Local date after adjusting for ZWO: {0:%F} {0:%T} {0:%Z}",
                zoned_time.get_local_time());

  // cur_tz->get_info()
  int date_mm =
      std::atoi(&fmt::format("{:%m}", zoned_time.get_local_time())[0]);
  int date_dd =
      std::atoi(&fmt::format("{:%d}", zoned_time.get_local_time())[0]);
  int date_yy =
      std::atoi(&fmt::format("{:%y}", zoned_time.get_local_time())[0]);

  int time_hh =
      std::atoi(&fmt::format("{:%H}", zoned_time.get_local_time())[0]);
  int time_mm =
      std::atoi(&fmt::format("{:%M}", zoned_time.get_local_time())[0]);
  int time_ss =
      std::atoi(&fmt::format("{:%S}", zoned_time.get_local_time())[0]);

  char plus_or_minus_tz = '+';
  // Yes...this is backwards...but it is how the ASCOM driver does it
  // in order for the Sidereal time to be calculated correctly.
  if (offset_minutes > 0min)
    plus_or_minus_tz = '-';

  int tz_hh = std::abs(offset_minutes / 60min);
  int tz_mm = 0;
  if (offset_minutes % 60 == 30min)
    tz_mm = 30;

  auto resp = send_command_to_mount(
      zwoc::cmd_set_timezone(plus_or_minus_tz, tz_hh), true, '\0');
  if (resp != "1")
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           "Problem setting date with driver");

  resp = send_command_to_mount(zwoc::cmd_set_date(date_mm, date_dd, date_yy),
                               true, '\0');
  if (resp != "1")
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           "Problem setting date with driver");

  resp = send_command_to_mount(zwoc::cmd_set_time(time_hh, time_mm, time_ss),
                               true, '\0');
  if (resp != "1")
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           "Problem setting time with driver");

  spdlog::debug("successfully set date, time and time zone on mount");
  spdlog::trace("raw date from mount: {}",
                send_command_to_mount(zwoc::cmd_get_date_and_time_and_tz()));
  spdlog::trace("sidereal time from mount: {}",
                send_command_to_mount(zwoc::cmd_get_sidereal_time()));

  return 0;
}

int zwo_am5_telescope::abort_slew() {
  throw_if_not_connected();
  throw_if_parked();
  spdlog::debug("abort_slew() invoked");
  send_command_to_mount(zwoc::cmd_stop_moving(), false);
  return 0;
}

std::vector<axis_rate>
zwo_am5_telescope::axis_rates(const telescope_axes_enum &axis) {
  throw_if_not_connected();
  // We are just returning one value with a min and max which is degrees per
  // second
  if (axis == telescope_axes_enum::primary ||
      axis == telescope_axes_enum::secondary)
    return std::vector<axis_rate>{axis_rate(0.0042 * 1440.0, 0.0042 * .25)};
  else if (axis == telescope_axes_enum::tertiary)
    return std::vector<axis_rate>{};
  else
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           fmt::format("{} is not a valid axis", axis));
}

bool zwo_am5_telescope::can_move_axis(const telescope_axes_enum &axis) {
  throw_if_not_connected();
  if (axis == telescope_axes_enum::primary)
    return true;
  if (axis == telescope_axes_enum::secondary)
    return true;
  return false;
}

// TODO: perhaps consider implementing this. I think I would have to
// make a guess based on target RA / DEC
pier_side_enum zwo_am5_telescope::destination_side_of_pier(const double &ra,
                                                           const double &dec) {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "This is not currently implemented");
  return pier_side_enum::unknown;
}

// TODO: potentially encapsulate the execution of the telescope command
// in here
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
    if (_telescope.slewing()) {
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
  cmd.run();
}

int zwo_am5_telescope::find_home() {
  using namespace std::chrono_literals;
  throw_if_not_connected();
  throw_if_parked();
  spdlog::debug("find_home invoked");
  send_command_to_mount(zwo_commands::cmd_home_position(), false);
  // block_while_moving();
  // spdlog::debug("initial timer has ended");
  // block_while_moving();
  return 0;
}

// I think this may be able to go away with the new status check
void zwo_am5_telescope::set_is_moving(bool is_moving) {
  std::lock_guard lock(_moving_mtx);
  _moving = is_moving;
};

// I'm making an assumption that east/west is mapped to RA...need to test
int zwo_am5_telescope::move_axis(const telescope_axes_enum &axis,
                                 const double &rate) {

  throw_if_not_connected();
  throw_if_parked();

  spdlog::debug("move_axis called");
  if ((std::abs(rate) < .0042 * .25 || std::abs(rate) > .0042 * 1440) &&
      rate != 0.0)
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        fmt::format("Rate: {} is not within acceptable range", rate));
  switch (axis) {
  // RA
  case telescope_axes_enum::primary:
    set_is_moving(true);
    send_command_to_mount(
        zwoc::cmd_set_moving_speed_precise(std::abs(rate) / .0042), false);
    if (rate > 0)
      send_command_to_mount(zwoc::cmd_move_towards_east(), false);
    else if (rate < 0)
      send_command_to_mount(zwoc::cmd_move_towards_west(), false);
    else {
      set_is_moving(false);
      send_command_to_mount(zwoc::cmd_stop_moving_towards_east(), false);
      send_command_to_mount(zwoc::cmd_stop_moving_towards_west(), false);
    }
    break;
  // Dec
  case telescope_axes_enum::secondary:
    set_is_moving(true);
    send_command_to_mount(
        zwoc::cmd_set_moving_speed_precise(std::abs(rate) / .0042), false);
    if (rate > 0)
      send_command_to_mount(zwoc::cmd_move_towards_north(), false);
    else if (rate < 0)
      send_command_to_mount(zwoc::cmd_move_towards_south(), false);
    else {
      set_is_moving(false);
      send_command_to_mount(zwoc::cmd_stop_moving_towards_south(), false);
      send_command_to_mount(zwoc::cmd_stop_moving_towards_north(), false);
    }
    break;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "unrecognized axis");
  }

  // Why am I doing this again?
  // set_tracking(_tracking_enabled);
  return 0;
}

int zwo_am5_telescope::park() {
  throw_if_not_connected();
  spdlog::debug("park() invoked");
  if (_parked)
    return 0;

  spdlog::trace("sending cmd_park()");
  send_command_to_mount(zwoc::cmd_park(), false);
  spdlog::trace("waiting for mount to park");
  using namespace std::chrono_literals;
  // std::this_thread::sleep_for(30s);  spdlog::debug("blocking while moving");
  block_while_moving();
  spdlog::debug("moving has stopped allegedly");
  _parked = true;
  return 0;
}

void zwo_am5_telescope::pulse_guide_proc(int duration_ms,
                                         char cardinal_direction) {
  using namespace std::chrono_literals;
  auto guide_rate = guide_rate_ascension();

  spdlog::debug("running guide thread");
  _is_pulse_guiding = true;

  // a 10 arc second movement is 10 / 15 seconds of time (ignoring sidereal
  // adjustment) i.e. 0.667 seconds. Also approx 15.042 arcseconds per second
  // of real time.
  //
  // 2000ms in equatorial movement should translate to 2 arc-seconds of movement
  // using the aforementioned conversion, assuming the guide rate is 1x sidereal
  // rate (which won't necessarily be the case), which means we can divide
  // 2000/15.042 which will give us our pulse duration. We should then divide that
  // by our guide rate.
  //
  // However, this doesn't seem to work as expected. I suspect the AM5 driver
  //

  int remaining_duration_ms = duration_ms * .7;

  // We will do multiple guide commands if needed
  if (remaining_duration_ms > 3000) {
    int count = remaining_duration_ms / 3000;
    remaining_duration_ms = remaining_duration_ms % 3000;

    spdlog::debug("issuing multiple guide commands");
    for (int i = 0; i < count; i++) {
      spdlog::debug("guiding {}/{}", i + 1, count);
      send_command_to_mount(zwoc::cmd_guide(cardinal_direction, 3000), false);
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
  }

  send_command_to_mount(
      zwoc::cmd_guide(cardinal_direction, remaining_duration_ms),
      false);
  spdlog::debug("sleeping for {}", remaining_duration_ms);

  std::this_thread::sleep_for(std::chrono::milliseconds(remaining_duration_ms));

  _is_pulse_guiding = false;
  spdlog::debug("pulse guide thread finished");
}

// The big problem with this I think is that the scope stops tracking when
// a move command is issued. Therefore when a Dec guide is issued, it
// will drift in RA.
void zwo_am5_telescope::pulse_guide_proc_using_move(int duration_ms,
                                                    char cardinal_direction) {
  spdlog::debug("running guide thread with move instead of guide");
  // _is_pulse_guiding = true;

  using namespace std::chrono_literals;

  // auto axis = telescope_axes_enum::primary;
  // int rate_direction = 1;

  // if (cardinal_direction == 'n' || cardinal_direction == 's')
  //   axis = telescope_axes_enum::secondary;

  auto side = side_of_pier();

  auto pulse_duration_ms = std::chrono::milliseconds((duration_ms * 10) / 15);

  send_command_to_mount(zwoc::cmd_set_0_5x_sidereal_rate(), false);

  switch (cardinal_direction) {
  case 'e':
    send_command_to_mount(zwoc::cmd_move_towards_east(), false);
    std::this_thread::sleep_for(pulse_duration_ms);
    send_command_to_mount(zwoc::cmd_stop_moving_towards_east(), false);
    break;
  case 'w':
    send_command_to_mount(zwoc::cmd_move_towards_west(), false);
    std::this_thread::sleep_for(pulse_duration_ms);
    send_command_to_mount(zwoc::cmd_stop_moving_towards_west(), false);
    break;
  case 'n':
    send_command_to_mount(zwoc::cmd_move_towards_north(), false);
    std::this_thread::sleep_for(pulse_duration_ms);
    send_command_to_mount(zwoc::cmd_stop_moving_towards_north(), false);
    break;
  case 's':
    send_command_to_mount(zwoc::cmd_move_towards_south(), false);
    std::this_thread::sleep_for(pulse_duration_ms);
    send_command_to_mount(zwoc::cmd_stop_moving_towards_south(), false);
    break;
  default:
    _is_pulse_guiding = false;
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        fmt::format("cardinal direction {} is not valid", cardinal_direction));
  }

  _is_pulse_guiding = false;
  spdlog::debug("pulse guide thread using move finished");
}

//  Duration parameter specifies the amount (in equatorial coordinates)
//  so 3000ms would be 3 arc-seconds I believe
//  I'm wondering if I need to take this and rationalize it into a movement that
//  makes sense
int zwo_am5_telescope::pulse_guide(const guide_direction_enum &direction,
                                   const int32_t &duration_ms) {
  throw_if_not_connected();
  throw_if_parked();
  char cardinal_direction = 0;
  int remaining_duration_ms = duration_ms;
  auto guide_rate = guide_rate_ascension();
  spdlog::debug("pulse_guide() invoked with direction: {}, duration: {}ms with rate:{}",
                direction, duration_ms, guide_rate);
  switch (direction) {
  case guide_direction_enum::guide_east:
    cardinal_direction = 'e';
    break;
  case guide_direction_enum::guide_west:
    cardinal_direction = 'w';
    break;
  case guide_direction_enum::guide_north:
    cardinal_direction = 'n';
    break;
  case guide_direction_enum::guide_south:
    cardinal_direction = 's';
    break;
  default:
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "invalid guide direction");
  }

  _is_pulse_guiding = true;

  spdlog::debug("creating guiding thread");
  _guiding_thread =
      std::thread(std::bind(&zwo_am5_telescope::pulse_guide_proc,
                            this, duration_ms, cardinal_direction));

  _guiding_thread.detach();
  spdlog::debug("guiding thread detached");

  return 0;
}

int zwo_am5_telescope::set_park() {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "This mount does not support setting park position");
  return 0;
}

int zwo_am5_telescope::slew_to_alt_az(const double &alt, const double &az) {
  throw_if_not_connected();
  throw_if_parked();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Alt Az support is not fully implemented yet.");

  return 0;
}

int zwo_am5_telescope::slew_to_alt_az_async(const double &alt,
                                            const double &az) {
  throw_if_not_connected();
  throw_if_parked();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Alt Az support is not fully implemented yet.");
  return 0;
}

int zwo_am5_telescope::slew_to_coordinates(const double &ra,
                                           const double &dec) {
  throw_if_not_connected();
  throw_if_parked();

  spdlog::debug("slew_to_coordinates invoked ra:{} dec:{}", ra, dec);

  set_target_right_ascension(ra);
  set_target_declination(dec);

  _ra_target_set = true;
  _dec_target_set = true;

  zwor::hh_mm_ss converted_ra(ra);
  zwor::sdd_mm_ss converted_dec(dec);

  auto resp = send_command_to_mount(
      zwoc::cmd_set_target_ra_and_dec_and_goto(
          converted_ra.hh, converted_ra.mm, converted_ra.ss,
          converted_dec.plus_or_minus, converted_dec.dd, converted_dec.mm,
          converted_dec.ss),
      true, '0');

  if (resp == "0") {
    block_while_moving();
    return 0;
  } else {
    spdlog::warn("error returned from mount on slew_to_coordinates: {}", resp);
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           fmt::format("mount returned {}", resp));
  }

  return -1;
}

// TODO: need to refactor this to share code
int zwo_am5_telescope::slew_to_coordinates_async(const double &ra,
                                                 const double &dec) {
  throw_if_not_connected();
  throw_if_parked();

  spdlog::debug("slew_to_coordinates_async invoked ra:{} dec:{}", ra, dec);

  zwor::hh_mm_ss converted_ra(ra);
  zwor::sdd_mm_ss converted_dec(dec);

  set_target_right_ascension(ra);
  set_target_declination(dec);

  _ra_target_set = true;
  _dec_target_set = true;

  if (!_tracking_enabled)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "Tracking is not enabled");

  auto resp = send_command_to_mount(
      zwoc::cmd_set_target_ra_and_dec_and_goto(
          converted_ra.hh, converted_ra.mm, converted_ra.ss,
          converted_dec.plus_or_minus, converted_dec.dd, converted_dec.mm,
          converted_dec.ss),
      true, '0');

  if (resp == "0") {
    return 0;
  } else {
    spdlog::warn("error returned from mount on slew_to_coordinates_async: {}",
                 resp);
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           fmt::format("mount returned {}", resp));
  }
  return -1;
}

int zwo_am5_telescope::slew_to_target() {
  throw_if_not_connected();
  throw_if_parked();
  if (!_ra_target_set || !_dec_target_set)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "RA and DEC target must be set");
  spdlog::debug("slew_to_target invoked");
  auto resp = send_command_to_mount(zwoc::cmd_goto(), true, '0');
  if (resp == "0") {
    block_while_moving();
    return 0;
  } else {
    spdlog::warn("error returned from mount on slew_to_coordinates: {}", resp);
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           fmt::format("goto failed. {}", resp));
  }
}

int zwo_am5_telescope::slew_to_target_async() {
  throw_if_not_connected();
  throw_if_parked();
  if (!_ra_target_set || !_dec_target_set)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "RA and DEC target must be set");
  spdlog::debug("slew_to_target_async invoked");
  auto resp = send_command_to_mount(zwoc::cmd_goto(), true, '0');
  if (resp == "0")
    return 0;
  else
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           fmt::format("goto failed. {}", resp));
}

int zwo_am5_telescope::sync_to_alt_az(const double &alt, const double &az) {
  throw_if_not_connected();
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "ZWO mounts do not support alt az sync");
  return 0;
}

int zwo_am5_telescope::sync_to_coordinates(const double &ra,
                                           const double &dec) {
  zwor::hh_mm_ss ra_vals(ra);
  zwor::sdd_mm_ss dec_vals(dec);
  _ra_target_set = true;
  _dec_target_set = true;
  spdlog::debug("sync_to_coordinates invoked ra: {} dec: {}", ra, dec);
  auto resp = send_command_to_mount(zwoc::cmd_set_target_ra_and_dec_and_sync(
      ra_vals.hh, ra_vals.mm, ra_vals.ss, dec_vals.plus_or_minus, dec_vals.dd,
      dec_vals.mm, dec_vals.ss));
  // auto result = zwor::parse_standard_response(resp);
  if (resp == "N/A#")
    return 0;
  else
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("sync_to_coordinates failed with: {}", resp));
}

// TODO: test this properly
int zwo_am5_telescope::sync_to_target() {
  throw_if_not_connected();
  if (!_ra_target_set || !_dec_target_set)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "RA and DEC target must be set");
  spdlog::debug("sync_to_target invoked");
  auto resp = send_command_to_mount(zwoc::cmd_sync());
  // Need to interpret response
  if (resp == "N/A#")
    return 0;
  else
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           fmt::format("sync_to_target failed with: {}", resp));
  return 0;
}

// We will move the telescope back to its home position on this command
int zwo_am5_telescope::unpark() {
  throw_if_not_connected();
  // Move scope back to home position on unpark
  // send_command_to_mount(zwo_commands::cmd_home_position(), false);
  // block_while_moving();
  _parked = false;
  return 0;
}

// TODO: I should do some validation here so that any UX can provide
// feedback about whether or not it was successful
int zwo_am5_telescope::set_serial_device(
    const std::string &serial_device_path) {
  _serial_device_path = serial_device_path;
  return 0;
}

std::string zwo_am5_telescope::get_serial_device_path() {
  return _serial_device_path;
}

std::string zwo_am5_telescope::get_serial_number() {
  throw_if_not_connected();
  auto resp = send_command_to_mount(":GMA#");
  return resp.substr(0, resp.size() - 1);
}
