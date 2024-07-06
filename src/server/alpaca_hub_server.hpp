#ifndef ALPACA_HUB_SERVER_HPP
#define ALPACA_HUB_SERVER_HPP

#include "common/alpaca_exception.hpp"
#include "common/alpaca_hub_common.hpp"
#include "common/image_bytes.hpp"
#include "drivers/qhy_alpaca_camera.hpp"
#include "drivers/qhy_alpaca_filterwheel.hpp"
#include "http_server_logger.hpp"
#include "interfaces/i_alpaca_camera.hpp"
#include "interfaces/i_alpaca_device.hpp"
#include "interfaces/i_alpaca_telescope.hpp"
#include "nlohmann/json_fwd.hpp"
#include "restinio/cast_to.hpp"
#include "restinio/common_types.hpp"
#include "restinio/core.hpp"
#include "restinio/helpers/easy_parser.hpp"
#include "restinio/http_headers.hpp"
#include "restinio/request_handler.hpp"
#include "restinio/router/easy_parser_router.hpp"
#include "restinio/router/express.hpp"
#include "restinio/router/std_regex_engine.hpp"
#include "restinio/sync_chain/fixed_size.hpp"
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include <asio/ip/udp.hpp>
#include <bit>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <ios>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

static auto console_sink =
    std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
static auto http_logger =
    std::make_shared<spdlog::logger>("HTTP", console_sink);
static auto core_logger =
    std::make_shared<spdlog::logger>("CORE", console_sink);

namespace alpaca_hub_server {
void enable_client_id_warnings();
namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

template <typename RESP> RESP init_resp(RESP resp);
template <typename RESP> RESP init_resp_imagebytes(RESP resp);
template <typename RESP> RESP init_resp_html(RESP resp);

// I'm not sure I really need a class here...I may just leverage the namespace
// class alpaca_hub_server {
// public:
//   alpaca_hub_server();
//   ~alpaca_hub_server();
// private:
// };

// Declaring this inline to ensure only one copy since it is accessed across
// other object files
inline std::map<std::string, std::vector<std::shared_ptr<i_alpaca_device>>>
    device_map;

static std::mutex server_tx_mtx;
static uint32_t server_transaction_number = 1;

// static uint32_t get_next_transaction_number();

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

using device_param_t = std::map<std::string, device_variant_t>;

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
std::basic_string<T> lowercase(const std::basic_string<T> &s);

using device_request_handler_t =
    std::function<restinio::request_handling_status_t(
        device_request_handle_t, restinio::router::route_params_t)>;

class api_v1_handler {
public:
  using device_num_t = uint64_t;

  // return handler->on_get_device_common(req, device_num, device_type,
  //                                      rest_of_path);

  restinio::request_handling_status_t
  on_get_device_common(const device_request_handle_t &req,
                       std::string device_type, device_num_t device_num,
                       std::string rest_of_path);

  restinio::request_handling_status_t
  on_put_device_common(const device_request_handle_t &req,
                       std::string device_type, device_num_t device_num,
                       std::string rest_of_path);

  template <typename Device_T, auto F>
  device_request_handler_t create_handler(std::string hint);

  template <typename Device_T, auto F>
  restinio::request_handling_status_t
  device_get_handler(const device_request_handle_t &req, std::string hint);

  // I'm trying to get rid of a ton of boilerplate code for common PUT handling
  // this is a templated approach as a first attempt
  template <typename Input_T, typename Device_T, auto Device_F>
  device_request_handler_t device_put_handler(std::string parameter_key = "",
                                              bool validate_True_False = false);

  template <typename T, auto F>
  void add_route_to_router(const std::shared_ptr<device_router_t> &router,
                           const std::string &route_action);
};

using device_lambda_t =
    std::function<device_request_handler_t(device_request_handle_t &)>;

std::function<restinio::request_handling_status_t(device_request_handle_t)>
create_device_api_handler();
std::function<restinio::request_handling_status_t(device_request_handle_t)>
server_handler();

}; // namespace alpaca_hub_server

#endif
