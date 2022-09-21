#pragma once

#include <stddef.h>
#include <set>
#include <string_view>

namespace transport_catalogue {

enum RouteType {
  LINEAR,
  CIRCULAR,
};

/**
 * Информация о маршруте.
 */
struct BusStats {
  // сколько остановок в маршруте, включая первую
  size_t stops_count = 0;
  size_t unique_stops_count = 0;
  // длина маршрута в метрах
  double route_length = 0;
  // as the crow flies
  double crow_route_length = 0;
};

/**
 * Информация об остановке: отсортированная коллекция уникальных марштуров,
 * которые проходят через остановку.
 */
using BusesForStop = std::set<std::string_view>;

}  // namespace transport_catalogue
