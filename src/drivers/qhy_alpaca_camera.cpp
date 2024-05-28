#include "qhy_alpaca_camera.hpp"

// Begin camera collection related functions.
// these are static functions to retrieve
// all cameras tied to the QHYSDK
u_int32_t qhy_alpaca_camera::_num_of_connected_cameras = 0;

// This stores the name of the camera and the corresponding
// camera index
std::map<std::string, int> qhy_alpaca_camera::_camera_map =
    std::map<std::string, int>();

void qhy_alpaca_camera::on_camera_unplugged(char *id) {
  std::string unplugged_msg(id);
  spdlog::trace("Camera unplugged");
  spdlog::trace("  id: {0}", unplugged_msg);
  qhy_alpaca_camera::_num_of_connected_cameras = ScanQHYCCD();
}

void qhy_alpaca_camera::on_camera_plugged(char *id) {
  std::string unplugged_msg(id);
  spdlog::trace("Camera plugged in");
  spdlog::trace("  id: {0}", unplugged_msg);
  qhy_alpaca_camera::_num_of_connected_cameras = ScanQHYCCD();
}

int qhy_alpaca_camera::InitializeQHYSDK() {
  uint32_t result = InitQHYCCDResource();
  // This line will enable fairly verbose logging from the QHYSDK
  // EnableQHYCCDMessage(true);
  spdlog::trace("InitQHYCCDResource() returned: {0}", result);
  spdlog::trace("Registering plug and unplug callbacks");

  RegisterPnpEventIn(&qhy_alpaca_camera::on_camera_plugged);
  RegisterPnpEventOut(&qhy_alpaca_camera::on_camera_unplugged);

  if (result == QHYCCD_SUCCESS) {
    spdlog::trace("InitQHYCCDResource() returned: {0}", result);
    qhy_alpaca_camera::_num_of_connected_cameras = ScanQHYCCD();
    spdlog::trace("num of cameras: {0}",
                  qhy_alpaca_camera::_num_of_connected_cameras);
  }

  return result;
};

int qhy_alpaca_camera::ReleaseQHYSDK() {
  spdlog::debug("Releasing QHYSDK");
  uint32_t r = ReleaseQHYCCDResource();
  if (r == QHYCCD_SUCCESS) {
    spdlog::debug("Successfully released QHYSDK");
  } else {
    spdlog::error("Problem releasing QHYSDK {0}", r);
  }
  return r;
};

uint32_t qhy_alpaca_camera::camera_count() {
  return qhy_alpaca_camera::_num_of_connected_cameras;
};

std::vector<std::string> qhy_alpaca_camera::get_connected_cameras() {
  std::vector<std::string> camera_list;
  for (int i = 0; i < qhy_alpaca_camera::_num_of_connected_cameras; i++) {
    std::string camera_identifier;

    // I believe 64 is the max size here.
    camera_identifier.resize(64);
    GetQHYCCDId(i, &camera_identifier[0]);
    camera_identifier.resize(std::strlen(camera_identifier.c_str()));
    camera_list.push_back(camera_identifier);
    _camera_map[camera_identifier] = i;
  };
  return camera_list;
};
// End camera collection related functions

// Helper function:
int qhy_alpaca_camera::get_is_qhy_control_available(CONTROL_ID control_id) {
  uint32_t qhy_res = QHYCCD_ERROR;
  qhy_res = IsQHYCCDControlAvailable(_cam_handle, control_id);
  return qhy_res;
}

