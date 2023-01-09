#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "domain.h"

namespace transport_catalogue {

namespace detail {

/**
 * Ключ для мапы с расстояниями между остановками
 */
using StopDisKey = std::pair<const Stop *, const Stop *>;

/**
 * Хешер для `StopDisKey`
 */
struct SDKHasher {
  size_t operator()(const StopDisKey sdk) const {
    return reinterpret_cast<size_t>(sdk.first) +
           reinterpret_cast<size_t>(sdk.second) * 43;
  }
};

}  // namespace detail

class TransportCatalogue {
 public:
  void AddStop(std::string name, geo::Coordinates);
  void AddBus(std::string name, RouteType route_type,
              const std::vector<std::string> &stop_names);
  void SetDistance(std::string_view from, std::string_view to, size_t distance);
  std::optional<BusStats> GetBusStats(std::string_view bus_name) const;
  std::optional<BusesForStop> GetStopInfo(std::string_view stop_name) const;
  std::vector<const Bus *> GetBuses() const;
  std::vector<const Stop *> GetStops() const;
  double GetRealDistance(const Stop *from, const Stop *to) const;

 private:
  /**
   * коллекция уникальных остановок. Важно, чтобы коллекция была `deque`,
   * чтобы указатели на элементы коллекции не инвалидировались при добавлении
   * новых элементов.
   */
  std::deque<Stop> stops_;

  /**
   * мапа <имя остановки> -> <указатель на остановку>
   * `string_view` смотрит на строку в самой структуре остановки
   */
  std::unordered_map<std::string_view, Stop *> stops_by_name_;

  /**
   * коллекция уникальных маршрутов. Важно, чтобы коллекция была `deque`,
   * чтобы указатели на элементы коллекции не инвалидировались при добавлении
   * новых элементов.
   */
  std::deque<Bus> buses_;

  /**
   * мапа <имя маршрута> -> <указатель на маршрут>
   * `string_view` смотрит на строку в самой структуре маршрута
   */
  std::unordered_map<std::string_view, const Bus *> buses_by_name_;

  /**
   * мапа с реальным расстоянием между остановками `first` и `second`
   */
  std::unordered_map<detail::StopDisKey, unsigned int, detail::SDKHasher>
      real_distances_;

  /**
   * мапа с набором маршрутов, проходящих через определённую остановку.
   * Если через остановку не проходит ни один маршрут, соответствующего
   * элемента в мапе не будет.
   */
  std::unordered_map<const Stop *, BusesForStop> buses_for_stop_;

  std::pair<double, double> CalcDistance(const Stop *from,
                                         const Stop *to) const;
  std::vector<const Stop *> ResolveStopNames(
      const std::vector<std::string> &stop_names);
};

}  // namespace transport_catalogue
