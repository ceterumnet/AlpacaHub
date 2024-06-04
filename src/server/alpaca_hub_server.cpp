#include "alpaca_hub_server.hpp"
#include "interfaces/i_alpaca_telescope.hpp"
#include "restinio/request_handler.hpp"
#include "restinio/router/express.hpp"

namespace alpaca_hub_server {
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

// static uint32_t get_next_transaction_number() {
//   std::lock_guard lock(server_tx_mtx);
//   return server_transaction_number++;
// }

template <typename T>
std::basic_string<T> lowercase(const std::basic_string<T> &s) {
  std::basic_string<T> s2 = s;
  std::transform(s2.begin(), s2.end(), s2.begin(),
                 [](const T v) { return static_cast<T>(std::tolower(v)); });
  return s2;
};

restinio::request_handling_status_t api_v1_handler::on_get_device_common(
    const device_request_handle_t &req, std::string device_type,
    device_num_t device_num, std::string rest_of_path) {
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
    spdlog::warn("size of device_map[{}]: {}", device_type,
                 device_map["camera"].size());
    spdlog::trace("Address of device_map from server {}",
                  fmt::ptr(&alpaca_hub_server::device_map));
    std::string err_msg = fmt::format("There is no {0} at {1}\nDetails: [{2}]",
                                      device_type, device_num, ex.what());
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
    }
  } catch (const std::exception &ex) {
    spdlog::error("General Error Occurred.\nDetails: [{0}]", ex.what());
    return init_resp(req->create_response()).set_body(ex.what()).done();
  }

  return restinio::request_not_handled();
}

restinio::request_handling_status_t api_v1_handler::on_put_device_common(
    const device_request_handle_t &req, std::string device_type,
    device_num_t device_num, std::string rest_of_path) {
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

    std::string err_msg = fmt::format("There is no {0} at {1}\nDetails: [{2}]",
                                      device_type, device_num, ex.what());
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
    spdlog::warn("ClientTransactionID not provided or not formatted correctly");
  }

  return restinio::request_not_handled();
}

template <typename Device_T, auto F>
device_request_handler_t api_v1_handler::create_handler(std::string hint) {
  return [=](auto req, auto) {
    spdlog::trace("Creating generic handler {0}", hint);
    return this->device_handler<Device_T, F>(req, hint);
  };
};