void qhy_alpaca_camera::initialize_camera_by_camera_id(std::string &camera_id) {
  uint32_t qhy_res = QHYCCD_ERROR;

  _cam_handle = OpenQHYCCD(&camera_id[0]);

  if (_cam_handle == 0) {
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("OpenQHYCCD failed for camera_id {0}", camera_id));
  }

  _qhy_model_name.resize(1024);
  // Set Model name
  qhy_res = GetQHYCCDModel(&camera_id[0], &_qhy_model_name[0]);
  if (qhy_res == QHYCCD_SUCCESS) {
    _qhy_model_name.resize(std::strlen(_qhy_model_name.c_str()));
    spdlog::debug("CCDModel Name: {0}", _qhy_model_name);
  } else {
    spdlog::error("GetQHYCCDModel returned error code: {0}", qhy_res);
    _qhy_model_name = "Generic QHY Camera";
  }

  // This happens once before calling the init
  qhy_res = GetQHYCCDNumberOfReadModes(_cam_handle, &_num_modes);
  if (qhy_res != QHYCCD_SUCCESS) {
    spdlog::error("GetQHYCCDNumberOfReadModes failed with return code: {0}",
                  qhy_res);
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("GetQHYCCDNumberOfReadModes failed for camera_id: {0}",
                    camera_id[0]));
  }

  qhy_res = QHYCCD_ERROR;

  qhy_res = SetQHYCCDReadMode(_cam_handle, 0);
  if (qhy_res != QHYCCD_SUCCESS)
    spdlog::error("SetQHYCCDReadMode failed with return code: {0}", qhy_res);

  // Let's push the mode names to our private instance variable
  for (int x = 0; x < _num_modes; x++) {
    std::string mode_name;
    // TODO: This is probably too large of a buffer...
    mode_name.resize(1024);
    // TODO: I really should handle any errors that might occur
    GetQHYCCDReadModeName(_cam_handle, x, &mode_name[0]);
    spdlog::debug("GetQHYCCDReadModeName: {0}", mode_name);
    mode_name.resize(std::strlen(mode_name.c_str()));
    _read_mode_names.push_back(mode_name);
  }

  // Chip info
  qhy_res = GetQHYCCDChipInfo(_cam_handle, &_chip_w, &_chip_h, &_image_w,
                              &_image_h, &_pixel_w, &_pixel_h, &_bpp);
  if (qhy_res == QHYCCD_SUCCESS) {
    spdlog::debug("Chip info:");
    spdlog::debug("  chip width:   {0}", _chip_w);
    spdlog::debug("  chip height:  {0}", _chip_h);
    spdlog::debug("  image width:  {0}", _image_w);
    spdlog::debug("  image height: {0}", _image_h);
    spdlog::debug("  pixel width:  {0}", _pixel_w);
    spdlog::debug("  pixel height: {0}", _pixel_h);
    spdlog::debug("  bpp:          {0}", _bpp);
  } else {
    spdlog::error(
        "Failed to successfully call GetQHYCCDChipInfo, SDK returned: {0}",
        qhy_res);
  }

  qhy_res = QHYCCD_ERROR;
  qhy_res = IsQHYCCDCFWPlugged(_cam_handle);

  if (qhy_res == QHYCCD_SUCCESS) {
    spdlog::debug("Filterwheel appears to be connected. Initializing");
    _filter_wheel = std::make_shared<qhy_alpaca_filterwheel>(this);

    int num_slots =
        (int)GetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_CFWSLOTSNUM);

    spdlog::debug("Number of filterwheel slots: {0}", num_slots);
    spdlog::debug("Current filter wheel position: {0}",
                  _filter_wheel->position() - '0');

    double cfwport_max = 0, cfwport_min = 0, cfwport_step = 0;
    uint32_t fw_res = QHYCCD_ERROR;
    fw_res =
        GetQHYCCDParamMinMaxStep(_cam_handle, CONTROL_ID::CONTROL_CFWPORT,
                                 &cfwport_min, &cfwport_max, &cfwport_step);
    if (fw_res == QHYCCD_SUCCESS)
      spdlog::debug("GetQHYCCDParamMinMaxStep for CONTROL_CFWPORT - max:{0}, "
                    "min:{1}, step:{2}",
                    cfwport_max, cfwport_min, cfwport_step);
    else
      spdlog::debug("unable to get min max and step for CONTROL_CFWPORT");

    char fw_status = 0;
    fw_res = QHYCCD_ERROR;

    fw_res = GetQHYCCDCFWStatus(_cam_handle, &fw_status);
    if (fw_res == QHYCCD_SUCCESS) {
      _has_filter_wheel = true;
      spdlog::debug("Successfully obtained fw_status: {0}", fw_status);
    } else
      spdlog::error("Failed to get fw_status: {0}", fw_res);

  } else {
    spdlog::debug("No filterwheel detected");
  }

  uint32_t qhy_ccd_type = 0;
  qhy_ccd_type = GetQHYCCDType(_cam_handle);
  spdlog::trace("CCDType: {0}", GetQHYCCDType(_cam_handle));

  // Initialize subframe width and height to the values returned for the chip
  _num_x = _image_w;
  _num_y = _image_h;

  // Let's initialize max width and height to the values returned for the chip
  _max_num_x = _image_w;
  _max_num_y = _image_h;

  _effective_num_x = _image_w;
  _effective_num_y = _image_h;

  if (!_include_overscan) {
    spdlog::debug(
        "We are not including the overscan, calculating effective dimensions.");
    qhy_res = QHYCCD_ERROR;
    qhy_res = GetQHYCCDEffectiveArea(_cam_handle, &_effective_start_x,
                                     &_effective_start_y, &_effective_num_x,
                                     &_effective_num_y);

    if (_effective_num_x != 0) {
      // Set the max width and height based on the effective values
      _max_num_x = _effective_num_x;
      _max_num_y = _effective_num_y;

      // Initialize the image width and height to the effective values
      _image_w = _max_num_x;
      _image_h = _max_num_y;

      // Initialize the start x and y based on the effective values
      _start_x = _effective_start_x;
      _start_y = _effective_start_y;

      _num_x = _effective_num_x;
      _num_y = _effective_num_y;

      spdlog::debug("Returned from GetQHYCCDEffectiveArea: ");
      spdlog::debug("  _effective_start_x: {0}", _effective_start_x);
      spdlog::debug("  _effective_start_y: {0}", _effective_start_y);
      spdlog::debug("  _effective_num_x:   {0}", _effective_num_x);
      spdlog::debug("  _effective_num_y:   {0}", _effective_num_y);
    } else {
      _effective_num_x = _image_w;
      _effective_num_y = _image_h;
      _effective_start_x = 0;
      _effective_start_y = 0;
    }
  }
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(200ms);
  // This gets the effective area if there is overscan

  // Initialize Camera
  // Probably need to return here or do some kind of invariant for
  // a failed initialization of camera?
  uint32_t initialization_result = QHYCCD_ERROR;
  initialization_result = InitQHYCCD(_cam_handle);
  if (initialization_result == QHYCCD_SUCCESS) {
    spdlog::debug("Successfully initialized camera");
  } else {
    spdlog::error("Error initializing camera, err code: {0}",
                  initialization_result);
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("Error initializing camera, err code: {0}",
                    initialization_result));
  }

  // Get and Set Model name from SDK
  _qhy_model_name.resize(1024);
  qhy_res = GetQHYCCDModel(&camera_id[0], &_qhy_model_name[0]);
  if (qhy_res == QHYCCD_SUCCESS) {
    _qhy_model_name.resize(std::strlen(_qhy_model_name.c_str()));
    spdlog::debug("CCDModel Name: {0}", _qhy_model_name);
  } else {
    spdlog::warn(
        "Unable to get model name. GetQHYCCDModel returned error code: {0}",
        qhy_res);
    _qhy_model_name = "Generic QHY Camera";
  }

  // Get and Set Sensor Name from SDK
  _sensor_name.resize(1024);
  uint32_t sensor_name_r = QHYCCD_ERROR;
  sensor_name_r = GetQHYCCDSensorName(_cam_handle, &_sensor_name[0]);
  if (sensor_name_r == QHYCCD_SUCCESS) {
    spdlog::debug("Successfully got sensor Name: {0}", _sensor_name);
  } else {
    spdlog::warn("Failed to get sensor name");
    _sensor_name = "Generic CCD";
  }

  if (get_is_qhy_control_available(CONTROL_ID::CONTROL_COOLER) ==
      QHYCCD_SUCCESS) {
    _can_control_cooler_power = true;

    // The QHYSDK _may_ have an issue that's outside of my control here.
    // The first time that CONTROL_COOLER is set, there is an uninitialized
    // buffer error reported from valgrind
    //
    // spdlog::trace("I think this is the call that is causing valgrind to
    // complain.");
    uint32_t c_set_r = QHYCCD_ERROR;
    c_set_r = SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_COOLER,
                             _current_set_temp);
    // spdlog::trace("This is right after that call: {0}", c_set_r);

    double c_temp = GetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_CURTEMP);
    double c_pwm = GetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_CURPWM);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);
    spdlog::debug("After initializing cooler to {0}c Temp, the camera reports "
                  "temp {1}c and pwm of {2}%",
                  _current_set_temp, c_temp, c_pwm);
  }

  spdlog::trace("Current chip temp: {0}",
                GetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_CURTEMP));

  spdlog::trace("Can control chip temp: {0}", can_set_ccd_temperature());
  spdlog::trace("Can get cooler power: {0}", can_get_cooler_power());
  spdlog::trace("Full well capacity: {0}", full_well_capacity());

  // TODO: I really should handle any error that might occur
  GetQHYCCDParamMinMaxStep(_cam_handle, CONTROL_ID::CONTROL_GAIN, &_gain_min,
                           &_gain_max, &_gain_step_size);
  spdlog::trace("Max Gain: {0}", _gain_max);
  spdlog::trace("Min Gain: {0}", _gain_min);
  spdlog::trace("Gain Step Size: {0}", _gain_step_size);

  for (int i = 0; i < _gain_max; i++) {
    // I don't love the c style cast here, but it should be pretty safe
    _gains.push_back(std::to_string((int)_gain_min + i));
  }

  _gain = GetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_GAIN);
  spdlog::trace("Gain: {0}", _gain);
  set_gain(_gain);

  // TODO: I really should handle any error that might occur
  GetQHYCCDParamMinMaxStep(_cam_handle, CONTROL_ID::CONTROL_EXPOSURE,
                           &_exposure_min, &_exposure_max,
                           &_exposure_step_size);
  spdlog::trace("Max Exposure: {0}", _exposure_max);
  spdlog::trace("Min Exposure: {0}", _exposure_min);
  spdlog::trace("Exposure Step Size: {0}", _exposure_step_size);

  // TODO: I really should handle any error that might occur
  GetQHYCCDParamMinMaxStep(_cam_handle, CONTROL_ID::CONTROL_OFFSET,
                           &_offset_min, &_offset_max, &_offset_step_size);

  spdlog::trace("Max Offset: {0}", _offset_max);
  spdlog::trace("Min Offset: {0}", _offset_min);
  spdlog::trace("Offset Step Size: {0}", _offset_step_size);
  for (int i = 0; i < _offset_max; i++) {
    // I don't love the c style cast here, but it should be pretty safe
    _offsets.push_back(std::to_string((int)_offset_min + i));
  }
  _offset = _offset_min;
  spdlog::trace("Trying to set offset: {0}",
                SetQHYCCDParam(_cam_handle, CONTROL_OFFSET, _offset_min));

  // Camera Status
  uint8_t cam_status_buf[4] = {};
  // TODO: I really should handle any error that might occur
  GetQHYCCDCameraStatus(_cam_handle, cam_status_buf);
  spdlog::trace("Camera Status: {0:#x}, {1:#x}, {2:#x}, {3:#x}",
                cam_status_buf[0], cam_status_buf[1], cam_status_buf[3],
                cam_status_buf[3]);

  uint32_t read_width = 0;
  uint32_t read_height = 0;

  uint32_t res_result = QHYCCD_ERROR;
  res_result =
      GetQHYCCDReadModeResolution(_cam_handle, 0, &read_width, &read_height);
  if (res_result == QHYCCD_SUCCESS) {
    spdlog::trace("Successfully fetched read mode resolution: ");
    spdlog::trace("  values: read_width: {0}, read_height: {1}", read_width,
                  read_height);
  } else {
    spdlog::error("failed to get read mode resolution");
  }
  // This is spurious I think?
  set_bin_x(_bin_x);

  // Set the resolution here, but I believe we must set it before each exposure
  set_resolution(_effective_start_x, _effective_start_y, _effective_num_x,
                 _effective_num_y);

  // SetQHYCCDParam(_cam_handle, CONTROL_USBTRAFFIC, 30);
  uint32_t stream_mode_set_res = SetQHYCCDStreamMode(_cam_handle, 0);
  if (stream_mode_set_res != QHYCCD_SUCCESS)
    spdlog::error("failed to set QHYCCD stream mode: {0}", stream_mode_set_res);
  // SetQHYCCDParam(_cam_handle, CONTROL_GAIN, 14);

  if (IsQHYCCDControlAvailable(_cam_handle, CONTROL_ID::CONTROL_USBTRAFFIC)) {
    spdlog::trace("CONTROL_USBTRAFFIC is available");
    double u_min = 0;
    double u_max = 0;
    double u_step = 0;
    GetQHYCCDParamMinMaxStep(_cam_handle, CONTROL_USBTRAFFIC, &u_min, &u_max,
                             &u_step);
    spdlog::trace("USB Traffic Settings - min:{0} max:{1} step:{2}", u_min,
                  u_max, u_step);

  } else {
    spdlog::trace("CONTROL_USBTRAFFIC is not available");
  }

  if (IsQHYCCDControlAvailable(_cam_handle, CONTROL_ID::CONTROL_SPEED)) {
    spdlog::trace("CONTROL_SPEED Available");
    spdlog::trace("Trying to set usb control speed: {0}",
                  SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_SPEED, 2));
  }

  spdlog::trace(
      "Can enter single frame mode?: {0}",
      IsQHYCCDControlAvailable(_cam_handle, CONTROL_ID::CAM_SINGLEFRAMEMODE));
  spdlog::trace("Determining max binning");
  _max_bin = 1;

  if (IsQHYCCDControlAvailable(_cam_handle, CAM_BIN8X8MODE) == QHYCCD_SUCCESS)
    _max_bin = 8;
  if (IsQHYCCDControlAvailable(_cam_handle, CAM_BIN6X6MODE) == QHYCCD_SUCCESS)
    _max_bin = 6;
  if (IsQHYCCDControlAvailable(_cam_handle, CAM_BIN4X4MODE) == QHYCCD_SUCCESS)
    _max_bin = 4;
  if (IsQHYCCDControlAvailable(_cam_handle, CAM_BIN2X2MODE) == QHYCCD_SUCCESS)
    _max_bin = 2;

  _has_shutter = false;
  _has_shutter =
      IsQHYCCDControlAvailable(_cam_handle, CONTROL_ID::CAM_MECHANICALSHUTTER);

  if (_has_shutter)
    spdlog::debug("CAM_MECHANICALSHUTTER is available");
  else
    spdlog::debug("CAM_MECHANICALSHUTTER control is not available");
}

