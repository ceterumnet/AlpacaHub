#ifndef ALPACA_HUB_COMMON_HPP
#define ALPACA_HUB_COMMON_HPP

#include "fmt/ostream.h"
#include <variant>

#include "nlohmann/json.hpp"
#include <cstdint>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/ostr.h>

template <> struct fmt::formatter<nlohmann::json> : fmt::ostream_formatter {};

// This is to facilitate serialization of our json of the types we commonly use
namespace nlohmann {

// Try to set the value of type T into the variant data
// if it fails, do nothing
template <typename T, typename... Ts>
void variant_from_json(const json &j, std::variant<Ts...> &data) {
  try {
    data = j.get<T>();
  } catch (...) {
  }
}

template <typename... Ts> struct adl_serializer<std::variant<Ts...>> {
  static void to_json(nlohmann::json &j, const std::variant<Ts...> &data) {
    // Will call j = v automatically for the right type
    std::visit([&j](const auto &v) { j = v; }, data);
  }

  static void from_json(const json &j, std::variant<Ts...> &data) {
    // Call variant_from_json for all types, only one will succeed
    (variant_from_json<Ts>(j, data), ...);
  }
};
}

#endif
