#include "drivers/primaluce_focuser_rotator.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Key Value structures", "[primaluce_kv_structures]") {
  spdlog::set_level(spdlog::level::trace);

  primaluce_kv_node root;
  esatto_focuser focuser(
      "/dev/serial/by-id/"
      "usb-Silicon_Labs_CP2102N_USB_to_UART_Bridge_Controller_"
      "b2f14184e185eb11ad7b8b1ab7d59897-if00-port0");
  // auto n = root.create_node("foo", "bar");

  SECTION("Basic node creation") {
    root.create_object("req")
        ->create_object("get")
        ->push_param("MODNAME", "")
        ->push_param("EXTRA", "VALUE");
    spdlog::debug(root.dump());
  }

  SECTION("System status") {
    focuser.set_connected(true);
    auto resp =
        focuser.send_command_to_focuser(focuser.get_all_system_data_cmd());
    auto data = nlohmann::json::parse(resp);
    // spdlog::debug(data["res"]);
    for (auto iter : data["res"]["get"].items()) {
      if (iter.value().is_object()) {
        spdlog::debug("|-{}", iter.key());
        for (auto iter2 : iter.value().items()) {
          if (iter2.value().is_object()) {
            spdlog::debug("  |-{}", iter2.key());
            for (auto iter3 : iter2.value().items()) {
              spdlog::debug("    |-{} -> {}", iter3.key(), iter3.value());
            }
          } else {
            spdlog::debug("  |-{} -> {}", iter2.key(), iter2.value());
          }
        }
      } else {
        spdlog::debug("|-{} -> {}", iter.key(), iter.value());
      }
    }
  }

  SECTION("MOT1 status") {
    focuser.set_connected(true);
    auto resp =
      focuser.send_command_to_focuser(focuser.get_mot1_status_cmd());
    auto data = nlohmann::json::parse(resp);
    // spdlog::debug(data["res"]);
    for (auto iter : data["res"]["get"].items()) {
      if (iter.value().is_object()) {
        spdlog::debug("|-{}", iter.key());
        for (auto iter2 : iter.value().items()) {
          if (iter2.value().is_object()) {
            spdlog::debug("  |-{}", iter2.key());
            for (auto iter3 : iter2.value().items()) {
              spdlog::debug("    |-{} -> {}", iter3.key(), iter3.value());
            }
          } else {
            spdlog::debug("  |-{} -> {}", iter2.key(), iter2.value());
          }
        }
      } else {
        spdlog::debug("|-{} -> {}", iter.key(), iter.value());
      }
    }
  }
}
