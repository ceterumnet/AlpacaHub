#include "qhy_alpaca_filterwheel.hpp"
#include "alpaca_exception.hpp"
#include "qhy_alpaca_camera.hpp"
#include <qhyccderr.h>

bool qhy_alpaca_filterwheel::connected() { return _connected; }

int qhy_alpaca_filterwheel::set_connected(bool connected) {
  _connected = connected;
  return 0;
}

std::string qhy_alpaca_filterwheel::description() { return _description; }

std::string qhy_alpaca_filterwheel::driverinfo() { return _driver_info; }

std::string qhy_alpaca_filterwheel::name() { return _name; }

uint32_t qhy_alpaca_filterwheel::interface_version() { return 2; };

std::string qhy_alpaca_filterwheel::driver_version() { return _driver_version; }

std::vector<std::string> qhy_alpaca_filterwheel::supported_actions() {
  return std::vector<std::string>();
}

void qhy_alpaca_filterwheel::initialize() {}

qhy_alpaca_filterwheel::qhy_alpaca_filterwheel(qhy_alpaca_camera *camera)
    : _camera(camera), _connected(false), _driver_version("v0.1"),
      _description("QHY Filterwheel"), _name("QHYFW") {


  // TODO: This should be driven off of what the filterwheel indicates is
  // actually there
  _names = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
  _focus_offsets = {0, 0, 0, 0, 0, 0, 0, 0, 0};
}

qhy_alpaca_filterwheel::~qhy_alpaca_filterwheel() {
  spdlog::debug("filterwheel destructor called");
}

int qhy_alpaca_filterwheel::position() {
  char fw_status = 0;
  uint32_t fw_res = QHYCCD_ERROR;
  fw_res = GetQHYCCDCFWStatus(_camera->_cam_handle, &fw_status);

  if (fw_res == QHYCCD_SUCCESS)
    return fw_status - '0';
  else
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           "Problem setting filter position");
}

std::vector<std::string> qhy_alpaca_filterwheel::names() { return _names; }

// TODO: need to add validation and logic for this to make sense
int qhy_alpaca_filterwheel::set_names(std::vector<std::string> names) {
  _names = names;
  return 0;
}

int qhy_alpaca_filterwheel::set_position(uint32_t position) {
  char pos = position + '0';
  uint32_t r = QHYCCD_ERROR;
  r = SendOrder2QHYCCDCFW(_camera->_cam_handle, &pos, 1);
  return r;
}

std::vector<int> qhy_alpaca_filterwheel::focus_offsets() {
  return _focus_offsets;
}

std::string qhy_alpaca_filterwheel::unique_id() {
  return _camera->unique_id() + std::string("FW");
}
