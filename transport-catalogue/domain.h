#pragma once

#include <stddef.h>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

enum RouteType {
  LINEAR,
  CIRCULAR,
};

struct Stop {
  std::string name;
  geo::Coordinates coords;
};

/**
 * Маршрут.
 * Бывает линейным, тогда он идёт так:
 * `S[0] -> S[1] -> ... -> S[n-2] -> S[n-1] -> S[n-2] -> ... -> S[1] -> S[0]`
 * где `S` это вектор `stops`.
 *
 * А также бывает кольцевым, тогда он идёт так:
 * `S[0] -> S[1] -> ... -> S[n-2] -> S[n-1] -> S[0]`
 */
struct Bus {
  std::string name;
  RouteType route_type = RouteType::LINEAR;
  // указатель смотрит на элемент `deque` в транспортном справочнике
  std::vector<const Stop*> stops;
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
