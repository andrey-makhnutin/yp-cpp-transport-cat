#pragma once

#include <stddef.h>
#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "geo.h"

namespace trans_cat {

enum RouteType {
  LINEAR,
  CIRCULAR,
};

namespace detail {

struct Stop {
  std::string name;
  geo::Coordinates coords;
  std::set<std::string_view> buses;
};

/**
 * Ключ для мапы с расстояниями между остановками
 */
using StopDisKey = std::pair<const Stop*, const Stop*>;

/**
 * Хешер для `StopDisKey`
 */
struct SDKHasher {
  size_t operator()(const StopDisKey sdk) const {
    return reinterpret_cast<size_t>(sdk.first)
        + reinterpret_cast<size_t>(sdk.second) * 43;
  }
};

/**
 * Маршрут.
 * Бывает линейным, тогда он идёт так:
 * `S[0] -> S[1] -> ... -> S[n-2] -> S[n-1] -> S[n-2] -> S[1] -> S[0]`
 * где `S` это вектор `stops`.
 *
 * А также бывает кольцевым, тогда он идёт так:
 * `S[0] -> S[1] -> ... -> S[n-2] -> S[n-1] -> S[0]`
 */
struct Bus {
  std::string name;
  RouteType route_type;
  // указатель смотрит на элемент `deque` в транспортном справочнике
  std::vector<Stop*> stops;
};

}  // namespace trans_cat::detail

/**
 * Информация о маршруте.
 */
struct BusStats {
  // сколько остановок в маршруте, включая первую
  unsigned int stops_count;
  unsigned int unique_stops_count;
  // длина маршрута в метрах
  double route_length;
  // as the crow flies
  double crow_route_length;
};

/**
 * Информация об остановке: отсортированная коллекция уникальных марштуров,
 * которые проходят через остановку.
 */
typedef std::set<std::string_view> BusesForStop;

class TransportCatalogue {
 public:
  void AddStop(std::string_view name, double lat, double lng);
  void AddBus(std::string_view name, RouteType route_type,
              std::vector<std::string_view> stop_names);
  void SetDistance(std::string_view from, std::string_view to,
                   unsigned int distance);
  std::optional<BusStats> GetBusStats(std::string_view bus_name) const;
  std::optional<BusesForStop> GetStopInfo(std::string_view stop_name) const;
 private:
  /**
   * коллекция уникальных остановок. Важно, чтобы коллекция была `deque`,
   * чтобы указатели на элементы коллекции не инвалидировались при добавлении
   * новых элементов.
   */
  std::deque<detail::Stop> stops_;
  /**
   * мапа <имя остановки> -> <указатель на остановку>
   * `string_view` смотрит на строку в самой структуре остановки
   */
  std::unordered_map<std::string_view, detail::Stop*> stops_by_name_;

  /**
   * коллекция уникальных маршрутов. Важно, чтобы коллекция была `deque`,
   * чтобы указатели на элементы коллекции не инвалидировались при добавлении
   * новых элементов.
   */
  std::deque<detail::Bus> buses_;
  /**
   * мапа <имя маршрута> -> <указатель на маршрут>
   * `string_view` смотрит на строку в самой структуре маршрута
   */
  std::unordered_map<std::string_view, const detail::Bus*> buses_by_name_;
  /**
   * мапа с реальным расстоянием между остановками `first` и `second`
   */
  std::unordered_map<detail::StopDisKey, unsigned int, detail::SDKHasher> real_distances_;

  std::pair<double, double> CalcDistance(const detail::Stop *from,
                                         const detail::Stop *to) const;
};

}  // namespace trans_cat
