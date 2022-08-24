#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace transport_catalogue {

/**
 * Добавить остановку в транспортный справочник.
 *
 * Кидает `invalid_argument`, если добавить одну и ту же остановку дважды.
 */
void TransportCatalogue::AddStop(string_view name,
                                 geo::Coordinates coordinates) {
  if (stops_by_name_.count(name) > 0) {
    throw invalid_argument("stop "s + string { name } + " already exists"s);
  }
  auto &ref = stops_.emplace_back(detail::Stop { string { name }, coordinates,
      { } });
  stops_by_name_.emplace(string_view { ref.name }, &ref);
}

/**
 * Добавить маршрут в транспортный справочник.
 * Кольцевой маршрут должен оканчиваться той же остановкой, с какой начинается.
 *
 * Кидает `invalid_argument` если:
 *  - добавить маршрут с тем же названием дважды;
 *  - добавить маршрут с остановкой, которой ещё нет в справочнике;
 *  - добавить маршрут без остановок;
 *  - добавить кольцевой маршрут, где первая и последняя остановки не совпадают.
 */
void TransportCatalogue::AddBus(string_view name, RouteType route_type,
                                vector<string_view> stop_names) {
  if (buses_by_name_.count(name) > 0) {
    throw invalid_argument("bus "s + string { name } + " already exists"s);
  }
  if (stop_names.size() == 0) {
    throw invalid_argument("empty stop list"s);
  }
  if (route_type == RouteType::CIRCULAR && stop_names[0] != stop_names.back()) {
    throw invalid_argument(
        "first and last stop in circular routes must be the same"s);
  }

  vector<detail::Stop*> stops;
  stops.reserve(stop_names.size());
  for (auto stop_name : stop_names) {
    auto found_it = stops_by_name_.find(stop_name);
    if (found_it == stops_by_name_.end()) {
      throw invalid_argument("unknown bus stop "s + string { stop_name });
    }
    stops.push_back(found_it->second);
  }

  // знаем (и проверили), что у кольцевых маршрутов последняя остановка совпадает
  // с первой, поэтому её можно не хранить.
  if (route_type == RouteType::CIRCULAR) {
    stops.resize(stops.size() - 1);
  }

  const auto &ref = buses_.emplace_back(detail::Bus { string { name },
      route_type, move(stops) });
  buses_by_name_.emplace(string_view { ref.name }, &ref);
  for (auto &stop : ref.stops) {
    stop->buses.emplace(string_view { ref.name });
  }
}

/**
 * Получить информацию о маршруте.
 * Если запрошенного маршрута не существует, вернёт `std::nullopt`.
 */
optional<BusStats> TransportCatalogue::GetBusStats(string_view bus_name) const {
  auto it = buses_by_name_.find(bus_name);
  if (it == buses_by_name_.end()) {
    return nullopt;
  }
  const detail::Bus &bus = *it->second;
  const auto &stops = bus.stops;

  // считаем, что одна остановка не может храниться в справочнике дважды,
  // поэтому одинаковость остановок можно определить по равенству указателей на неё
  unordered_set<const detail::Stop*> uniq_stops { stops.begin(), stops.end() };

  size_t stops_count;
  double route_length = 0;
  double crow_route_length = 0;
  assert(stops.size() > 0);
  // считаем расстояние по маршруту в одну сторону. Это можно делать одинаково
  // для линейных и кольцевых маршрутов
  for (size_t i = 1; i < stops.size(); ++i) {
    auto [real, crow] = CalcDistance(stops[i - 1], stops[i]);
    route_length += real;
    crow_route_length += crow;
  }
  switch (bus.route_type) {
    case RouteType::LINEAR:
      stops_count = stops.size() * 2 - 1;
      // для подсчёта длины линейного маршрута нужно ещё раз пройти по маршруту,
      // но уже в обратную сторону.
      for (size_t i = stops.size() - 1; i >= 1; --i) {
        auto [real, _] = CalcDistance(stops[i], stops[i - 1]);
        route_length += real;
      }
      // расстояние по прямой можно просто умножить на два
      crow_route_length *= 2;
      break;
    case RouteType::CIRCULAR:
      stops_count = stops.size() + 1;
      // для подсчёта длины кольцевого маршрута нужно ещё добавить длину между последней
      // и первой остановками
      auto [real, crow] = CalcDistance(stops.back(), stops[0]);
      route_length += real;
      crow_route_length += crow;
      break;
  }
  return BusStats { stops_count, uniq_stops.size(), route_length,
      crow_route_length };
}

/**
 * Получить информацию об остановке: отсортированную коллекцию названий
 * уникальных маршрутов, которые проходят через запрошенную остановку.
 * Если запрошенной остановки не существует, вернёт `std::nullopt`.
 *
 * `string_view` в возвращаемом значении смотрит на строку внутри справочника.
 */
std::optional<BusesForStop> TransportCatalogue::GetStopInfo(
    std::string_view stop_name) const {
  auto it = stops_by_name_.find(stop_name);
  if (it == stops_by_name_.end()) {
    return nullopt;
  }
  const detail::Stop &stop = *it->second;

  return stop.buses;
}

/**
 * Задать реальное расстояние от остановки `from` до `to` в метрах.
 */
void TransportCatalogue::SetDistance(std::string_view from, std::string_view to,
                                     size_t distance) {
  auto from_it = stops_by_name_.find(from);
  if (from_it == stops_by_name_.end()) {
    throw invalid_argument { "unknown stop "s + string { from } };
  }
  auto to_it = stops_by_name_.find(to);
  if (to_it == stops_by_name_.end()) {
    throw invalid_argument { "unknown stop "s + string { to } };
  }
  detail::StopDisKey key { from_it->second, to_it->second };
  if (real_distances_.count(key) > 0) {
    throw invalid_argument { "distance between "s + string { from } + " and "s
        + string { to } + " has already been set" };
  }
  real_distances_.emplace(key, distance);
}

/**
 * Рассчитать расстояние от остановки `from` до `to` в метрах.
 * Учитываются реальные расстояния, заданные с помощью функции `SetDistance`
 * между остановками (в ту или обратную сторону). А если таковых нет,
 * считается расстояние по "прямой".
 *
 * Возвращает пару, где первый элемент - расстояние с учётом реальных данных,
 * а второй - расстояние по "прямой".
 */
pair<double, double> TransportCatalogue::CalcDistance(
    const detail::Stop *from, const detail::Stop *to) const {
  // as the crow flies
  double crow_dis = geo::ComputeDistance(from->coords, to->coords);
  double real_dis = crow_dis;
  auto it = real_distances_.find( { from, to });
  if (it == real_distances_.end()) {
    it = real_distances_.find( { to, from });
  }
  if (it != real_distances_.end()) {
    real_dis = it->second;
  }
  return {real_dis, crow_dis};
}

}  // namespace transport_catalogue
