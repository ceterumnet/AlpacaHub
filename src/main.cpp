#include "drivers/pegasus_alpaca_focuscube3.hpp"
#include "drivers/pegasus_alpaca_ppba.hpp"
#include "drivers/primaluce_focuser_rotator.hpp"
#include "drivers/qhy_alpaca_filterwheel_standalone.hpp"
#include "drivers/zwo_am5_telescope.hpp"
#include "server/alpaca_hub_server.hpp"
#include <ostream>

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

// Alpaca Discovery code:
// To listen for IPv4 DISCOVERY MESSAGEs, Alpaca devices should:
//  1. Listen for IPv4 broadcasts on the DISCOVERY PORT
//  2. Assess each received message to confirm whether it is a valid
//  DISCOVERY MESSAGE.
//  3. If the request is valid, return a RESPONSE MESSAGE indicating the
//  device's ALPACA PORT.

void discovery_thread_proc() {
  spdlog::debug("entering discovery_thread_proc");
  using namespace std::chrono_literals;

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

  bool show_help = false;

  std::map<std::string, std::string> cli_args;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-h") {
      show_help = true;
      break;
    }
    if (i + 1 < argc) {
      std::string arg = argv[i];
      std::string arg_v = argv[i + 1];
      cli_args[arg] = arg_v;
    }
  }

  bool auto_connect_devices = false;
  bool gains_value_mode = false;
  bool offsets_value_mode = false;

  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-cw") {
      alpaca_hub_server::enable_client_id_warnings();
    }

    if (std::string(argv[i]) == "-ac") {
      auto_connect_devices = true;
    }

    if (std::string(argv[i]) == "-ov") {
      offsets_value_mode = true;
    }

    if (std::string(argv[i]) == "-gv") {
      gains_value_mode = true;
    }

    else if (i + 1 < argc) {
      std::string arg = argv[i];
      std::string arg_v = argv[i + 1];
      cli_args[arg] = arg_v;
    }
  }

  if (show_help) {
    // this is kinda ugly. I may need to add a basic args library to avoid this
    // hand wrapped shit.
    std::cout
        << "Usage: AlpacaHub [OPTION]..." << std::endl
        << "Open Source Alpaca Implementation " << std::endl
        << std::endl
        << "  -h                     Displays this help message" << std::endl
        << std::endl
        << "  -l LEVEL               Set the log to LEVEL:" << std::endl
        << "                           1 - INFO (default)" << std::endl
        << "                           2 - DEBUG" << std::endl
        << "                           3 - TRACE" << std::endl
        << "                           4 - TRACE +" << std::endl
        << std::endl
        << "  -t THREAD_COUNT        Sets the number of concurrent threads "
        << std::endl
        << "                         for the web server. 4 threads is the "
           "default."
        << std::endl
        << std::endl
        << "  -d                     Disable Alpaca Discovery" << std::endl
        << std::endl
        << "  -p PORT                Sets the web server to listen on "
        << std::endl
        << std::endl
        << "                         PORT. Default is port 8080" << std::endl
        << "  -cw                    Enable ClientID and ClientTransactionID "
           "warnings"
        << std::endl
        << std::endl
        << "  -ac                    Auto-connect devices on startup "
        << std::endl
        << std::endl
        << "  -ov                    Force camera offset value mode "
        << std::endl
        << std::endl
        << "  -gv                    Force camera gain value mode " << std::endl
        << std::endl;

    return 0;
  }

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
      http_logger->set_level(spdlog::level::debug);
    }
    if (log_level == "4") {
      spdlog::set_level(spdlog::level::trace);
      http_logger->set_level(spdlog::level::trace);
    }
  } else {
    spdlog::set_level(spdlog::level::info);
  }

  int thread_pool_size = 4;

  cli_map_iter = cli_args.find("-t");
  if (cli_map_iter != cli_args.end()) {
    auto thread_pool_size_cli = cli_map_iter->second;
    spdlog::info("Thread pool size of {0} requested",
                 std::string(thread_pool_size_cli));
    thread_pool_size = restinio::cast_to<int>(cli_map_iter->second);
  }

  bool run_discovery = true;

  cli_map_iter = cli_args.find("-d");
  if (cli_map_iter != cli_args.end()) {
    auto run_discovery_arg = cli_map_iter->second;
    run_discovery = false;
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
  alpaca_hub_server::device_map["rotator"] =
      std::vector<std::shared_ptr<i_alpaca_device>>();

  // BEGIN Implementation specific initialization of various device types:
  // TODO: figure out how to setup the implementation specific pieces in a
  // different part of the project to make it more extensible and clean.

  for (auto iter : zwo_am5_telescope::serial_devices()) {
    auto telescope_ptr = std::make_shared<zwo_am5_telescope>();
    telescope_ptr->set_serial_device(iter);
    spdlog::info("Adding ZWO mount at {}", iter);
    alpaca_hub_server::device_map["telescope"].push_back(telescope_ptr);

    if (auto_connect_devices) {
      spdlog::info("Attempting to autoconnect: {}", iter);
      try {
        telescope_ptr->set_connected(true);
      } catch (std::exception &ex) {
        spdlog::error("Failed to autoconnect device: {}", iter);
      }
    }
  }

  try {
    auto focuser_ptr = std::make_shared<esatto_focuser>(
        "/dev/serial/by-id/"
        "usb-Silicon_Labs_CP2102N_USB_to_UART_Bridge_Controller_"
        "b2f14184e185eb11ad7b8b1ab7d59897-if00-port0");
    alpaca_hub_server::device_map["focuser"].push_back(focuser_ptr);

    try {
      focuser_ptr->init_rotator();
    } catch (std::exception &ex) {
      spdlog::error("Failed to init arco device: {}", ex.what());
    }

    spdlog::debug("added focuser esatto focuser at {}",
                  focuser_ptr->get_serial_device_path());

    if (auto_connect_devices) {
      spdlog::info("Attempting to autoconnect Focuser at: {}",
                   focuser_ptr->get_serial_device_path());
      try {
        focuser_ptr->set_connected(true);
      } catch (std::exception &ex) {
        spdlog::error("Failed to autoconnect device: {}",
                      focuser_ptr->get_serial_device_path());
      }
    }

    if (focuser_ptr->arco_present()) {
      focuser_ptr->init_rotator();
      auto rotator_ptr = focuser_ptr->rotator();
      alpaca_hub_server::device_map["rotator"].push_back(rotator_ptr);

      if (auto_connect_devices) {
        spdlog::info("Attempting to autoconnect attached rotator");
        try {
          rotator_ptr->set_connected(true);
        } catch (std::exception &ex) {
          spdlog::error("Failed to autoconnect rotator: {}");
        }
      }
    }

  } catch(std::exception &ex) {
    spdlog::error("failed to add esatto: {}", ex.what());
  }


  // for (auto iter : pegasus_alpaca_focuscube3::serial_devices()) {
  //   auto focuser_ptr = std::make_shared<pegasus_alpaca_focuscube3>();
  //   focuser_ptr->set_serial_device(iter);
  //   spdlog::info("Adding Pegasus Focuser at {}", iter);
  //   alpaca_hub_server::device_map["focuser"].push_back(focuser_ptr);

  //   if (auto_connect_devices) {
  //     spdlog::info("Attempting to autoconnect: {}", iter);
  //     try {
  //       focuser_ptr->set_connected(true);
  //     } catch (std::exception &ex) {
  //       spdlog::error("Failed to autoconnect device: {}", iter);
  //     }
  //   }
  // }

  for (auto iter : pegasus_alpaca_ppba::serial_devices()) {
    auto switch_ptr = std::make_shared<pegasus_alpaca_ppba>();
    switch_ptr->set_serial_device(iter);
    spdlog::info("Adding Pegasus Pocket Powerbox Advanced at {}", iter);
    alpaca_hub_server::device_map["switch"].push_back(switch_ptr);

    if (auto_connect_devices) {
      spdlog::info("Attempting to autoconnect: {}", iter);
      try {
        switch_ptr->set_connected(true);
      } catch (std::exception &ex) {
        spdlog::error("Failed to autoconnect device: {}", iter);
      }
    }
  }

  try {
    using namespace std::chrono;

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

      if (gains_value_mode) {
        spdlog::info("Enabling gain value mode for: {}", camera_item);
        cam_ptr->enable_gains_value_mode();
      }

      if (offsets_value_mode) {
        spdlog::info("Enabling offset value mode for: {}", camera_item);
        cam_ptr->enable_offsets_value_mode();
      }

      alpaca_hub_server::device_map["camera"].push_back(cam_ptr);
      spdlog::info("  camera: [{0}] added", camera_item);

      if (auto_connect_devices) {
        spdlog::info("Attempting to autoconnect: {}", camera_item);
        try {
          cam_ptr->set_connected(true);
        } catch (std::exception &ex) {
          spdlog::error("Failed to autoconnect device: {}", camera_item);
        }
      }

      // This is for QHY camera attached filter wheels
      if (cam_ptr->has_filter_wheel()) {
        auto fw_ptr = cam_ptr->filter_wheel();
        spdlog::info("filterwheel added");
        alpaca_hub_server::device_map["filterwheel"].push_back(fw_ptr);
        spdlog::debug("attempting to invoke connected...");
        spdlog::debug(
            "                     connected:{0}",
            alpaca_hub_server::device_map["filterwheel"][0]->connected());
        if (auto_connect_devices) {
          spdlog::info("Attempting to autoconnect attached filterwheel");
          try {
            cam_ptr->filter_wheel()->set_connected(true);
          } catch (std::exception &ex) {
            spdlog::error("Failed to autoconnect attached filterwheel");
          }
        }
      }
    }

    // TODO: Add standalone filter wheels here
    auto filterwheel_list =
        qhy_alpaca_filterwheel_standalone::get_connected_filterwheels();
    for (auto &fw_item : filterwheel_list) {
      auto fw_ptr =
          std::make_shared<qhy_alpaca_filterwheel_standalone>(fw_item);
      alpaca_hub_server::device_map["filterwheel"].push_back(fw_ptr);
      spdlog::info("filterwheel added at {}", fw_item);
      if (auto_connect_devices) {
        try {
          fw_ptr->set_connected(true);
        } catch (std::exception &ex) {
          spdlog::warn("Failed to autoconnect QHY filterwheel");
        }
      }
    }
    // END Implementation specific initialization of various device types:

    std::thread discovery_thread;
    if (run_discovery)
      discovery_thread = std::thread(&discovery_thread_proc);

    if (thread_pool_size > 1) {
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

    if (run_discovery) {
      spdlog::trace("joining discovery thread");
      cancel_discovery();
      discovery_thread.join();
      spdlog::trace("discovery thread joined");
    }
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }

  spdlog::trace("Release QHY SDK: {0}", qhy_alpaca_camera::ReleaseQHYSDK());
  spdlog::info("AlpacaHub Exiting");
  return 0;
}
