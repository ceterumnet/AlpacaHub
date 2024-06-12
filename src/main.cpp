#include "drivers/zwo_am5_telescope.hpp"
#include "server/alpaca_hub_server.hpp"

// We need these for 2 reasons:
//  1. If we ever want to log source location etc...these are compile time
//     loggers
//  2. During destruction of the http logger, restinio attempts to destroy the
//     sink

static asio::io_context io_ctx(1);

bool _keep_running_discovery = true;

bool keep_running_discovery() {
  std::lock_guard lock(alpaca_hub_server::server_tx_mtx);
  return _keep_running_discovery;
}

void cancel_discovery() {
  std::lock_guard lock(alpaca_hub_server::server_tx_mtx);
  _keep_running_discovery = false;
}

void discovery_thread_proc() {
  spdlog::debug("entering discovery_thread_proc");
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
    asio::socket_base::bytes_readable command(true);
    socket.io_control(command);
    std::size_t bytes_readable = command.get();

    if (bytes_readable > 0) {
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
  spdlog::debug("exiting discovery_thread_proc");
}

int main(int argc, char **argv) {
  spdlog::set_default_logger(core_logger);

  std::map<std::string, std::string> cli_args;
  for (int i = 1; i < argc; i++) {
    if (i + 1 < argc) {
      std::string arg = argv[i];
      std::string arg_v = argv[i + 1];
      cli_args[arg] = arg_v;
    }
  }

  // auto console_sink =
  // std::make_shared<spdlog::sinks::stdout_color_sink_mt>(); auto http_logger =
  // std::make_shared<spdlog::logger>("HTTP", console_sink); auto core_logger =
  // std::make_shared<spdlog::logger>("CORE", console_sink);

  // I may try to get some good formatting in place later. I think
  // this is the default format:
  // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

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
  alpaca_hub_server::device_map["camera"] =
      std::vector<std::shared_ptr<i_alpaca_device>>();
  alpaca_hub_server::device_map["filterwheel"] =
      std::vector<std::shared_ptr<i_alpaca_device>>();
  alpaca_hub_server::device_map["telescope"] =
      std::vector<std::shared_ptr<i_alpaca_device>>();
  alpaca_hub_server::device_map["focuser"] =
      std::vector<std::shared_ptr<i_alpaca_device>>();

  for(auto iter : zwo_am5_telescope::serial_devices()) {
    auto telescope_ptr = std::make_shared<zwo_am5_telescope>();
    telescope_ptr->set_serial_device(iter);
    spdlog::info("Adding ZWO mount at {}", iter);
    alpaca_hub_server::device_map["telescope"].push_back(telescope_ptr);
  }

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

      alpaca_hub_server::device_map["camera"].push_back(cam_ptr);

      if (cam_ptr->has_filter_wheel()) {
        auto fw_ptr = cam_ptr->filter_wheel();
        spdlog::info("filterwheel added");
        alpaca_hub_server::device_map["filterwheel"].push_back(fw_ptr);
        spdlog::debug("attempting to invoke connected...");
        spdlog::debug(
            "                     connected:{0}",
            alpaca_hub_server::device_map["filterwheel"][0]->connected());
      }
    }

    // auto discover_f = std::bind(&discovery_thread_proc, &io_context);
    std::thread discovery_thread(&discovery_thread_proc);
    // discovery_thread.detach();

    if (thread_pool_size > 1) {
      // http_logger->set_level(spdlog::level::info);
      spdlog::info("Starting web server in multithreaded mode with {0} threads",
                   thread_pool_size);
      restinio::run(
          restinio::on_thread_pool<alpaca_hub_server::chained_device_traits_t>(
              thread_pool_size)
              .logger(http_logger)
              .address("0.0.0.0")
              .request_handler(alpaca_hub_server::create_device_api_handler(),
                               alpaca_hub_server::server_handler())
              .read_next_http_message_timelimit(100s)
              .write_http_response_timelimit(30s)
              .handle_request_timeout(30s));

    } else {
      spdlog::info("Starting web server in singlethreaded mode",
                   thread_pool_size);
      restinio::run(
          restinio::on_this_thread<
              alpaca_hub_server::chained_device_traits_t>() // single
              .logger(http_logger)
              .address("0.0.0.0")
              .request_handler(alpaca_hub_server::create_device_api_handler(),
                               alpaca_hub_server::server_handler())
              .read_next_http_message_timelimit(100s)
              .write_http_response_timelimit(30s)
              .handle_request_timeout(30s));
    }

    spdlog::trace("Exiting restinio loop");
    spdlog::trace("joining discovery thread");
    cancel_discovery();

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
