#ifndef I_ALPACA_DEVICE_HPP
#define I_ALPACA_DEVICE_HPP

#include "common/alpaca_hub_common.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <map>

// Basic interface for Alpaca devices

enum pier_side_enum : int { east = 0, west = 1, unknown = -1 };

struct axis_rate {
  double Max;
  double Min;
  axis_rate(const double &max_, const double &min_) : Max(max_), Min(min_){};
};

enum guide_direction_enum : int {
  guide_north = 0,
  guide_south = 1,
  guide_east = 2,
  guide_west = 3
};

enum drive_rate_enum : int { sidereal = 0, lunar = 1, solar = 2, king = 3 };

enum alignment_mode_enum : int { alt_az = 0, polar = 1, german_polar = 2 };

enum equatorial_system_enum : int {
  // Custom or unknown equinox and/or reference frame.
  other = 0,

  // Topocentric coordinates. Coordinates of the object at the current
  // date having allowed for annual aberration, precession and
  // nutation. This is the most common coordinate type for amateur
  // telescopes.
  topocentric = 1,

  // J2000 equator/equinox. Coordinates of the object at mid-day on 1st
  // January
  // 2000, ICRS reference frame.
  j2000 = 2,

  // J2050 equator/equinox, ICRS reference frame.
  j2050 = 3,

  // B1950 equinox, FK4 reference frame.
  b1950 = 4,

  // Obsolete. Please use equTopocentric instead - see Astronomical
  // Coordinates
  // for an explanation.
  local_topocentric = 1
};

enum telescope_axes_enum : int { primary = 0, secondary = 1, tertiary = 2 };

// I think this should probably go away...
using device_mgmt_list_entry_t =
    std::map<std::string, std::variant<std::string, int>>;

// Takes a variant for the first arg and creates a new variant
// that is composed of the original variant and the list of
// types after
// Commenting out as I am using a different version of this below
// template <typename... Args0, typename... Args1>
// struct variant_concatenator<std::variant<Args0...>, Args1...> {
//   using type = std::variant<Args0..., Args1...>;
// };

template <typename T, typename... Args> struct variant_concatenator;

template <typename... Args0, typename... Args1>
struct variant_concatenator<std::variant<Args0...>, std::variant<Args1...>> {
  using type = std::variant<Args0..., Args1...>;
};

template <typename T> struct vectors_of_type_list;

template <typename... Args0>
struct vectors_of_type_list<std::variant<Args0...>> {
  using type = std::variant<std::vector<Args0>...>;
};

template <typename Key_T, typename T> struct maps_of_type_list;

template <typename Key_T, typename... Args0>
struct maps_of_type_list<Key_T, std::variant<Args0...>> {
  using type = std::variant<std::map<Key_T, std::variant<Args0...>>,
                            std::map<Key_T, Args0>...>;
};

// This is where we add supported primitives / integral types to
// our variants which will propogate into collections like vectors and maps
using device_value_t =
    std::variant<drive_rate_enum, pier_side_enum, telescope_axes_enum,
                 axis_rate, long unsigned int, bool, uint8_t, uint16_t,
                 uint32_t, int, long, double, std::string,
                 device_mgmt_list_entry_t, std::vector<uint32_t>>;

// Generate variant of std::vector with each of the types above
using device_vector_of_values_t = vectors_of_type_list<device_value_t>::type;
using device_variant_intermediate_t =
    variant_concatenator<device_value_t, device_vector_of_values_t>::type;
// Generate map variants
using device_map_to_variants =
    maps_of_type_list<std::string, device_variant_intermediate_t>::type;

using device_variant_t = variant_concatenator<device_variant_intermediate_t,
                                              device_map_to_variants>::type;

class i_alpaca_device {
public:
  virtual bool connected() = 0;
  virtual int set_connected(bool) = 0;
  virtual std::string description() = 0;
  virtual std::string driverinfo() = 0;
  virtual std::string name() = 0;
  virtual uint32_t interface_version() = 0;
  virtual std::string driver_version() = 0;
  virtual ~i_alpaca_device(){};
  virtual std::vector<std::string> supported_actions() = 0;
  virtual std::string unique_id() = 0;
  virtual std::map<std::string, device_variant_t> details() = 0;

};

#endif
