#pragma once

#include <cmath>

namespace trans_cat::geo {

namespace detail {

const double RAD_PER_DEG = 3.1415926535 / 180.0;
// радиус Земли в метрах
const double EARTH_RADIUS = 6371000.0;

}  // namespace trans_cat::geo::detail

struct Coordinates {
  double lat;
  double lng;
  bool operator==(const Coordinates &other) const {
    return lat == other.lat && lng == other.lng;
  }
  bool operator!=(const Coordinates &other) const {
    return !(*this == other);
  }
};

inline double ComputeDistance(Coordinates from, Coordinates to) {
  using namespace std;
  using namespace detail;

  if (from == to) {
    return 0;
  }
  return acos(
      sin(from.lat * RAD_PER_DEG) * sin(to.lat * RAD_PER_DEG)
          + cos(from.lat * RAD_PER_DEG) * cos(to.lat * RAD_PER_DEG)
              * cos(abs(from.lng - to.lng) * RAD_PER_DEG)) * EARTH_RADIUS;
}

}  // namespace trans_cat::geo
