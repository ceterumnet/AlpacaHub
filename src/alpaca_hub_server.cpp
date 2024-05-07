#include "alpaca_hub_server.hpp"
#include "alpaca_exception.hpp"
#include "i_alpaca_camera.hpp"
#include "image_bytes.hpp"
#include "nlohmann/json_fwd.hpp"
#include "restinio/cast_to.hpp"
#include "restinio/http_headers.hpp"
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "uuid/uuid.h"
#include <asio/ip/udp.hpp>
#include <cctype>

class http_server_logger_t {
public:
  http_server_logger_t(std::shared_ptr<spdlog::logger> logger)
      : m_logger{std::move(logger)} {}

  template <typename Builder> void trace(Builder &&msg_builder) {
    log_if_enabled(spdlog::level::trace, std::forward<Builder>(msg_builder));
  }

  template <typename Builder> void info(Builder &&msg_builder) {
    log_if_enabled(spdlog::level::info, std::forward<Builder>(msg_builder));
  }

  template <typename Builder> void warn(Builder &&msg_builder) {
    log_if_enabled(spdlog::level::warn, std::forward<Builder>(msg_builder));
  }

  template <typename Builder> void error(Builder &&msg_builder) {
    log_if_enabled(spdlog::level::err, std::forward<Builder>(msg_builder));
  }

private:
  template <typename Builder>
  void log_if_enabled(spdlog::level::level_enum lv, Builder &&msg_builder) {
    if (m_logger->should_log(lv)) {
      m_logger->log(lv, msg_builder());
    }
  }

  //! Logger object.
  std::shared_ptr<spdlog::logger> m_logger;
};

static std::map<std::string, std::string> device_uuid_lookup{
    {std::string("QHY9S-M"),
     std::string("25dd748b-2433-45b7-a219-05bf5bd716e4")},
    {std::string("QHYFW"),
     std::string("f0b60810-a45e-4035-b880-ec4d1fb02edf")}};

// static auto common_logger =
//   spdlog::basic_logger_mt(logger_name, );

// We need these for 2 reasons:
//  1. If we ever want to log source location etc...these are compile time
//     loggers
//  2. During destruction of the http logger, restinio attempts to destroy the
//     sink
static auto SPD_CONSOLD = spdlog::stdout_color_mt("console");
static auto ERR_LOGGER = spdlog::stderr_color_mt("stderr");

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

template <typename RESP> RESP init_resp(RESP resp) {
  resp.append_header("Server", "AlpacaHub /v.0.1");
  resp.append_header_date_field().append_header(
      "Content-Type", "application/json; charset=utf-8");
  return resp;
}

template <typename RESP> RESP init_resp_imagebytes(RESP resp) {
  resp.append_header("Server", "AlpacaHub /v.0.1");
  resp.append_header_date_field().append_header("Content-Type",
                                                "application/imagebytes");
  return resp;
}

template <typename RESP> RESP init_resp_html(RESP resp) {
  resp.append_header("Server", "AlpacaHub /v.0.1");
  resp.append_header_date_field().append_header("Content-Type",
                                                "text/html; charset=utf-8");
  return resp;
}

static int32_t foo = alpaca_exception::INVALID_VALUE;

// Contains a dictionary of all the devices laid out like this:
// <device_type_key>
//        devices in vector
// so for cameras:
// device_map['camera'][0] should map accordingly to the first camera
// device_map['camera'][1] should map accordingly to the second camera...
//
// After working on this for a bit, I'm not sure this approach really
// buys much. I might be better off having a collection for each of the
// supported device types
static std::map<std::string, std::vector<std::shared_ptr<i_alpaca_device>>>
    device_map;

static std::mutex server_tx_mtx;
static uint32_t server_transaction_number = 1;

static uint32_t get_next_transaction_number() {
  std::lock_guard lock(server_tx_mtx);
  return server_transaction_number++;
}

inline std::string slurp(const std::string &path) {
  auto fb = std::ifstream(path, std::ifstream::in);
  std::ostringstream buffer;
  buffer << fb.rdbuf();
  return buffer.str();
}

// TODO: add client id and clienttransaction id to this as well
using device_mgmt_list_entry_t =
    std::map<std::string, std::variant<std::string, int>>;

typedef std::map<
    std::string,
    std::variant<bool, uint8_t, uint16_t, uint32_t, int, long, double,
                 std::string, std::vector<std::string>, std::vector<int>,
                 std::vector<std::vector<uint32_t>>, std::vector<device_mgmt_list_entry_t>, std::map<std::string, std::string> >>
    device_param_t;

// This is a data structure that allows us to pass our data between
// the various rest handlers
struct device_instance_data {
  std::string device_name;
  std::string device_type;
  uint8_t device_num = 0;
  // TODO: I don't think I'm really using this
  std::vector<std::string> error_messages;
  uint32_t client_tx_num = 0;
  uint32_t server_tx_num = 0;
  uint32_t client_id_num = 0;

  device_param_t response_map;

  std::shared_ptr<i_alpaca_device> device;
};

struct device_data_factory {
  using data_t = device_instance_data;

  void make_within(restinio::extra_data_buffer_t<data_t> buf) {
    new (buf.get()) data_t{};
  }
};

using device_request_handle_t =
    restinio::generic_request_handle_t<device_data_factory::data_t>;

using device_router_t = restinio::router::generic_express_router_t<
    restinio::router::std_regex_engine_t, device_data_factory>;

using easy_device_router_t =
    restinio::router::generic_easy_parser_router_t<device_data_factory>;

struct chained_device_traits_t
    : public restinio::traits_t<
          restinio::asio_timer_manager_t,
          // restinio::single_threaded_ostream_logger_t,
          http_server_logger_t,
          restinio::sync_chain::fixed_size_chain_t<2, device_data_factory>> {
  using extra_data_factory_t = device_data_factory;
};

template <typename T>
std::basic_string<T> lowercase(const std::basic_string<T> &s) {
  std::basic_string<T> s2 = s;
  std::transform(s2.begin(), s2.end(), s2.begin(),
                 [](const T v) { return static_cast<T>(std::tolower(v)); });
  return s2;
};

class api_v1_handler {
public:
  using device_num_t = uint64_t;

