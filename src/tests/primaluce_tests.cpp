#include "drivers/primaluce_focuser_rotator.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Key Value structures",
          "[primaluce_kv_structures]") {
  spdlog::set_level(spdlog::level::trace);

  primaluce_kv_node root;

  // auto n = root.create_node("foo", "bar");

  SECTION("Basic node creation") {
    root.create_object("req")->create_object("get")->push_param("MODNAME", "")->push_param("EXTRA", "VALUE");
    spdlog::debug(root.dump());
  }

  SECTION("Deeper creation") {
    //root.
  }

}