template <typename Device_T, auto F>
restinio::request_handling_status_t
api_v1_handler::device_handler(const device_request_handle_t &req,
                               std::string hint) {
  std::shared_ptr<Device_T> the_device =
      std::dynamic_pointer_cast<Device_T>(req->extra_data().device);

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

template <typename Input_T, typename Device_T, auto Device_F>
device_request_handler_t
api_v1_handler::create_put_handler(std::string parameter_key,
                                   bool validate_True_False) {
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
          response_map["ErrorMessage"] = fmt::format(
              "Invalid Value for {0} of {1} passed", parameter_key, raw_value);
          return init_resp(req->create_response(restinio::status_bad_request()))
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
          response_map["ErrorMessage"] =
              fmt::format("Failed to set device parameter {0}", parameter_key);
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
          response_map["ErrorMessage"] =
              fmt::format("Failed to set device parameter {0}", parameter_key);
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
void api_v1_handler::add_route_to_router(
    const std::shared_ptr<device_router_t> &router,
    const std::string &route_action) {

  std::string device_type_regex;

  // Restrict the paths
  if (std::is_same<T, i_alpaca_camera>::value) {
    device_type_regex = "camera";
  }

  if (std::is_same<T, i_alpaca_filterwheel>::value) {
    device_type_regex = "filterwheel";
  }

  if (std::is_same<T, i_alpaca_telescope>::value) {
    device_type_regex = "telescope";
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

std::function<restinio::request_handling_status_t(device_request_handle_t)>
create_device_api_handler() {
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

std::function<restinio::request_handling_status_t(device_request_handle_t)>
server_handler() {
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

  router->http_get("/management/v1/configureddevices", [](auto req,
                                                          auto params) {
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
      response_map["ClientID"] = restinio::cast_to<uint32_t>(qp["clientid"]);
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
        device_entry["UniqueID"] = d_entry->unique_id();
        device_management_list.push_back(device_entry);
      }
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

  // BEGIN telescope routes

  // GET alignmentmode
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::alignment_mode>(
      router, "alignmentmode");

  // GET altitude
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::altitude>(
          router, "altitude");

  // GET aperturearea
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::aperture_area>(
      router, "aperturearea");

  // GET aperturediameter
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::aperture_diameter>(
      router, "aperturediameter");

  // GET athome
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::at_home>(
          router, "athome");

  // GET atpark
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::at_park>(
          router, "atpark");

  // GET azimuth
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::azimuth>(
          router, "azimuth");

  // GET canfindhome
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_find_home>(
      router, "canfindhome");

  // GET canpark
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::can_park>(
          router, "canpark");

  // GET canpulseguide
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_pulse_guide>(
      router, "canpulseguide");

  // GET cansetdeclinationrate
  api_handler->add_route_to_router<
      i_alpaca_telescope, &i_alpaca_telescope::can_set_declination_rate>(
      router, "cansetdeclinationrate");

  // GET cansetguiderates
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_set_guide_rates>(
      router, "cansetguiderates");

  // GET cansetpark
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_set_park>(
      router, "cansetpark");

  // GET cansetpierside
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_set_pier_side>(
      router, "cansetpierside");

  // GET cansetrightascensionrate
  api_handler->add_route_to_router<
      i_alpaca_telescope, &i_alpaca_telescope::can_set_right_ascension_rate>(
      router, "cansetrightascensionrate");

  // GET cansettracking
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_set_tracking>(
      router, "cansettracking");

  // GET canslew
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::can_slew>(
          router, "canslew");

  // GET canslewasync
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_slew_async>(
      router, "canslewasync");

  // GET canslewaltaz
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_slew_alt_az>(
      router, "canslewaltaz");

  // GET canslewaltazasync
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_slew_alt_az_async>(
      router, "canslewaltazasync");

  // GET cansync
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::can_sync>(
          router, "cansync");

  // GET cansyncaltaz
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_sync_alt_az>(
      router, "cansyncaltaz");

  // GET canunpark
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::can_unpark>(
      router, "canunpark");

  // GET declination
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::declination>(
      router, "declination");

  // GET declinationrate
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::declination_rate>(
      router, "declinationrate");

  // GET doesrefraction
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::does_refraction>(
      router, "doesrefraction");

  // GET equatorialsystem
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::equatorial_system>(
      router, "equatorialsystem");

  // GET focallength
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::focal_length>(
      router, "focallength");

  // GET guideratedeclination
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::guide_rate_declination>(
      router, "guideratedeclination");

  // GET guiderateascension
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::guide_rate_ascension>(
      router, "guiderateascension");

  // GET ispulseguiding
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::is_pulse_guiding>(
      router, "ispulseguiding");

  // GET rightascension
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::right_ascension>(
      router, "rightascension");

  // GET rightascensionrate
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::right_ascension_rate>(
      router, "rightascensionrate");

  // GET sideofpier
  // api_handler->add_route_to_router<i_alpaca_telescope,
  //                                  &i_alpaca_telescope::side_of_pier>(
  //     router, "sideofpier");

  // GET siderealtime
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::sidereal_time>(
      router, "siderealtime");

  // GET siteelevation
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::site_elevation>(
      router, "siteelevation");

  // GET sitelatitude
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::site_latitude>(
      router, "sitelatitude");

  // GET sitelongitude
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::site_longitude>(
      router, "sitelongitude");

  // GET slewing
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::slewing>(
          router, "slewing");

  // GET slewsettletime
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::slew_settle_time>(
      router, "slewsettletime");

  // GET targetdeclination
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::target_declination>(
      router, "targetdeclination");

  // GET targetrightascension
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::target_right_ascension>(
      router, "targetrightascension");

  // GET tracking
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::tracking>(
          router, "tracking");

  // GET trackingrate
  api_handler->add_route_to_router<i_alpaca_telescope,
                                   &i_alpaca_telescope::tracking_rate>(
      router, "trackingrate");

  // GET trackingrates
  // api_handler->add_route_to_router<i_alpaca_telescope,
  //                                  &i_alpaca_telescope::tracking_rates>(
  //     router, "trackingrates");

  // GET utctime
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::utc_time>(
          router, "utctime");

  // GET axisrates
  // api_handler->add_route_to_router<i_alpaca_telescope,
  //                                  &i_alpaca_telescope::axis_rates>(
  //     router, "axisrates");

  // GET canmoveaxis
  // api_handler->add_route_to_router<i_alpaca_telescope,
  //                                  &i_alpaca_telescope::can_move_axis>(
  //     router, "canmoveaxis");

  // TODO: this is a multiparameter GET so I'll need to hand code this one
  // GET destinationsideofpier
  // api_handler->add_route_to_router<i_alpaca_telescope,
  //                                  &i_alpaca_telescope::destination_side_of_pier>(
  //     router, "destinationsideofpier");

  // GET findhome
  api_handler
      ->add_route_to_router<i_alpaca_telescope, &i_alpaca_telescope::find_home>(
          router, "findhome");

  // PUT declinationrate

  // PUT doesrefraction
  router->http_put(
      "/api/v1/telescope/:device_number/doesrefraction",
      api_handler->create_put_handler<bool, i_alpaca_telescope,
                                      &i_alpaca_telescope::set_does_refraction>(
          "DoesRefraction", true));

  // PUT guideratedeclination
  router->http_put(
      "/api/v1/telescope/:device_number/guideratedeclination",
      api_handler
          ->create_put_handler<double, i_alpaca_telescope,
                               &i_alpaca_telescope::set_guide_rate_declination>(
              "guideratedeclination"));


  // PUT guiderateascension
  router->http_put(
      "/api/v1/telescope/:device_number/guiderateascension",
      api_handler
          ->create_put_handler<double, i_alpaca_telescope,
                               &i_alpaca_telescope::set_guide_rate_ascension>(
              "guiderateascension"));

  // PUT rightascensionrate
  router->http_put(
      "/api/v1/telescope/:device_number/rightascensionrate",
      api_handler
          ->create_put_handler<double, i_alpaca_telescope,
                               &i_alpaca_telescope::set_right_ascension_rate>(
              "rightascensionrate"));

  // PUT sideofpier
  // router->http_put(
  //     "/api/v1/telescope/:device_number/sideofpier",
  //     api_handler
  //         ->create_put_handler<int, i_alpaca_telescope,
  //                              &i_alpaca_telescope::set_side_of_pier>(
  //             "sideofpier"));

  // PUT siteelevation

  // PUT sitelatitude

  // PUT sitelongitude

  // PUT slewsettletime

  // PUT targetdeclination

  // PUT targetrightascension

  // PUT tracking

  // PUT trackingrate

  // PUT utctime

  // PUT abortslew

  // PUT moveaxis

  // PUT park

  // PUT pulseguide

  // PUT setpark

  // PUT slewtoaltaz

  // PUT slewtoaltazasync

  // PUT slewtocoordinates

  // PUT slewtotarget

  // PUT slewtotargetasync

  // PUT synctoaltaz

  // PUT synctocoordinates

  // PUT synctotarget

  // END telescope routes

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

}; // namespace alpaca_hub_server