// TODO: I need to move a lot of the code in the CTOR to an initialization
// method that will be invoked when connected is set to true
qhy_alpaca_camera::qhy_alpaca_camera(std::string &camera_id)
    : _camera_state(camera_state_enum::CAMERA_IDLE), _cam_handle(0),
      _camera_id(camera_id), _connected(false), _bin_x(1), _bin_y(1),
      _start_x(0), _start_y(0), _chip_w(0), _chip_h(0), _image_w(0),
      _image_h(0), _pixel_w(0), _pixel_h(0), _num_x(0), _num_y(0), _gain(0),
      _current_set_temp(40), _exposure_max(0), _exposure_min(0),
      _subexposure_duration(0), _max_bin(1), _fast_readout(false),
      _readout_mode(0), _last_exposure_duration(0),
      _can_control_cooler_power(false), _effective_num_x(0),
      _effective_num_y(0), _effective_start_x(0), _effective_start_y(0),
      _include_overscan(false), _max_num_x(0), _max_num_y(0),
      _percent_complete(100), _set_cooler_power(0),
      _can_control_ccd_temp(false), _run_cooler_thread(false),
      _has_filter_wheel(false) {
  initialize_camera_by_camera_id(camera_id);
};

qhy_alpaca_camera::~qhy_alpaca_camera() {
  spdlog::trace("Closing Camera");
  if (_cooler_thread.joinable())
    _cooler_thread.join();
  uint32_t r = CloseQHYCCD(_cam_handle);
  if (r == QHYCCD_SUCCESS) {
    spdlog::trace("Successfully Closed Camera");
  } else {
    spdlog::error("Error Closing Camera: {0}", r);
  }
}