  // template <typename Device_T = i_alpaca_device>
  auto on_get_device_common(const device_request_handle_t &req,
                            std::string device_type, device_num_t device_num,
                            std::string rest_of_path) {
    spdlog::trace("hitting common GET device handler");

    auto device_data = req->extra_data();

    if (device_type != "camera" && device_type != "telescope" &&
        device_type != "focuser" && device_type != "filterwheel") {
      std::string err_msg =
          fmt::format("Unsupported device_type: {0}\nDetails: [device "
                      "parameter in path must "
                      "be one of camera, telescope, filterwheel, or focuser.]",
                      device_type);

      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(err_msg)
          .done();
    }

    req->extra_data().device_type = device_type;

    try {
      req->extra_data().device_num = device_num;
    } catch (const std::exception &ex) {
      spdlog::warn("Exception: {0}", ex.what());
      std::string err_msg =
          fmt::format("Problem processing device number {0}, device must "
                      "be a number.\n  Details: [{1}]",
                      device_num, ex.what());
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(err_msg)
          .done();
    }

    try {
      auto d = device_map[device_type].at(device_num);
    } catch (const std::exception &ex) {
      spdlog::warn("Exception: {0}", ex.what());

      std::string err_msg =
          fmt::format("There is no {0} at {1}\nDetails: [{2}]", device_type,
                      device_num, ex.what());
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(err_msg)
          .done();
    }

    try {
      // TODO: This is camera specific as written, but this code should
      // work for all device types as far as a generic device pre-handler
      if (!device_map[device_type].empty()) {
        auto the_device = std::dynamic_pointer_cast<i_alpaca_device>(
            device_map[device_type][device_num]);
        req->extra_data().device = the_device;

        // This all needs to be refactored to be shared code...all of
        // the device number stuff through the basic params that are
        // standard
        auto raw_qp = restinio::parse_query(req->header().query());

        std::map<std::string, std::string> qp;
        for (auto &query_param : raw_qp) {
          std::string key(lowercase(std::string(query_param.first)));
          qp[key] = query_param.second;
        }

        auto &response_map = req->extra_data().response_map;

        response_map["ErrorNumber"] = 0;
        response_map["ErrorMessage"] = "";

        try {
          response_map["ClientID"] =
              restinio::cast_to<uint32_t>(qp["clientid"]);

        } catch (std::exception &ex) {
          spdlog::warn("ClientID not provided or not formatted correctly");
        }

        try {
          response_map["ClientTransactionID"] =
              restinio::cast_to<uint32_t>(qp["clienttransactionid"]);

        } catch (std::exception &ex) {
          spdlog::warn(
              "ClientTransactionID not provided or not formatted correctly");
        }

        response_map["ServerTransactionID"] = get_next_transaction_number();
      }
    } catch (const std::exception &ex) {
      spdlog::error("General Error Occurred.\nDetails: [{0}]", ex.what());
      return init_resp(req->create_response()).set_body(ex.what()).done();
    }

    return restinio::request_not_handled();
  }

  auto on_put_device_common(const device_request_handle_t &req,
                            std::string device_type, device_num_t device_num,
                            std::string rest_of_path) {
    spdlog::trace("hitting common PUT device handler");

    if (device_type != "camera" && device_type != "telescope" &&
        device_type != "focuser" && device_type != "filterwheel") {
      std::string err_msg =
          fmt::format("Unsupported device_type: {0}\nDetails: [device "
                      "parameter in path must "
                      "be one of camera, telescope, filterwheel, or focuser.]",
                      device_type);

      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(err_msg)
          .done();
    }

    if (device_num < 0) {
      spdlog::warn("Negative device number passed: {0}", device_num);
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body("Invalid device number")
          .done();
    }

    try {
      auto d = device_map[device_type].at(device_num);
    } catch (const std::exception &ex) {
      spdlog::warn("Exception: {0}", ex.what());

      std::string err_msg =
          fmt::format("There is no {0} at {1}\nDetails: [{2}]", device_type,
                      device_num, ex.what());
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(err_msg)
          .done();
    }

    auto the_device = std::dynamic_pointer_cast<i_alpaca_device>(
        device_map[device_type][device_num]);
    req->extra_data().device = the_device;

    auto b = req->body();
    spdlog::trace("body: \n{0}", b);

    const auto qp = restinio::parse_query(b);

    auto &response_map = req->extra_data().response_map;

    response_map["ServerTransactionID"] = get_next_transaction_number();
    response_map["ErrorNumber"] = 0;
    response_map["ErrorMessage"] = "";
    // End common PUT code for device

    try {
      response_map["ClientID"] = restinio::cast_to<uint32_t>(qp["ClientID"]);
    } catch (std::exception &ex) {
      spdlog::warn("ClientID not provided or not formatted correctly");
    }

    try {
      response_map["ClientTransactionID"] =
          restinio::cast_to<uint32_t>(qp["ClientTransactionID"]);
    } catch (std::exception &ex) {
      spdlog::warn(
          "ClientTransactionID not provided or not formatted correctly");
    }

    return restinio::request_not_handled();
  }

  template <typename Device_T, auto F>
  auto device_handler(const device_request_handle_t &req, std::string hint) {
    std::shared_ptr<Device_T> the_device =
        std::dynamic_pointer_cast<Device_T>(req->extra_data().device);

    spdlog::debug("device ptr: {0}", fmt::ptr(the_device.get()));

    if (!the_device) {
      spdlog::error("device pointer is null. aborting request");
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body("major fault occurred.")
          .done();
    }

    auto &response_map = req->extra_data().response_map;
    auto f = std::bind(F, the_device);

    spdlog::trace("Generic handler is invoking {0}", hint);

    try {
      req->extra_data().response_map["Value"] = f();
    } catch (alpaca_exception &ex) {
      spdlog::warn("Generic handler received alpaca_exception. error_code: {0} "
                   "error_message: {1}",
                   ex.error_code(), ex.what());
      response_map["ErrorNumber"] = ex.error_code();
      response_map["ErrorMessage"] = ex.what();
    }

    return init_resp(req->create_response())
        .set_body(nlohmann::json(req->extra_data().response_map).dump())
        .done();
  };

  template <typename Device_T, auto F> auto create_handler(std::string hint) {
    return [=](auto req, auto) {
      spdlog::trace("Creating generic handler {0}", hint);
      return this->device_handler<Device_T, F>(req, hint);
    };
  };

