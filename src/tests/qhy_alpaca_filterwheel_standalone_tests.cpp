#include "drivers/qhy_alpaca_filterwheel_standalone.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Serial connection attempt", "[set_connected_on_standalone_filterwheel]") {
  spdlog::set_level(spdlog::level::trace);
  auto filterwheels = qhy_alpaca_filterwheel_standalone::get_connected_filterwheels();
  // spdlog::debug(
  //   "Filterwheel devices: {}",
  //   filterwheels);

  SECTION("Invalid serial device path") {
    qhy_alpaca_filterwheel_standalone filterwheel("/dev/foo");
    REQUIRE_THROWS_AS(filterwheel.set_connected(true), alpaca_exception);
  }

  SECTION("Valid serial device path") {
    qhy_alpaca_filterwheel_standalone filterwheel(filterwheels[0]);
    // filterwheel.set_serial_device(filterwheels[0]);
    filterwheel.set_connected(true);
    REQUIRE(filterwheel.connected());
  }
}
