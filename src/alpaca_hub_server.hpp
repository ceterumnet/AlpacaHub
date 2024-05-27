#ifndef ALPACA_HUB_SERVER_HPP
#define ALPACA_HUB_SERVER_HPP

#include "alpaca_hub_common.hpp"
#include "drivers/qhy_alpaca_camera.hpp"
#include "drivers/qhy_alpaca_filterwheel.hpp"
#include "interfaces/i_alpaca_camera.hpp"
#include "interfaces/i_alpaca_device.hpp"
#include "interfaces/i_alpaca_telescope.hpp"
#include "restinio/cast_to.hpp"
#include "restinio/common_types.hpp"
#include "restinio/core.hpp"
#include "restinio/helpers/easy_parser.hpp"
#include "restinio/http_headers.hpp"
#include "restinio/request_handler.hpp"
#include "restinio/router/easy_parser_router.hpp"
#include "restinio/router/express.hpp"
#include "restinio/sync_chain/fixed_size.hpp"
#include <bit>
#include <cstdint>
#include <filesystem>
#include <ios>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#endif
