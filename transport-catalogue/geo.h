#pragma once

#include <cmath>

namespace transport_catalogue::geo {

namespace detail {

const double RAD_PER_DEG = M_PI / 180.0;

// радиус Земли в метрах
const double EARTH_RADIUS = 6371000.0;

/**
 * Точность, с которой сравниваем числа с плавающей запятой
 * не может быть меньше, чем 180 / (2 ** 52)
 */
const double FP_PRECISION = 1e-13;

}  // namespace detail

/**
 * Точность, с которой можно указывать координаты
 */
const double COORD_PRECISION = 0.000001;

/**
 * Координаты точки на земном "шаре".
 * Точки, отстоящие друг от друга меньше чем на 1 милиградус
 * по широте и долготе считаются равными, тк функция `ComputeDistance`
 * не сможет рассчитать дистанцию между точками с большей точностью.
 */
struct Coordinates {
  double lat = 0;
  double lng = 0;

  bool operator==(const Coordinates &other) const {
    using namespace std;
    using detail::FP_PRECISION;

    return std::abs(lat - other.lat) < (COORD_PRECISION - FP_PRECISION) &&
           std::abs(lng - other.lng) < (COORD_PRECISION - FP_PRECISION);
  }
  bool operator!=(const Coordinates &other) const { return !(*this == other); }
};

inline double ComputeDistance(Coordinates from, Coordinates to) {
  using namespace std;
  using namespace detail;

  if (from == to) {
    return 0;
  }

  return acos(sin(from.lat * RAD_PER_DEG) * sin(to.lat * RAD_PER_DEG) +
              cos(from.lat * RAD_PER_DEG) * cos(to.lat * RAD_PER_DEG) *
                  cos(abs(from.lng - to.lng) * RAD_PER_DEG)) *
         EARTH_RADIUS;
}

}  // namespace transport_catalogue::geo