  // I'm trying to get rid of a ton of boilerplate code for common PUT handling
  // this is a templated approach as a first attempt
  template <typename Input_T, typename Device_T, auto Device_F>
  auto create_put_handler(std::string parameter_key = "",
                          bool validate_True_False = false) {

    return [parameter_key, validate_True_False](auto req, auto) {
      auto &response_map = req->extra_data().response_map;
      const auto qp = restinio::parse_query(req->body());
      std::variant<bool, std::string, int, uint32_t, double> input_variant;

      if (validate_True_False) {
        try {
          auto raw_value = qp[parameter_key];
          input_variant = false;
          if (raw_value == "True")
            input_variant = true;
          else if (raw_value == "False")
            input_variant = false;
          else {
            response_map["ErrorNumber"] = alpaca_exception::INVALID_VALUE;
            response_map["ErrorMessage"] =
                fmt::format("Invalid Value for {0} of {1} passed",
                            parameter_key, raw_value);
            return init_resp(
                       req->create_response(restinio::status_bad_request()))
                .set_body(nlohmann::json(response_map).dump())
                .done();
          }
        } catch (std::exception &ex) {
          response_map["ErrorNumber"] = alpaca_exception::INVALID_VALUE;
          response_map["ErrorMessage"] =
              fmt::format("Problem with parameters: {0}", ex.what());
          return init_resp(req->create_response(restinio::status_bad_request()))
              .set_body(nlohmann::json(response_map).dump())
              .done();
        }

      } else {
        try {

          if (std::is_same<Input_T, std::string>::value) {
            input_variant = restinio::cast_to<std::string>(qp[parameter_key]);
          }

          if (std::is_same<Input_T, int>::value) {
            input_variant = restinio::cast_to<int>(qp[parameter_key]);
          }

          if (std::is_same<Input_T, uint32_t>::value) {
            input_variant = restinio::cast_to<uint32_t>(qp[parameter_key]);
          }

          if (std::is_same<Input_T, double>::value) {
            input_variant = restinio::cast_to<double>(qp[parameter_key]);
          }

        } catch (std::exception &ex) {
          response_map["ErrorNumber"] = alpaca_exception::INVALID_VALUE;
          response_map["ErrorMessage"] =
              fmt::format("Invalid Value for {0} passed", parameter_key);
          return init_resp(req->create_response(restinio::status_bad_request()))
              .set_body(nlohmann::json(response_map).dump())
              .done();
        }
      }

      std::shared_ptr<Device_T> the_device =
          std::dynamic_pointer_cast<Device_T>(req->extra_data().device);

      if constexpr (std::is_same<Input_T, void>::value) {
        try {
          auto f = std::bind(Device_F, the_device);

          if (f() == 0) {
            return init_resp(req->create_response())
                .set_body(nlohmann::json(response_map).dump())
                .done();
          } else {
            response_map["ErrorNumber"] = -1;
            response_map["ErrorMessage"] = fmt::format(
                "Failed to set device parameter {0}", parameter_key);
            return init_resp(req->create_response())
                .set_body(nlohmann::json(response_map).dump())
                .done();
          }
        } catch (alpaca_exception &ex) {
          response_map["ErrorNumber"] = ex.error_code();
          response_map["ErrorMessage"] = ex.what();
          return init_resp(req->create_response())
              .set_body(nlohmann::json(response_map).dump())
              .done();
        }
      } else {
        auto f = std::bind(Device_F, the_device, std::placeholders::_1);
        auto input_value = std::get<Input_T>(input_variant);

        try {
          if (f(input_value) == 0) {
            return init_resp(req->create_response())
                .set_body(nlohmann::json(response_map).dump())
                .done();
          } else {
            response_map["ErrorNumber"] = -1;
            response_map["ErrorMessage"] = fmt::format(
                "Failed to set device parameter {0}", parameter_key);
            return init_resp(req->create_response())
                .set_body(nlohmann::json(response_map).dump())
                .done();
          }
        } catch (alpaca_exception &ex) {
          response_map["ErrorNumber"] = ex.error_code();
          response_map["ErrorMessage"] = ex.what();
          return init_resp(req->create_response())
              .set_body(nlohmann::json(response_map).dump())
              .done();
        }
      }
    };
  };

  template <typename T, auto F>
  void add_route_to_router(const std::shared_ptr<device_router_t> &router,
                           const std::string &route_action) {

    std::string device_type_regex;

    // Restrict the paths
    if (std::is_same<T, i_alpaca_camera>::value) {
      device_type_regex = "camera";
    }

    if (std::is_same<T, i_alpaca_filterwheel>::value) {
      device_type_regex = "filterwheel";
    }

    if (std::is_same<T, i_alpaca_device>::value) {
      device_type_regex = "telescope|camera|focuser|filterwheel";
    }

    std::string base_route_path = fmt::format(
        "/api/v1/:device_type({0})/:device_number/", device_type_regex);
    std::string route_path = base_route_path;
    route_path.append(route_action);

    router->http_get(route_path, this->create_handler<T, F>(route_action));
  };
};

// This handler is meant to be the first in the chain
auto create_device_api_handler() {
  namespace epr = restinio::router::easy_parser_router;
  auto router = std::make_shared<easy_device_router_t>();

  auto handler = std::make_shared<api_v1_handler>();

  auto device_num_p =
      epr::non_negative_decimal_number_p<api_v1_handler::device_num_t>();

  auto device_type_p = epr::path_fragment_p();

  auto base_path =
      epr::path_to_params("/api/v1/", device_type_p, "/", device_num_p, "/",
                          epr::path_fragment_p());

  router->http_get(base_path, [handler](const device_request_handle_t &req,
                                        auto device_num, auto device_type,
                                        auto rest_of_path) {
    return handler->on_get_device_common(req, device_num, device_type,
                                         rest_of_path);
  });

  router->http_put(base_path, [handler](const device_request_handle_t &req,
                                        auto device_num, auto device_type,
                                        auto rest_of_path) {
    return handler->on_put_device_common(req, device_num, device_type,
                                         rest_of_path);
  });

  return [_handler = std::move(router)](const device_request_handle_t &req) {
    return (*_handler)(req);
  };
}

