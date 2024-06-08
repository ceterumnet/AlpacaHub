#include "drivers/zwo_am5_commands.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Split On", "[split_on]") {
  using namespace zwo_responses;
  spdlog::set_level(spdlog::level::trace);
  REQUIRE(split_on("foo&bar", "&").size() == 2);
  REQUIRE(split_on("foo&bar", "&")[0] == "foo");
  REQUIRE(split_on("foo&bar", "&")[1] == "bar");
  REQUIRE(split_on("foo&bar&baz", "&")[1] == "bar");
  REQUIRE(split_on("foo&bar&baz", "&")[2] == "baz");
  REQUIRE(split_on("foobar", "&").size() == 1);
}
TEST_CASE("ZWO single value responses", "[zwo_responses_single_value]") {
  using namespace zwo_responses;
  REQUIRE(parse_standard_response("1#") == 1);
  REQUIRE(parse_standard_response("0#") == 0);
  REQUIRE(parse_standard_response("e2#") == 2);
  REQUIRE_THROWS_AS(parse_standard_response("e2"), alpaca_exception);
}

TEST_CASE("ZWO hours, minutes, seconds responses", "[zwo_responses_time]") {
  using namespace zwo_responses;
  REQUIRE(parse_hh_mm_ss_response("12:10:34#").hh == 12);
  REQUIRE(parse_hh_mm_ss_response("12:10:34#").mm == 10);
  REQUIRE(parse_hh_mm_ss_response("12:10:34#").ss == 34);
  REQUIRE(parse_hh_mm_ss_response("02:10:34#").hh == 2);
  REQUIRE(parse_hh_mm_ss_response("02:10:04#").ss == 4);

  REQUIRE_THROWS_AS(parse_hh_mm_ss_response("02:99:123#"), alpaca_exception);
}

TEST_CASE("ZWO degrees, minutes, seconds responses", "[zwo_responses_dec]") {
  using namespace zwo_responses;
  REQUIRE(parse_dd_mm_ss_response("12*10:34#").dd == 12);
  REQUIRE(parse_dd_mm_ss_response("12*10:34#").mm == 10);
  REQUIRE(parse_dd_mm_ss_response("12*10:34#").ss == 34);
  REQUIRE(parse_dd_mm_ss_response("02*10:34#").dd == 2);
  REQUIRE(parse_dd_mm_ss_response("02*10:04#").ss == 4);
  REQUIRE_THROWS_AS(parse_dd_mm_ss_response("02*99:123#"), alpaca_exception);
}

TEST_CASE("ZWO month, day, year responses", "[zwo_responses_date]") {
  using namespace zwo_responses;
  REQUIRE(parse_mm_dd_yy_response("12/10/34#").mm == 12);
  REQUIRE(parse_mm_dd_yy_response("12/10/34#").dd == 10);
  REQUIRE(parse_mm_dd_yy_response("12/10/34#").yy == 34);
  REQUIRE(parse_mm_dd_yy_response("02/10/34#").mm == 2);
  REQUIRE(parse_mm_dd_yy_response("02/10/04#").yy == 4);
  REQUIRE_THROWS_AS(parse_dd_mm_ss_response("02/99/123#"), alpaca_exception);
}

TEST_CASE("ZWO timezone format", "[zwo_responses_tz]") {
  using namespace zwo_responses;
  REQUIRE(parse_shh_mm_response("+05:30#").plus_or_minus == '+');
  REQUIRE(parse_shh_mm_response("+05:30#").hh == 5);
  REQUIRE(parse_shh_mm_response("+05:30#").mm == 30);

  REQUIRE_THROWS_AS(parse_shh_mm_response("+05/30#"), alpaca_exception);
  REQUIRE_THROWS_AS(parse_shh_mm_response("+05:30"), alpaca_exception);
}

TEST_CASE("ZWO latitude format", "[zwo_responses_latitude]") {
  using namespace zwo_responses;
  REQUIRE(parse_sdd_mm_response("+05*30#").plus_or_minus == '+');
  REQUIRE(parse_sdd_mm_response("+05*30#").dd == 5);
  REQUIRE(parse_sdd_mm_response("+05*30#").mm == 30);

  REQUIRE_THROWS_AS(parse_sdd_mm_response("+05*30"), alpaca_exception);
  REQUIRE_THROWS_AS(parse_sdd_mm_response("+05/30#"), alpaca_exception);
}

TEST_CASE("ZWO longitude format", "[zwo_responses_longitude]") {
  using namespace zwo_responses;
  REQUIRE(parse_sddd_mm_response("+105*30#").plus_or_minus == '+');
  REQUIRE(parse_sddd_mm_response("+105*30#").ddd == 105);
  REQUIRE(parse_sddd_mm_response("+005*30#").mm == 30);

  REQUIRE_THROWS_AS(parse_sddd_mm_response("+05*30"), alpaca_exception);
  REQUIRE_THROWS_AS(parse_sddd_mm_response("+05/30#"), alpaca_exception);
}

TEST_CASE("ZWO altitude format", "[zwo_responses_altitude]") {
  using namespace zwo_responses;
  REQUIRE(parse_sdd_mm_ss_response("+34*11:23#").plus_or_minus == '+');
  REQUIRE(parse_sdd_mm_ss_response("+34*11:23#").dd == 34);
  REQUIRE(parse_sdd_mm_ss_response("+34*11:23#").mm == 11);
  REQUIRE(parse_sdd_mm_ss_response("+34*11:23#").ss == 23);
  REQUIRE_THROWS_AS(parse_sdd_mm_ss_response("+34*11:23"), alpaca_exception);
}

TEST_CASE("Degrees, hours, minutes, seconds conversions",
          "[zwo_responses_conversions]") {
  using namespace zwo_responses;

  // sdd_mm_ss data{.plus_or_minus = '-', .dd = 81, .mm = 30, .ss = 30};
  sdd_mm_ss data;
  data.plus_or_minus = '-';
  data.dd = 81;
  data.mm = 30;
  data.ss = 30;

  REQUIRE(data.as_decimal() < -81.508);
  REQUIRE(data.as_decimal() > -81.509);
}

TEST_CASE("Construction of ddd_mm_ss from double",
          "[zwo_responses_ddd_mm_ss_ctor]") {
  using namespace zwo_responses;
  double val = 100.55;
  ddd_mm_ss converted(val);
  spdlog::info("converted: {0} to {1:#03d}*{2:#02d}:{3:#02d}", val,
               converted.ddd, converted.mm, converted.ss);

  spdlog::info("using formatter converted: {0} to {1}", val, converted);
}