uint32_t qhy_alpaca_camera::interface_version() { return 3; }

std::string qhy_alpaca_camera::driver_version() { return "v0.1"; }

bool qhy_alpaca_camera::connected() { return this->_connected; };

int qhy_alpaca_camera::set_connected(bool connected) {
  std::lock_guard lock(_cam_mutex);
  _connected = connected;
  return 0;
}

short qhy_alpaca_camera::bin_x() { return _bin_x; };

short qhy_alpaca_camera::bin_y() { return _bin_y; };

int qhy_alpaca_camera::set_bin_x(short x) {
  spdlog::debug("setting bin: {0}", x);

  // no op if binning is already set
  if (x == _bin_x)
    return 0;

  std::lock_guard lock(_cam_mutex);

  if (x > _max_bin || x < 1)
    return -1;
  if (SetQHYCCDBinMode(_cam_handle, x, x) == QHYCCD_SUCCESS) {
    _bin_x = x;
    _bin_y = x;

    _start_x = _effective_start_x / x;
    _start_y = _effective_start_y / x;
    _num_x = _effective_num_x / x;
    _num_y = _effective_num_y / x;

    set_resolution(_start_x, _start_y, _num_x, _num_y);

    return 0;
  } else {
    return -1;
  }
}

int qhy_alpaca_camera::set_bin_y(short y) { return set_bin_x(y); }

qhy_alpaca_camera::camera_state_enum qhy_alpaca_camera::camera_state() {
  std::lock_guard lock(_cam_mutex);
  return _camera_state;
};

// width in unbinned pixels
// excludes overscan pixels unless inclusion is enabled
long qhy_alpaca_camera::camera_x_size() { return _max_num_x; };
long qhy_alpaca_camera::camera_y_size() { return _max_num_y; };

// All QHY Cameras support this according to the SDK
bool qhy_alpaca_camera::can_abort_exposure() { return true; };

// None of the QHY cameras support asymmetric binning
bool qhy_alpaca_camera::can_asymmetric_bin() { return false; };

bool qhy_alpaca_camera::can_get_cooler_power() {
  return _can_control_cooler_power;
};

// I don't think cameras ever implement this, or at least I can't
// think of a scenario.
bool qhy_alpaca_camera::can_pulse_guide() { return false; };

// TODO: figure out CCD temp control mechanics
// I'm not sure if this is mutually exclusive with can set cooler power
// I'm thinking I might be able to get away without a control thread for temp?
bool qhy_alpaca_camera::can_set_ccd_temperature() {
  // return false;
  return _can_control_cooler_power;
};

// My understanding from the QHY SDK is that all of there cameras support
// this
bool qhy_alpaca_camera::can_stop_exposure() { return true; };

double qhy_alpaca_camera::ccd_temperature() {
  if (!_can_control_cooler_power)
    throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                           "CCD temp not supported on this camera");
  spdlog::trace("ccd_temperature() invoked");
  std::lock_guard lock(_cam_mutex);
  double current_temp = 0;
  current_temp = GetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_CURTEMP);
  spdlog::trace("ccd_temperature: {0}", current_temp);
  return current_temp;
};

// I'm just basically checking if the cur pwm or manual pwm is set
// TODO: I might use a different mutex for cooler functions...but I'm
// not sure it's necessary
bool qhy_alpaca_camera::cooler_on() {
  spdlog::trace("cooler_on() invoked");
  std::lock_guard lock(_cam_mutex);
  if (!_can_control_cooler_power) {
    throw alpaca_exception(
        alpaca_exception::NOT_IMPLEMENTED,
        "CCD Cooler Control is Not supported on this camera");
  }

  return _run_cooler_thread;
}