auto server_handler() {
  auto api_handler = std::make_shared<api_v1_handler>();

  auto router = std::make_shared<device_router_t>();

  // GET request to homepage.
  router->http_get("/", [](auto req, auto) {
    return init_resp(req->create_response())
        .set_body("GET request to the homepage.")
        .done();
  });

  // Let's handle rendering static content from ./html
  router->http_get("/html/:page(.+)", [](auto req, auto params) {
    std::string path = "../src/html/";
    // std::string path = "./html/";
    path.append(params["page"]);

    spdlog::debug("Looking for: {0}", path);

    if (std::filesystem::exists(path)) {
      spdlog::debug("file found, rendering: {0}", path);
      std::string data = slurp(path);
      return init_resp_html(req->create_response()).set_body(data).done();
    } else {
      return init_resp_html(req->create_response(restinio::status_not_found()))
          .set_body("Page not found.")
          .done();
    }
  });


  router->http_get("/management/apiversions", [](auto req, auto params) {
    std::vector<int> api_versions{1};
    auto raw_qp = restinio::parse_query(req->header().query());

    std::map<std::string, std::string> qp;
    device_param_t response_map;

    for (auto &query_param : raw_qp) {
      std::string key(lowercase(std::string(query_param.first)));
      qp[key] = query_param.second;
    }

    response_map["Value"] = api_versions;
    response_map["ServerTransactionID"] = get_next_transaction_number();

    try {
      response_map["ClientID"] = restinio::cast_to<uint32_t>(qp["clientid"]);
      response_map["ClientTransactionID"] =
          restinio::cast_to<uint32_t>(qp["clienttransactionid"]);
    } catch (std::exception &ex) {
      spdlog::warn("problem with request: {0}", ex.what());
    }

    return init_resp(req->create_response())
        .set_body(nlohmann::json(response_map).dump())
        .done();
  });

  router->http_get("/management/v1/description", [](auto req, auto params) {
    auto raw_qp = restinio::parse_query(req->header().query());

    std::map<std::string, std::string> qp;
    device_param_t response_map;

    for (auto &query_param : raw_qp) {
      std::string key(lowercase(std::string(query_param.first)));
      qp[key] = query_param.second;
    }

    response_map["Value"] = std::map<std::string, std::string>{
        {std::string("ServerName"), std::string("Dave's Alpaca Hub")},
        {std::string("Manufacturer"), std::string("Dave's Brain")},
        {std::string("ManufacturerVersion"), std::string("v0.1")},
        {std::string("Location"), std::string("Austin, TX")}};

    response_map["ServerTransactionID"] = get_next_transaction_number();

    try {
      response_map["ClientID"] = restinio::cast_to<uint32_t>(qp["clientid"]);
      response_map["ClientTransactionID"] =
          restinio::cast_to<uint32_t>(qp["clienttransactionid"]);
    } catch (std::exception &ex) {
      spdlog::warn("problem with request: {0}", ex.what());
    }


    return init_resp(req->create_response())
        .set_body(nlohmann::json(response_map).dump())
        .done();
  });

  router->http_get(
      "/management/v1/configureddevices", [](auto req, auto params) {
        std::vector<device_mgmt_list_entry_t> device_management_list;
        auto raw_qp = restinio::parse_query(req->header().query());

        device_param_t response_map;
        std::map<std::string, std::string> qp;

        for (auto &query_param : raw_qp) {
          std::string key(lowercase(std::string(query_param.first)));
          qp[key] = query_param.second;
        }

        response_map["ServerTransactionID"] = get_next_transaction_number();

        try {
          response_map["ClientID"] =
              restinio::cast_to<uint32_t>(qp["clientid"]);
          response_map["ClientTransactionID"] =
              restinio::cast_to<uint32_t>(qp["clienttransactionid"]);
        } catch (std::exception &ex) {
          spdlog::warn("problem with request: {0}", ex.what());
        }

        for (auto d_type_iter : device_map) {

          int idx = 0;
          for (auto d_entry : d_type_iter.second) {
            device_mgmt_list_entry_t device_entry;

            device_entry["DeviceType"] = d_type_iter.first;
            device_entry["DeviceName"] = d_entry->name();
            // Kinda kludgy...but I can revisit this as I work on more
            // devices as a collection related stuff
            device_entry["DeviceNumber"] = idx++;
            // Need to think about unique ids here.
            uuid_t device_uuid;

            // device_uuid.resize(36);
            uuid_generate(device_uuid);
            // std::vector<unsigned char> uuid_vec;
            // uuid_vec.assign(*device_uuid, 16);
            std::string device_uuid_str;
            device_uuid_str.resize(36);

            uuid_unparse(device_uuid, &device_uuid_str[0]);

            device_entry["UniqueID"] = d_entry->unique_id();
            device_management_list.push_back(device_entry);
          }
          // device_entry["name"] = iter.second;
        }

        response_map["Value"] = device_management_list;

        return init_resp(req->create_response())

            .set_body(nlohmann::json(response_map).dump())
            .done();
      });
  // Begin unsupported endpoints
  //
  // PUT method for device action, commandbool and commandblind
  // We aren't supporting these custom things at this time
  auto unsupported_response = [](auto req, auto params) {
    std::string err_msg = "Not supported at this time";
    auto &response_map = req->extra_data().response_map;
    response_map["ErrorNumber"] = alpaca_exception::NOT_IMPLEMENTED;
    response_map["ErrorMessage"] = err_msg;
    return init_resp(req->create_response())
        .set_body(nlohmann::json(response_map).dump())
        .done();
  };

  router->http_put("/api/v1/:device_type/:device_number/action",
                   unsupported_response);
  router->http_put("/api/v1/:device_type/:device_number/commandblind",
                   unsupported_response);
  router->http_put("/api/v1/:device_type/:device_number/commandbool",
                   unsupported_response);
  router->http_put("/api/v1/:device_type/:device_number/commandstring",
                   unsupported_response);
  // End unsupported endpoints

  // TODO: I need to see if I can just get rid of this by restricting the good
  // routes with a regex
  std::string bad_device_num_path =
      R"(/api/v1/:device_type/:device_number(-\d+|[a-zA-Z][:alpha:]*)/:anything)";
  router->http_get(bad_device_num_path, [](auto req, auto params) {
    const auto raw_qp = restinio::parse_query(req->header().query());

    std::map<std::string, std::string> qp;
    for (auto &query_param : raw_qp) {
      std::string key(lowercase(std::string(query_param.first)));
      qp[key] = query_param.second;
    }

    auto &response_map = req->extra_data().response_map;

    try {
      response_map["ClientID"] = restinio::cast_to<uint32_t>(qp["clientid"]);
    } catch (std::exception &ex) {
      spdlog::warn("ClientID not provided or not formatted correctly");
    }

    try {
      response_map["ClientTransactionID"] =
          restinio::cast_to<uint32_t>(qp["clienttransactionid"]);

    } catch (std::exception &ex) {
      spdlog::warn(
          "ClientTransactionID not provided or not formatted correctly");
    }

    response_map["ServerTransactionID"] = get_next_transaction_number();

    response_map["ErrorNumber"] = alpaca_exception::INVALID_VALUE;
    response_map["ErrorMessage"] =
        fmt::format("Invalid device number: {0}", params["device_number"]);
    return init_resp(req->create_response(restinio::status_bad_request()))
        .set_body(nlohmann::json(response_map).dump())
        .done();
  });

  router->http_put(bad_device_num_path, [](auto req, auto params) {
    auto &response_map = req->extra_data().response_map;
    const auto raw_qp = restinio::parse_query(req->body());

    std::map<std::string, std::string> qp;
    for (auto &query_param : raw_qp) {
      std::string key(lowercase(std::string(query_param.first)));
      qp[key] = query_param.second;
    }

    try {
      response_map["ClientID"] = restinio::cast_to<uint32_t>(qp["clientid"]);
    } catch (std::exception &ex) {
      spdlog::warn("ClientID not provided or not formatted correctly");
    }

    try {
      response_map["ClientTransactionID"] =
          restinio::cast_to<uint32_t>(qp["clienttransactionid"]);

    } catch (std::exception &ex) {
      spdlog::warn(
          "ClientTransactionID not provided or not formatted correctly");
    }

    response_map["ServerTransactionID"] = get_next_transaction_number();

    response_map["ErrorNumber"] = alpaca_exception::INVALID_VALUE;
    response_map["ErrorMessage"] =
        fmt::format("Invalid device number: {0}", params["device_number"]);
    return init_resp(req->create_response(restinio::status_bad_request()))
        .set_body(nlohmann::json(req->extra_data().response_map).dump())
        .done();
  });

  // GET connected
  api_handler
      ->add_route_to_router<i_alpaca_device, &i_alpaca_device::connected>(
          router, "connected");

  // GET description
  api_handler
      ->add_route_to_router<i_alpaca_device, &i_alpaca_device::description>(
          router, "description");

  // GET driverinfo
  api_handler
      ->add_route_to_router<i_alpaca_device, &i_alpaca_device::driverinfo>(
          router, "driverinfo");

  // GET driverversion
  api_handler
      ->add_route_to_router<i_alpaca_device, &i_alpaca_device::driver_version>(
          router, "driverversion");

  // GET interfaceversion
  api_handler->add_route_to_router<i_alpaca_device,
                                   &i_alpaca_device::interface_version>(
      router, "interfaceversion");

  // GET name
  api_handler->add_route_to_router<i_alpaca_device, &i_alpaca_device::name>(
      router, "name");

  // GET supportedactions
  api_handler->add_route_to_router<i_alpaca_device,
                                   &i_alpaca_device::supported_actions>(
      router, "supportedactions");

  //
  // Camera specific routes:
  //

  // GET bayeroffsetx
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::bayer_offset_x>(
          router, "bayeroffsetx");

  // GET bayeroffsety
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::bayer_offset_y>(
          router, "bayeroffsety");

  // GET binx
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::bin_x>(
      router, "binx");

  // GET biny
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::bin_y>(
      router, "biny");

  // GET camerastate
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::camera_state>(
          router, "camerastate");

  // GET cameraxsize
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::camera_x_size>(
          router, "cameraxsize");

  // GET cameraysize
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::camera_y_size>(
          router, "cameraysize");

  // GET canabortexposure
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::can_abort_exposure>(
      router, "canabortexposure");

  // GET canasymmetricbin
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::can_asymmetric_bin>(
      router, "canasymmetricbin");

  // GET canfastreadout
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::can_fast_readout>(
      router, "canfastreadout");

  // GET cangetcoolerpower
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::can_get_cooler_power>(
      router, "cangetcoolerpower");

  // GET canpulseguide
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::can_pulse_guide>(
          router, "canpulseguide");

  // GET cansetccdtemperature
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::can_set_ccd_temperature>(
      router, "cansetccdtemperature");

  // GET canstopexposure
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::can_stop_exposure>(
      router, "canstopexposure");

  // GET ccdtemperature
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::ccd_temperature>(
          router, "ccdtemperature");

  // GET cooleron
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::cooler_on>(
          router, "cooleron");

  // GET coolerpower
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::cooler_power>(
          router, "coolerpower");

  // GET electronsperadu
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::electrons_per_adu>(
      router, "electronsperadu");

  // GET exposuremax
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::exposure_max>(
          router, "exposuremax");

  // GET exposuremin
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::exposure_min>(
          router, "exposuremin");

  // GET exposureresolution
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::exposure_resolution>(
      router, "exposureresolution");

  // GET fastreadout
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::fast_readout>(
          router, "fastreadout");

  // GET fullwellcapacity
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::full_well_capacity>(
      router, "fullwellcapacity");

  // GET gain
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::gain>(
      router, "gain");

  // GET gainmax
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::gain_max>(
      router, "gainmax");

  // GET gainmin
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::gain_min>(
      router, "gainmin");

  // GET gains
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::gains>(
      router, "gains");

  // GET hasshutter
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::has_shutter>(
          router, "hasshutter");

  // GET heatsinktemperature
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::heat_sink_temperature>(
      router, "heatsinktemperature");

  // Handler for imagearray and imagevariant
  auto image_array_handler = [](auto req, auto) {
    std::string accept_header = req->header().get_field_or(
        restinio::http_field::accept, "application/imagebytes");

    spdlog::trace("accept_header: {0}", accept_header);

    auto cmp_res = accept_header.compare("application/imagebytes");
    auto &response_map = req->extra_data().response_map;
    std::shared_ptr<i_alpaca_camera> the_cam =
        std::dynamic_pointer_cast<i_alpaca_camera>(req->extra_data().device);

    if (!the_cam->image_ready()) {

      // throw std::runtime_error("Image is not ready");
      response_map["ErrorNumber"] = alpaca_exception::INVALID_OPERATION;
      response_map["ErrorMessage"] = "Image is not ready";
      return init_resp(req->create_response())
          .set_body(nlohmann::json(response_map).dump())
          .done();
    }

    auto i2d = the_cam->image_2d();
    response_map["Value"] = i2d;
    response_map["Type"] = 2;
    response_map["Rank"] = 2;

    if (cmp_res > -1) {
      spdlog::debug("Client requested imagebytes");
      std::stringstream stream;

      if (the_cam->bpp() == 8) {
        spdlog::debug("8bpp for imagebytes");
        image_bytes_t<uint8_t> image_bytes;
        image_bytes.client_transaction_number =
            std::get<uint32_t>(response_map["ClientTransactionID"]);
        image_bytes.server_transaction_number =
            std::get<uint32_t>(response_map["ServerTransactionID"]);
        image_bytes.image_element_type = 6;
        image_bytes.transmission_element_type = 6;

        image_bytes.dimension1 = i2d.size();
        image_bytes.dimension2 = i2d[0].size();

        size_t size_1d = i2d.size() * i2d[0].size();
        std::vector<uint8_t> img_1d(size_1d);
        uint8_t *p = &img_1d[0];

        for (const auto &row : i2d) {
          for (auto x : row)
            *p++ = x;
        }

        image_bytes.image_data_1d = img_1d;
        image_bytes.serialize(stream);
        spdlog::debug("image dimensions: {0} x {1}", image_bytes.dimension1,
                      image_bytes.dimension2);

      } else {
        spdlog::debug("16bpp for imagebytes");
        image_bytes_t<uint16_t> image_bytes;
        image_bytes.client_transaction_number =
            std::get<uint32_t>(response_map["ClientTransactionID"]);
        image_bytes.server_transaction_number =
            std::get<uint32_t>(response_map["ServerTransactionID"]);
        image_bytes.image_element_type = 8;
        image_bytes.transmission_element_type = 8;

        image_bytes.dimension1 = i2d.size();
        image_bytes.dimension2 = i2d[0].size();

        size_t size_1d = i2d.size() * i2d[0].size();

        std::vector<uint16_t> img_1d(size_1d);

        uint16_t *p = &img_1d[0];

        for (const auto &row : i2d) {
          for (auto x : row)
            *p++ = x;
        }

        image_bytes.image_data_1d = img_1d;
        image_bytes.serialize(stream);
      }

      spdlog::debug("Image bytes size: {0}", stream.str().size());
      return init_resp_imagebytes(req->create_response())
          .set_body(stream.str())
          .done();
    }

    return init_resp(req->create_response())
        .set_body(nlohmann::json(req->extra_data().response_map).dump())
        .done();
  };

  // GET imagearray
  router->http_get("/api/v1/camera/:device_number/imagearray",
                   image_array_handler);

  // GET imagearrayvariant
  // not sure if I'm gonna have to implement this or not
  router->http_get("/api/v1/camera/:device_number/imagearrayvariant",
                   image_array_handler);

  // GET imageready
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::image_ready>(
          router, "imageready");

  // GET ispulseguiding
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::is_pulse_guiding>(
      router, "ispulseguiding");

  // GET lastexposureduration
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::last_exposure_duration>(
      router, "lastexposureduration");

  // GET lastexposurestarttime
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::last_exposure_start_time>(
      router, "lastexposurestarttime");

  // GET maxadu
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::max_adu>(
      router, "maxadu");

  // GET maxbinx
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::max_bin_x>(
          router, "maxbinx");

  // GET maxbiny
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::max_bin_y>(
          router, "maxbiny");

  // GET numx
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::num_x>(
      router, "numx");

  // GET numy
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::num_y>(
      router, "numy");

  // GET offset
  router->http_get(
      "/api/v1/camera/:device_number/offset",
      api_handler->create_handler<i_alpaca_camera, &i_alpaca_camera::offset>(
          "offset"));

  // GET offsetmax
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::offset_max>(
          router, "offsetmax");

  // GET offsetmin
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::offset_min>(
          router, "offsetmin");

  // GET offsets
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::offsets>(
      router, "offsets");

  // GET percentcompleted
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::percent_complete>(
      router, "percentcompleted");

  // GET pixelsizex
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::pixel_size_x>(
          router, "pixelsizex");

  // GET pixelsizey
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::pixel_size_y>(
          router, "pixelsizey");

  // GET readoutmode
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::readout_mode>(
          router, "readoutmode");

  // GET readoutmodes
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::readout_modes>(
          router, "readoutmodes");

  // GET sensorname
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::sensor_name>(
          router, "sensorname");

  // GET sensortype
  api_handler
      ->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::sensor_type>(
          router, "sensortype");

  // GET setccdtemperature
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::get_set_ccd_temperature>(
      router, "setccdtemperature");

  // GET startx
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::start_x>(
      router, "startx");

  // GET starty
  api_handler->add_route_to_router<i_alpaca_camera, &i_alpaca_camera::start_y>(
      router, "starty");

  // GET subexposureduration
  api_handler->add_route_to_router<i_alpaca_camera,
                                   &i_alpaca_camera::subexposure_duration>(
      router, "subexposureduration");

  // PUT abortexposure
  router->http_put(
      "/api/v1/camera/:device_number/abortexposure",
      api_handler->create_put_handler<void, i_alpaca_camera,
                                      &i_alpaca_camera::abort_exposure>());

  // PUT binx
  router->http_put(
      "/api/v1/camera/:device_number/binx",
      api_handler->create_put_handler<int, i_alpaca_camera,
                                      &i_alpaca_camera::set_bin_x>("BinX"));

  // PUT biny
  router->http_put(
      "/api/v1/camera/:device_number/biny",
      api_handler->create_put_handler<int, i_alpaca_camera,
                                      &i_alpaca_camera::set_bin_y>("BinY"));

  // PUT connected
  router->http_put(
      "/api/v1/:device_type/:device_number/connected",
      api_handler->create_put_handler<bool, i_alpaca_device,
                                      &i_alpaca_device::set_connected>(
          "Connected", true));

  // PUT cooleron
  router->http_put(
      "/api/v1/camera/:device_number/cooleron",
      api_handler->create_put_handler<bool, i_alpaca_camera,
                                      &i_alpaca_camera::set_cooler_on>(
          "CoolerOn", true));

  // PUT fastreadout
  router->http_put(
      "/api/v1/camera/:device_number/fastreadout",
      api_handler->create_put_handler<bool, i_alpaca_camera,
                                      &i_alpaca_camera::set_fast_readout>(
          "FastReadout", true));

  // PUT gain
  router->http_put(
      "/api/v1/camera/:device_number/gain",
      api_handler->create_put_handler<int, i_alpaca_camera,
                                      &i_alpaca_camera::set_gain>("Gain"));
  // PUT numx
  router->http_put(
      "/api/v1/camera/:device_number/numx",
      api_handler->create_put_handler<uint32_t, i_alpaca_camera,
                                      &i_alpaca_camera::set_num_x>("NumX"));

  // PUT numy
  router->http_put(
      "/api/v1/camera/:device_number/numy",
      api_handler->create_put_handler<uint32_t, i_alpaca_camera,
                                      &i_alpaca_camera::set_num_y>("NumY"));

  // PUT offset
  router->http_put(
      "/api/v1/camera/:device_number/offset",
      api_handler->create_put_handler<uint32_t, i_alpaca_camera,
                                      &i_alpaca_camera::set_offset>("Offset"));

  // PUT pulseguide
  // Unsupported at this time
  router->http_put(
      "/api/v1/camera/:device_number/pulseguide", [](auto req, auto) {
        auto &response_map = req->extra_data().response_map;

        response_map["ErrorNumber"] = alpaca_exception::NOT_IMPLEMENTED;
        response_map["ErrorMessage"] = "Pulse guiding not implemented";
        return init_resp(req->create_response())
            .set_body(nlohmann::json(response_map).dump())
            .done();
      });

  // PUT readoutmode
  router->http_put(
      "/api/v1/camera/:device_number/readoutmode",
      api_handler->create_put_handler<int, i_alpaca_camera,
                                      &i_alpaca_camera::set_readout_mode>(
          "ReadoutMode"));

  // PUT setccdtemperature
  router->http_put(
      "/api/v1/camera/:device_number/setccdtemperature",
      api_handler->create_put_handler<double, i_alpaca_camera,
                                      &i_alpaca_camera::set_ccd_temperature>(
          "SetCCDTemperature"));

  // PUT startx
  router->http_put(
      "/api/v1/camera/:device_number/startx",
      api_handler->create_put_handler<uint32_t, i_alpaca_camera,
                                      &i_alpaca_camera::set_start_x>("StartX"));

  // PUT starty
  router->http_put(
      "/api/v1/camera/:device_number/starty",
      api_handler->create_put_handler<uint32_t, i_alpaca_camera,
                                      &i_alpaca_camera::set_start_y>("StartY"));

  // PUT subexposureduration
  router->http_put(
      "/api/v1/camera/:device_number/subexposureduration",
      api_handler->create_put_handler<
          double, i_alpaca_camera, &i_alpaca_camera::set_subexposure_duration>(
          "SubExposureDuration"));

  // TODO - figure out how to have this use the common handler creation
  // so that it handles the multiple values
  // PUT startexposure
  router->http_put("/api/v1/camera/:device_number/startexposure", [](auto req,
                                                                     auto) {
    auto &response_map = req->extra_data().response_map;
    const auto qp = restinio::parse_query(req->body());

    double duration_value = 0; // = restinio::cast_to<double>(qp["duration"]);

    try {
      duration_value = restinio::cast_to<double>(qp["Duration"]);
    } catch (std::exception &ex) {
      response_map["ErrorNumber"] = alpaca_exception::INVALID_VALUE;
      response_map["ErrorMessage"] =
          fmt::format("Invalid Value passed for duration");
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(nlohmann::json(response_map).dump())
          .done();
    }

    auto conv_is_light_value = false;

    std::string is_light_value;
    try {
      is_light_value = qp["Light"];
    } catch (std::exception &ex) {
      response_map["ErrorNumber"] = alpaca_exception::INVALID_VALUE;
      response_map["ErrorMessage"] =
          fmt::format("Value of Light is invalid: {0}", ex.what());
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(nlohmann::json(response_map).dump())
          .done();
    }

    if (is_light_value == "True")
      conv_is_light_value = true;
    else if (is_light_value == "False")
      conv_is_light_value = false;
    else {
      response_map["ErrorNumber"] = alpaca_exception::INVALID_VALUE;
      response_map["ErrorMessage"] =
          fmt::format("Value of {0} is invalid", is_light_value);
      return init_resp(req->create_response(restinio::status_bad_request()))
          .set_body(nlohmann::json(response_map).dump())
          .done();
    }

    std::shared_ptr<i_alpaca_camera> the_cam =
        std::dynamic_pointer_cast<i_alpaca_camera>(req->extra_data().device);

    try {
      if (the_cam->start_exposure(duration_value, conv_is_light_value) == 0) {
        return init_resp(req->create_response())
            .set_body(nlohmann::json(response_map).dump())
            .done();
      } else {
        response_map["ErrorNumber"] = -1;
        response_map["ErrorMessage"] = fmt::format("Failed to start exposure");
        return init_resp(req->create_response())
            .set_body(nlohmann::json(response_map).dump())
            .done();
      }
    } catch (alpaca_exception &ex) {
      response_map["ErrorNumber"] = ex.error_code();
      response_map["ErrorMessage"] = ex.what();
      return init_resp(req->create_response())
          .set_body(nlohmann::json(response_map).dump())
          .done();
    }
  });

  // PUT stopexposure
  router->http_put(
      "/api/v1/camera/:device_number/stopexposure",
      api_handler->create_put_handler<void, i_alpaca_camera,
                                      &i_alpaca_camera::stop_exposure>());

  // END camera routes

  // BEGIN focuser routes

  // GET position
  api_handler->add_route_to_router<i_alpaca_filterwheel,
                                   &i_alpaca_filterwheel::position>(router,
                                                                    "position");

  // GET names
  api_handler
      ->add_route_to_router<i_alpaca_filterwheel, &i_alpaca_filterwheel::names>(
          router, "names");

  // GET offsets
  api_handler->add_route_to_router<i_alpaca_filterwheel,
                                   &i_alpaca_filterwheel::focus_offsets>(
      router, "focusoffsets");

  // PUT position
  router->http_put(
      "/api/v1/filterwheel/:device_number/position",
      api_handler->create_put_handler<uint32_t, i_alpaca_filterwheel,
                                      &i_alpaca_filterwheel::set_position>(
          "Position"));

  // END focuser routes
  router->non_matched_request_handler([](auto req) {
    return req->create_response(restinio::status_not_found())
        .append_header_date_field()
        .connection_close()
        .done();
  });

  // return router;
  return
      [_handler = std::move(router)](
          const restinio::generic_request_handle_t<device_data_factory::data_t>
              &req) { return (*_handler)(req); };
}

