#include "common/alpaca_exception.hpp"
#include "drivers/pegasus_alpaca_focuscube3.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Serial connection attempt", "[set_connected_pegasus_focuser]") {
  spdlog::set_level(spdlog::level::trace);
  pegasus_alpaca_focuscube3 focuser;
  SECTION("Invalid serial device path") {
    focuser.set_serial_device("/dev/ttyARGGWTF");
    REQUIRE_THROWS_AS(focuser.set_connected(true), alpaca_exception);
  }

  SECTION("Valid serial device path") {
    focuser.set_serial_device("/dev/serial/by-id/usb-PegasusAstro_FocusCube3_48:27:e2:44:73:14-if00");
    focuser.set_connected(true);
    // focuser.send_command_to_focuser("FV\n");
    // focuser.send_command_to_focuser("FA\n");
    // focuser.send_command_to_focuser("FA\n");
    // focuser.send_command_to_focuser("FA\n");
    REQUIRE(focuser.connected());
  }
}

TEST_CASE("Serial Device Listing Tests", "[serial_devices]") {
  spdlog::set_level(spdlog::level::debug);
  spdlog::info("running serial devices test");
  pegasus_alpaca_focuscube3::serial_devices();
}