bool qhy_alpaca_camera::should_run_cooler_proc() {
  std::lock_guard lock(_cam_mutex);
  return _run_cooler_thread;
}

void qhy_alpaca_camera::ensure_temp_is_set() {
  std::lock_guard lock(_cam_mutex);
  spdlog::trace("ensuring temp is set");
  SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_COOLER, _current_set_temp);
  spdlog::trace("current temp is {0}",
                GetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_CURTEMP));
}

void qhy_alpaca_camera::cooler_proc() {
  spdlog::debug("cooler thread running");

  using namespace std::chrono_literals;
  while (should_run_cooler_proc()) {
    // if(_not_reading_image)
    ensure_temp_is_set();
    std::this_thread::sleep_for(1000ms);
  }
  spdlog::debug("cooler thread exited");
};

int qhy_alpaca_camera::set_cooler_on(bool cooler_on) {
  if (!_can_control_cooler_power)
    throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                           "Cooler Power Setting not available on this camera");
  spdlog::debug("setting cooler on to {0}", cooler_on);

  std::lock_guard lock(_cam_mutex);

  if (_run_cooler_thread == true && cooler_on) {
    spdlog::warn("Cooler is already on. This is a no-op");
    return 0;
  }

  if (cooler_on) {
    _run_cooler_thread = true;
    // cooler_power = _set_cooler_power / 100.0 * 255.0;
    spdlog::debug("starting turning cooler on with set temp of {0}",
                  _current_set_temp);
    _cooler_thread =
        std::thread(std::bind(&qhy_alpaca_camera::cooler_proc, this));
    _cooler_thread.detach();
    return 0;
  } else {
    _run_cooler_thread = false;
    spdlog::debug("cooler control thread has been requested to shutdown");
    if (SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_MANULPWM, 0) ==
        QHYCCD_SUCCESS)
      return 0;
  }
  return -1;
};

// TODO: I need to make sure this is a sane value and it doesn't need
// some kind of manipulation or better error checking
double qhy_alpaca_camera::cooler_power() {
  if (!_can_control_cooler_power)
    throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                           "Cooler Power Setting not available on this camera");
  std::lock_guard lock(_cam_mutex);
  auto curpwm = GetQHYCCDParam(_cam_handle, CONTROL_CURPWM) / 255.0 * 100.0;
  spdlog::trace("cooler_power() invoked and returning {0}%", curpwm);

  return curpwm;
};

// TODO: This is one of those weird ones...not sure
// this is correct or how much it matters?
double qhy_alpaca_camera::electrons_per_adu() {
  // std::lock_guard lock(_cam_mutex);
  // return GetQHYCCDParam(_cam_handle, CONTROL_GAIN);
  return 0.1;
};

// TODO: This is an implementation that I'm not sure is right or not yet.
// I need to test this.
double qhy_alpaca_camera::full_well_capacity() {
  std::lock_guard lock(_cam_mutex);
  double fullwell = pow(2, _bpp) - 1;
  // QHYCCD_curveFullWell(_cam_handle, _gain, &fullwell);
  return fullwell;
}

// TODO: may want to move this to CTOR
bool qhy_alpaca_camera::has_shutter() { return _has_shutter; }

double qhy_alpaca_camera::heat_sink_temperature() {
  if (!_can_control_cooler_power)
    throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                           "Cooler Power Setting not available on this camera");
  std::lock_guard lock(_cam_mutex);
  return GetQHYCCDParam(_cam_handle, CONTROL_CURTEMP);
}

// TODO: I'm not sure this needs to be a public method on the
// camera interface.
int qhy_alpaca_camera::image_array(std::vector<uint8_t> &theImage) {
  // theImage.resize(_img_data.size());
  std::lock_guard lock(_cam_mutex);
  theImage = _img_data;
  return 0;
}

// TODO: consider refactoring the image_2d code into a separate helper
// class that can implement templates accordingly
std::vector<std::vector<uint32_t>> qhy_alpaca_camera::image_2d_8bpp() {
  std::vector<uint8_t> data_ptr =
      reinterpret_cast<std::vector<uint8_t> &>(_img_data);

  std::vector<std::vector<uint32_t>> image_2d_data(
      _image_w, std::vector<uint32_t>(_image_h, 0));

  for (int row_idx = 0; row_idx < _image_w; row_idx++) {
    for (int col_idx = 0; col_idx < _image_h; col_idx++) {
      image_2d_data[row_idx][col_idx] =
          data_ptr[row_idx + (col_idx * _image_w)];
    }
  }

  spdlog::trace("image_2d_data is {0} x {1}", image_2d_data.size(),
                image_2d_data[0].size());
  return image_2d_data;
}

std::vector<std::vector<uint32_t>> qhy_alpaca_camera::image_2d_16bpp() {
  if (_img_data.size() == 0) {
    return std::vector<std::vector<uint32_t>>(0);
  }

  std::vector<uint16_t> data_ptr =
      reinterpret_cast<std::vector<uint16_t> &>(_img_data);

  std::vector<std::vector<uint32_t>> image_2d_data(
      _image_w, std::vector<uint32_t>(_image_h, 0));

  for (int row_idx = 0; row_idx < _image_w; row_idx++) {
    for (int col_idx = 0; col_idx < _image_h; col_idx++) {
      image_2d_data[row_idx][col_idx] =
          data_ptr[row_idx + (col_idx * _image_w)];
    }
  }
  spdlog::trace("image_2d_data is {0} x {1}", image_2d_data.size(),
                image_2d_data[0].size());
  return image_2d_data;
}

std::vector<std::vector<uint32_t>> qhy_alpaca_camera::image_2d() {
  if (_bpp == 8) {
    return image_2d_8bpp();
  }

  return image_2d_16bpp();
}

void qhy_alpaca_camera::set_reading_state() {
  std::lock_guard lock(_cam_mutex);
  _camera_state = camera_state_enum::CAMERA_READING;
}