// TODO: Move this to the camera implementation
std::string camera_state_str(i_alpaca_camera::camera_state_enum c) {
  switch (c) {
  case i_alpaca_camera::camera_state_enum::CAMERA_IDLE:
    return "CAMERA_IDLE";
  case i_alpaca_camera::camera_state_enum::CAMERA_WAITING:
    return "CAMERA_WAITING";
  case i_alpaca_camera::camera_state_enum::CAMERA_EXPOSING:
    return "CAMERA_EXPOSING";
  case i_alpaca_camera::camera_state_enum::CAMERA_READING:
    return "CAMERA_READING";
  case i_alpaca_camera::camera_state_enum::CAMERA_DOWNLOAD:
    return "CAMERA_DOWNLOAD";
  case i_alpaca_camera::camera_state_enum::CAMERA_ERROR:
    return "CAMERA_ERROR";
  default:
    return "unknown";
  }
}

bool _keep_running_discovery = true;

bool keep_running_discovery() {
  std::lock_guard lock(server_tx_mtx);
  return _keep_running_discovery;
}

void cancel_discovery() {
  std::lock_guard lock(server_tx_mtx);
  _keep_running_discovery = false;
}

static asio::io_context io_ctx(1);

void discovery_thread_proc() {
  using namespace std::chrono_literals;

  // Alpaca Discovery code:
  // To listen for IPv4 DISCOVERY MESSAGEs, Alpaca devices should:
  //  1. Listen for IPv4 broadcasts on the DISCOVERY PORT
  //  2. Assess each received message to confirm whether it is a valid
  //  DISCOVERY MESSAGE.
  //  3. If the request is valid, return a RESPONSE MESSAGE indicating the
  //  device's ALPACA PORT.

  // asio::io_context io_ctx(1);
  int alpaca_discovery_port = 32227;

  // Standard discovery message: byte 0-14
  // byte 15
  // version 0-F
  // bytes 16-63 are reserved for future use
  std::string discovery_msg = "alpacadiscovery";

  asio::ip::udp::endpoint udp_endpoint(asio::ip::udp::v4(),
                                       alpaca_discovery_port);

  asio::ip::udp::socket socket(io_ctx, udp_endpoint);
  asio::socket_base::broadcast option(true);

  // asio::socket_base::time
  socket.set_option(option);
  asio::ip::udp::endpoint client;
  spdlog::trace("waiting to receive from udp broadcast");

  char recv_str[1024] = {};
  std::map<std::string, int> disc_resp_map;
  disc_resp_map["AlpacaPort"] = 8080;
  auto disc_resp = nlohmann::json(disc_resp_map).dump();

  while (keep_running_discovery()) {
    // socket.non_blocking()
    asio::socket_base::bytes_readable command(true);
    socket.io_control(command);
    std::size_t bytes_readable = command.get();

    if(bytes_readable > 0) {
      socket.receive_from(asio::buffer(recv_str), client);
      std::string msg_received(recv_str);
      spdlog::trace("received {0}", msg_received);
      if (msg_received == "alpacadiscovery1") {
        spdlog::trace("received discovery message!");

        spdlog::trace("sending back: {0}", disc_resp);
        socket.send_to(asio::buffer(disc_resp), client);
      } else {
        spdlog::trace("received {0}", msg_received);
      }
    }
    std::this_thread::sleep_for(1000ms);

  }
}

