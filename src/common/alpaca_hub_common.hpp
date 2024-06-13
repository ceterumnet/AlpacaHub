#ifndef ALPACA_HUB_COMMON_HPP
#define ALPACA_HUB_COMMON_HPP

#include "fmt/ostream.h"
#include <variant>

#include <nlohmann/json.hpp>
#include <cstdint>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "date/date.h"
#include "date/tz.h"
#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/fmt/chrono.h>


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

template <typename Duration>
inline auto a_localtime(date::local_time<Duration> time) -> std::tm {
  return fmt::localtime(
      std::chrono::system_clock::to_time_t(date::current_zone()->to_sys(time)));
}

template <typename Char, typename Duration>
struct fmt::formatter<date::local_time<Duration>, Char>
    : formatter<std::tm, Char> {
  FMT_CONSTEXPR formatter() {
    this->format_str_ = detail::string_literal<Char, '%', 'F', ' ', '%', 'T'>{};
  }

  template <typename FormatContext>
  auto format(date::local_time<Duration> val, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    using period = typename Duration::period;
    if (period::num != 1 || period::den != 1 ||
        std::is_floating_point<typename Duration::rep>::value) {
      const auto epoch = val.time_since_epoch();
      const auto subsecs = std::chrono::duration_cast<Duration>(
          epoch - std::chrono::duration_cast<std::chrono::seconds>(epoch));

      return formatter<std::tm, Char>::do_format(
          a_localtime(std::chrono::time_point_cast<std::chrono::seconds>(val)),
          ctx, &subsecs);
    }

    return formatter<std::tm, Char>::format(
        a_localtime(std::chrono::time_point_cast<std::chrono::seconds>(val)),
        ctx);
  }
};

#endif