void qhy_alpaca_camera::read_image_from_camera() {
  set_reading_state();

  spdlog::trace("read_image_from_camera started");
  uint32_t w = 0;
  uint32_t h = 0;
  uint32_t bpp = 0;
  uint32_t channels = 0;
  uint32_t img_size = 0;
  img_size = GetQHYCCDMemLength(_cam_handle);
  spdlog::trace("Image size: {0}", img_size);
  _img_data.resize(img_size);

  spdlog::trace("Calling GetQHYCCDSingleFrame and fetching img_data", img_size);

  // Adding this per the SDK spec so that nothing else should happen while
  // reading from the camera
  std::lock_guard lock(_cam_mutex);
  uint32_t r =
      GetQHYCCDSingleFrame(_cam_handle, &w, &h, &bpp, &channels, &_img_data[0]);

  _image_w = w;
  _image_h = h;
  spdlog::debug("w: {0} h:{1} bpp:{2} channels: {3}", w, h, bpp, channels);

  if (r == QHYCCD_SUCCESS) {
    spdlog::trace("Successfully executed GetQHYCCDSingleFrame with {0} size",
                  img_size);
    spdlog::trace("setting camera state to idle");
    _camera_state = camera_state_enum::CAMERA_IDLE;
  } else {
    // TODO: push last error here
    _camera_state = camera_state_enum::CAMERA_ERROR;
  }
}

// TODO: This isn't quite right yet
bool qhy_alpaca_camera::image_ready() {
  std::lock_guard lock(_cam_mutex);

  if (_camera_state == camera_state_enum::CAMERA_IDLE &&
      _last_exposure_duration > 0) {
    return true;
  }

  return false;
}

bool qhy_alpaca_camera::is_pulse_guiding() {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Pulse guiding is not supported");
  return false;
}

std::string qhy_alpaca_camera::last_error() { return _last_cam_error; }

double qhy_alpaca_camera::last_exposure_duration() {
  if (_last_exposure_duration == 0)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "No image has been taken");
  return _last_exposure_duration;
}

std::string qhy_alpaca_camera::last_exposure_start_time() {
  if (_last_exposure_duration == 0)
    throw alpaca_exception(alpaca_exception::INVALID_OPERATION,
                           "No image has been taken");
  return _last_exposure_start_time_fits;
}

long qhy_alpaca_camera::max_adu() { return pow(2, _bpp) - 1; }

short qhy_alpaca_camera::max_bin_x() { return _max_bin; }

short qhy_alpaca_camera::max_bin_y() { return max_bin_x(); }

long qhy_alpaca_camera::num_x() {
  std::lock_guard lock(_cam_mutex);
  uint32_t startX = 0;
  uint32_t startY = 0;
  uint32_t sizeX = 0;
  uint32_t sizeY = 0;
  uint32_t r =
      GetQHYCCDCurrentROI(_cam_handle, &startX, &startY, &sizeX, &sizeY);
  return sizeX;
}

long qhy_alpaca_camera::num_y() {
  std::lock_guard lock(_cam_mutex);
  uint32_t startX = 0;
  uint32_t startY = 0;
  uint32_t sizeX = 0;
  uint32_t sizeY = 0;

  uint32_t r =
      GetQHYCCDCurrentROI(_cam_handle, &startX, &startY, &sizeX, &sizeY);
  return sizeY;
}

// TODO: refactor this code as it is a common call to the SetQHYCCDResolution
int qhy_alpaca_camera::set_num_x(long num_x) {
  if (num_x > _max_num_x)
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        fmt::format(
            "Attempt to set image width to {0}, which exceeds max of {1}",
            num_x, _max_num_x));
  uint32_t r = QHYCCD_ERROR;
  r = set_resolution(_start_x, _start_y, num_x, _num_y);
  //    SetQHYCCDResolution(_cam_handle, _start_x, _start_y, num_x, _num_y);
  if (r != QHYCCD_SUCCESS)
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("Failed to set num_x with error {0}", r));
  _num_x = num_x;
  return 0;
}

int qhy_alpaca_camera::set_num_y(long num_y) {
  if (num_y > _max_num_y)
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        fmt::format(
            "Attempt to set image height to {0}, which exceeds max of {1}",
            num_y, _max_num_y));
  uint32_t r =
      SetQHYCCDResolution(_cam_handle, _start_x, _start_y, _num_x, num_y);
  if (r != QHYCCD_SUCCESS)
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("Failed to set num_x with error {0}", r));
  _num_y = num_y;
  return 0;
}

double qhy_alpaca_camera::pixel_size_x() { return _pixel_w; }

double qhy_alpaca_camera::pixel_size_y() { return _pixel_h; }

int qhy_alpaca_camera::set_ccd_temperature(double temp) {
  // throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED, "This is an open
  // loop cooler. PWM can be controlled, but temp can not be set directly. ");
  std::lock_guard lock(_cam_mutex);
  spdlog::debug("set_ccd_temperature invoked with {0}", temp);
  _current_set_temp = temp;
  return 0;
}

double qhy_alpaca_camera::get_set_ccd_temperature() {
  // throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
  //                        "This is an open loop cooler. PWM can be controlled,
  //                        " "but temp can not be set directly. ");
  std::lock_guard lock(_cam_mutex);
  return _current_set_temp;
  // throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
  //                        "automatic temperature control not implemented");
}

// Subframe start x coordinate
long qhy_alpaca_camera::start_x() { return _start_x; }

int qhy_alpaca_camera::set_start_x(long start_x) {
  spdlog::debug("set_start_x: {0}", start_x);
  if (start_x > _effective_num_x)
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        fmt::format("Attempted to set sub-frame start position x: {0}, which "
                    "exceeds the max dimension of {1}",
                    start_x, _effective_num_x));
  _start_x = (start_x + _effective_start_x) / _bin_x;
  return 0;
}

// Subframe start y coordinate
long qhy_alpaca_camera::start_y() { return _start_y; };