int main(int argc, char **argv) {

  std::map<std::string, std::string> cli_args;
  for (int i = 1; i < argc; i++) {
    if (i + 1 < argc) {
      std::string arg = argv[i];
      std::string arg_v = argv[i + 1];
      cli_args[arg] = arg_v;
    }
  }

  // auto http_logger =
  // std::make_shared<spdlog::logger>(spdlog::default_logger());
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  // std::string logger_name = ("common logger");
  auto http_logger = std::make_shared<spdlog::logger>("HTTP", console_sink);
  auto core_logger = std::make_shared<spdlog::logger>("CORE", console_sink);

  // I may try to get some good formatting in place later. I think
  // this is the default format:
  // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

  spdlog::set_default_logger(core_logger);

  auto cli_map_iter = cli_args.find("-l");

  if (cli_map_iter != cli_args.end()) {
    spdlog::info("logging level passed from user");
    auto log_level = cli_map_iter->second;
    if (log_level == "1") {
      spdlog::set_level(spdlog::level::info);
      http_logger->set_level(spdlog::level::info);
    }
    if (log_level == "2") {
      spdlog::set_level(spdlog::level::debug);
      http_logger->set_level(spdlog::level::debug);
    }
    if (log_level == "3") {
      spdlog::set_level(spdlog::level::trace);
      http_logger->set_level(spdlog::level::trace);
    }

  } else {
    spdlog::set_level(spdlog::level::info);
  }

  int thread_pool_size = 1;

  cli_map_iter = cli_args.find("-t");
  if (cli_map_iter != cli_args.end()) {
    auto thread_pool_size_cli = cli_map_iter->second;
    spdlog::info("Thread pool size of {0} requested",
                 std::string(thread_pool_size_cli));
    thread_pool_size = restinio::cast_to<int>(cli_map_iter->second);
  }

  spdlog::info("Starting AlpacaHub");
  device_map["camera"] = std::vector<std::shared_ptr<i_alpaca_device>>();
  device_map["filterwheel"] = std::vector<std::shared_ptr<i_alpaca_device>>();

  try {
    using namespace std::chrono;

    // TODO: consolidate the different SDKs. Right now I'm only doing QHY,
    // but I plan on supporting ZWO and Pegasus as well.
    spdlog::debug("Initializing QHY SDK");

    // TODO: need to actually check result
    auto init_result = qhy_alpaca_camera::InitializeQHYSDK();
    spdlog::debug("  result: {0}", init_result);
    spdlog::info("Number of QHY cameras connected {0}",
                 qhy_alpaca_camera::camera_count());

    auto camera_list = qhy_alpaca_camera::get_connected_cameras();

    spdlog::info("List of QHY Cameras Found: ");
    for (auto &camera_item : camera_list) {
      auto cam_ptr = std::make_shared<qhy_alpaca_camera>(camera_item);
      spdlog::info("  camera: [{0}] added", camera_item);

      device_map["camera"].push_back(cam_ptr);

      if (cam_ptr->has_filter_wheel()) {
        auto fw_ptr = cam_ptr->filter_wheel();
        spdlog::info("filterwheel added");
        device_map["filterwheel"].push_back(fw_ptr);
        spdlog::debug("attempting to invoke connected...");
        spdlog::debug("                     connected:{0}",
                      device_map["filterwheel"][0]->connected());
      }
    }
    // asio::io_context io_ctx(1);
    // auto io_ctx = std::make_shared<asio::io_context>(1);

    // auto discover_f = std::bind(&discovery_thread_proc, &io_context);
    std::thread discovery_thread(&discovery_thread_proc);
    // discovery_thread.detach();

    if (thread_pool_size > 1) {
      spdlog::info("Starting web server in multithreaded mode with {0} threads",
                   thread_pool_size);
      restinio::run(
          restinio::on_thread_pool<chained_device_traits_t>(thread_pool_size)
              .logger(std::move(http_logger))
              .address("0.0.0.0")
              .request_handler(create_device_api_handler(), server_handler())
              .read_next_http_message_timelimit(100s)
              .write_http_response_timelimit(30s)
              .handle_request_timeout(30s));

    } else {
      spdlog::info("Starting web server in singlethreaded mode",
                   thread_pool_size);
      restinio::run(
          restinio::on_this_thread<chained_device_traits_t>() // single
              .logger(std::move(http_logger))
              .address("0.0.0.0")
              .request_handler(create_device_api_handler(), server_handler())
              .read_next_http_message_timelimit(100s)
              .write_http_response_timelimit(30s)
              .handle_request_timeout(30s));
    }

    spdlog::trace("Exiting restinio loop");
    // io_context.run
    spdlog::trace("joining discovery thread");
    cancel_discovery();
    // io_ctx.stop();

    discovery_thread.join();
    spdlog::trace("discovery thread joined");
    // TODO: need to actually check result of this and handle errors

  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }

  spdlog::trace("Release QHY SDK: {0}", qhy_alpaca_camera::ReleaseQHYSDK());
  spdlog::info("AlpacaHub Exiting");
  return 0;
}