int qhy_alpaca_camera::set_start_y(long start_y) {
  spdlog::debug("set_start_y: {0}", start_y);
  if (start_y > _effective_num_y)
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        fmt::format("Attempted to set sub-frame start position y: {0}, which "
                    "exceeds the max dimension of {1}",
                    start_y, _effective_num_y));

  _start_y = (start_y + _effective_start_y) / _bin_y;
  return 0;
}

int qhy_alpaca_camera::abort_exposure() {
  std::lock_guard lock(_cam_mutex);
  uint32_t r = QHYCCD_ERROR;
  r = CancelQHYCCDExposingAndReadout(_cam_handle);
  if (r == QHYCCD_SUCCESS) {
    _camera_state = camera_state_enum::CAMERA_IDLE;
    spdlog::debug(
        "Successfully stopped exposure. Image data can not be read now.");
    return 0;
  }
  _camera_state = camera_state_enum::CAMERA_ERROR;

  spdlog::error("Problem stopping exposure: {0}", r);
  return -1;
}

int qhy_alpaca_camera::pulse_guide(qhy_alpaca_camera::guide_direction,
                                   long duration) {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Pulse guiding not supported");
};

int qhy_alpaca_camera::start_exposure_proc() {
  _io.reset();
  uint32_t r = 0;
  spdlog::trace("start_exposure_proc() invoked");
  spdlog::trace("Invoking ExpQHYCCDSingleFrame");

  auto time_stamp = std::chrono::system_clock::now();

  _last_exposure_start_time_fits = fmt::format("{:%FT%T}", time_stamp);
  spdlog::trace("Start Time of exposure: {0}", _last_exposure_start_time_fits);

  r = ExpQHYCCDSingleFrame(_cam_handle);
  double remaining = GetQHYCCDExposureRemaining(_cam_handle);
  long wait_time = 0;
  if (remaining > 100)
    wait_time = remaining / 1000000;

  spdlog::trace("ExpQHYCCDSingleFrame returned and {0}s remaining", wait_time);
  if (r == QHYCCD_SUCCESS || r == QHYCCD_READ_DIRECTLY) {
    asio::steady_timer __t(_io, asio::chrono::seconds(wait_time));
    // I may need to have the __t as a member so that I can control the expiry
    // of the timer when an exposure is canceled and the image must be read out
    __t.async_wait(std::bind(&qhy_alpaca_camera::read_image_from_camera, this));
    _io.run();
    spdlog::trace("start_exposure_proc ended");
    return 0;
  }
  spdlog::error("Failed to start exposing frame: {0}", r);
  uint8_t cam_status_buf[4] = {};
  GetQHYCCDCameraStatus(_cam_handle, cam_status_buf);
  spdlog::trace("Camera Status: {0}, {1}, {2}, {3}", cam_status_buf[0],
                cam_status_buf[1], cam_status_buf[3], cam_status_buf[3]);
  return -1;
}

// TODO: need to clean up the implementation
// The threading code is a little kludgy at the moment
int qhy_alpaca_camera::start_exposure(double duration_seconds, bool is_light) {
  std::lock_guard lock(_cam_mutex);
  _camera_state = camera_state_enum::CAMERA_EXPOSING;
  spdlog::debug("Setting Exposure to: {0} seconds", duration_seconds);

  // _bpp = 16;
  uint32_t set_bits_res = SetQHYCCDBitsMode(_cam_handle, _bpp);
  if (set_bits_res == QHYCCD_SUCCESS) {
    spdlog::trace("Successfully set bits mode");
  } else {

    spdlog::error("Failed to set bits mode: {0}", set_bits_res);
    // throw alpaca_exception(
    //     alpaca_exception::DRIVER_ERROR,
    //     fmt::format("Failed to set bits mode: {0}", set_bits_res));
  }

  _last_exposure_duration = duration_seconds;

  double u_seconds = duration_seconds * 1000000;
  spdlog::trace("Exposure time in uSeconds: {0}", u_seconds);
  uint32_t r = 0;

  // r = SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_USBTRAFFIC, 30);
  r = SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_TRANSFERBIT, _bpp);
  r = SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_EXPOSURE, u_seconds);

  if (r == QHYCCD_SUCCESS) {
    // Thread
    // TODO: I think this thread is getting orphaned potentially which
    // probably creates the memory leak / memory violation I'm seeing in
    // valgrind
    std::thread t(std::bind(&qhy_alpaca_camera::start_exposure_proc, this));
    t.detach();
    return 0;
  }
  spdlog::error("Failed to set exposure duration. {0}", r);
  return -1;
}

// TODO: need to interrupt the exposing thread...
int qhy_alpaca_camera::stop_exposure() {
  uint32_t r = CancelQHYCCDExposing(_cam_handle);
  if (r == QHYCCD_SUCCESS) {
    spdlog::debug(
        "Successfully stopped exposure. Image data must be read now.");
    return 0;
  }
  spdlog::error("Problem stopping exposure: {0}", r);
  return -1;
};

// TODO: implement color specifics
int qhy_alpaca_camera::sensor_type() {

  uint32_t r = QHYCCD_ERROR;
  // r = IsQHYCCDControlAvailable(_cam_handle, CONTROL_ID::CAM_COLOR);
  // if(r != QHYCCD_ERROR) {
  //     switch (r) {
  //     case: CONTROL_ID
  //     }
  // }

  // Monochrome
  return 0;
}

std::string qhy_alpaca_camera::sensor_name() { return _sensor_name; }

std::string qhy_alpaca_camera::get_camera_model_name() {
  return _qhy_model_name;
}

int qhy_alpaca_camera::set_resolution(const uint32_t start_x,
                                      const uint32_t start_y,
                                      const uint32_t num_x,
                                      const uint32_t num_y) {
  // std::lock_guard lock(_cam_mutex);
  uint32_t set_result = QHYCCD_ERROR;

  spdlog::debug(
      "Setting resolution with start_x: {0} start_y: {1} num_x: {2} num_y: {3}",
      start_x, start_y, num_x, num_y);
  set_result = SetQHYCCDResolution(_cam_handle, start_x, start_y, num_x, num_y);
  if (set_result == QHYCCD_SUCCESS) {
    spdlog::trace("Successfully set QHYCCD Resolution {0} x {1}", num_x, num_y);
    return 0;
  } else {
    spdlog::error(
        "failed to set QHYCCD Resolution to: {0}x{1} start_x:{2} start_y:{3}",
        num_x, num_y, start_x, start_y, set_result);
    return -1;
  }
}

// TODO: implement a better description
std::string qhy_alpaca_camera::description() { return "Description of device"; }

// TODO: implement a better driver info
std::string qhy_alpaca_camera::driverinfo() { return "Driver Information"; }

// TODO: implement an actual name value
std::string qhy_alpaca_camera::name() { return _qhy_model_name; }

double qhy_alpaca_camera::exposure_max() { return _exposure_max / 1000000; }

double qhy_alpaca_camera::exposure_min() { return _exposure_min / 1000000; }

double qhy_alpaca_camera::exposure_resolution() {
  return _exposure_step_size / 1000000;
};

double qhy_alpaca_camera::subexposure_duration() {
  return _subexposure_duration;
}

int qhy_alpaca_camera::set_subexposure_duration(double duration_seconds) {
  std::lock_guard lock(_cam_mutex);
  double u_seconds = duration_seconds * 1000000;
  spdlog::trace("Exposure time in uSeconds: {0}", u_seconds);
  uint32_t r = QHYCCD_ERROR;

  r = SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_EXPOSURE, u_seconds);
  if (r == QHYCCD_SUCCESS) {

    return 0;
  }
  return -1;
}

bool qhy_alpaca_camera::can_fast_readout() {
  // throw std::runtime_error("Fast readout not implemented");
  return false; //_can_fast_readout;
};

bool qhy_alpaca_camera::fast_readout() {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Fast readout not implemented");
  // return _fast_readout;
};

int qhy_alpaca_camera::set_fast_readout(bool fast_readout) {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "Fast readout not implemented");
};

uint32_t qhy_alpaca_camera::gain() { return _gain; };

uint32_t qhy_alpaca_camera::gain_max() { return _gain_max; };

uint32_t qhy_alpaca_camera::gain_min() { return _gain_min; };

// TODO: need to decide if I'm going to support gains vs min max
std::vector<std::string> qhy_alpaca_camera::gains() {
  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED, "can't get gains");
  return std::vector<std::string>();
};

int qhy_alpaca_camera::set_gain(uint32_t gain) {
  std::lock_guard lock(_cam_mutex);
  uint32_t r = QHYCCD_ERROR;
  if (gain >= _gain_min && gain <= _gain_max) {
    r = SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_GAIN, gain);
    if (r == QHYCCD_SUCCESS) {
      _gain = gain;
      return 0;
    }
  } else {
    throw alpaca_exception(alpaca_exception::INVALID_VALUE,
                           "attempted to set gain out of range");
  }
  return -1;
}

int qhy_alpaca_camera::set_offset(int offset_v) {
  std::lock_guard lock(_cam_mutex);
  if (offset_v < _offset_min || offset_v > _offset_max)
    throw alpaca_exception(
        alpaca_exception::INVALID_VALUE,
        fmt::format("Offset {0} provided is out of range", offset_v));
  uint32_t r = SetQHYCCDParam(_cam_handle, CONTROL_OFFSET, offset_v);
  if (r == QHYCCD_SUCCESS) {
    _offset = offset_v;
    return 0;
  }
  return -1;
}

int qhy_alpaca_camera::offset() { return _offset; }
int qhy_alpaca_camera::offset_max() { return _offset_max; };
int qhy_alpaca_camera::offset_min() { return _offset_min; };

std::vector<std::string> qhy_alpaca_camera::offsets() {

  throw alpaca_exception(alpaca_exception::NOT_IMPLEMENTED,
                         "not supporting offsets as a list");
  return _offsets;
}

int qhy_alpaca_camera::readout_mode() { return _readout_mode; }

int qhy_alpaca_camera::set_readout_mode(int idx) {
  std::lock_guard lock(_cam_mutex);
  try {
    spdlog::trace("Setting read mode to {0}", _read_mode_names.at(idx));
    uint32_t r = SetQHYCCDReadMode(_cam_handle, idx);
    if (r == QHYCCD_SUCCESS) {
      _readout_mode = idx;
      return 0;
    }
  } catch (std::exception ex) {
    spdlog::error("Problem setting read mode {0}", idx);
  }
  return -1;
}

std::vector<std::string> qhy_alpaca_camera::readout_modes() {
  return _read_mode_names;
}

uint8_t qhy_alpaca_camera::bpp() { return _bpp; }

int qhy_alpaca_camera::percent_complete() { return _percent_complete; }

int qhy_alpaca_camera::bayer_offset_x() { return _bayer_offset_x; }

int qhy_alpaca_camera::bayer_offset_y() { return _bayer_offset_y; }

std::vector<std::string> qhy_alpaca_camera::supported_actions() {
  return std::vector<std::string>();
}

// I think this needs to be removed
int qhy_alpaca_camera::set_cooler_power(double cooler_power) {
  std::lock_guard lock(_cam_mutex);
  double qhy_cooler_power = cooler_power / 100.0 * 255.0;
  uint32_t r = QHYCCD_ERROR;
  spdlog::debug("set_cooler_power invoked with {0}", cooler_power);
  r = SetQHYCCDParam(_cam_handle, CONTROL_ID::CONTROL_MANULPWM,
                     qhy_cooler_power);
  if (r == QHYCCD_SUCCESS) {
    _set_cooler_power = cooler_power;
    return 0;
  } else {
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           "Problem setting cooler power with SDK");
  }
  return -1;
}

bool qhy_alpaca_camera::has_filter_wheel() { return _has_filter_wheel; }

std::shared_ptr<qhy_alpaca_filterwheel> qhy_alpaca_camera::filter_wheel() {
  if (_has_filter_wheel) {
    return _filter_wheel;
  } else {
    throw alpaca_exception(alpaca_exception::DRIVER_ERROR,
                           "There is not a filterwheel attached. ");
  }
}

std::string qhy_alpaca_camera::unique_id() { return _camera_id; }