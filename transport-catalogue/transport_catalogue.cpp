#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <set>
#include <stdexcept>
#include <unordered_set>

#include "geo.h"

using namespace std;

namespace transport_catalogue {

/**
 * Добавить остановку в транспортный справочник.
 *
 * Кидает `invalid_argument`, если добавить одну и ту же остановку дважды.
 *
 * Параметр `name` принимается по значению, т.к. всё равно будет скопирован.
 */
void TransportCatalogue::AddStop(string name, geo::Coordinates coordinates) {
  if (stops_by_name_.count(name) > 0) {
    throw invalid_argument("stop "s + name + " already exists"s);
  }
  auto &ref = stops_.emplace_back(Stop{move(name), coordinates});
  stops_by_name_.emplace(string_view{ref.name}, &ref);
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
 *
 * Параметр `name` принимается по значению, т.к. всё равно будет скопирован.
 */
void TransportCatalogue::AddBus(string name, RouteType route_type,
                                const vector<string> &stop_names) {
  if (buses_by_name_.count(name) > 0) {
    throw invalid_argument("bus "s + name + " already exists"s);
  }
  if (stop_names.size() == 0) {
    throw invalid_argument("empty stop list"s);
  }
  if (route_type == RouteType::CIRCULAR && stop_names[0] != stop_names.back()) {
    throw invalid_argument(
        "first and last stop in circular routes must be the same"s);
  }
  vector<const Stop *> stops = ResolveStopNames(stop_names);

  // знаем (и проверили), что у кольцевых маршрутов последняя остановка
  // совпадает с первой, поэтому её можно не хранить.
  if (route_type == RouteType::CIRCULAR) {
    stops.resize(stops.size() - 1);
  }

  const auto &ref =
      buses_.emplace_back(Bus{move(name), route_type, move(stops)});
  buses_by_name_.emplace(ref.name, &ref);
  for (const Stop *stop : ref.stops) {
    auto &buses_for_stop = buses_for_stop_[stop];
    buses_for_stop.emplace(ref.name);
  }
}

/**
 * Переводит вектор с названиями остановок в вектор с указателями на
 * объект-остановку в справочнике.
 *
 * Если остановка с таким именем не найдена, кидает `invalid_argument`.
 */
vector<const Stop *> TransportCatalogue::ResolveStopNames(
    const vector<string> &stop_names) {
  vector<const Stop *> stops;
  stops.reserve(stop_names.size());
  for (const auto &stop_name : stop_names) {
    auto found_it = stops_by_name_.find(stop_name);
    if (found_it == stops_by_name_.end()) {
      throw invalid_argument("unknown bus stop "s + stop_name);
    }
    stops.push_back(found_it->second);
  }
  return stops;
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
  const Bus &bus = *it->second;
  const auto &stops = bus.stops;

  // считаем, что одна остановка не может храниться в справочнике дважды,
  // поэтому одинаковость остановок можно определить по равенству указателей на
  // неё
  unordered_set<const Stop *> uniq_stops{stops.begin(), stops.end()};

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

      // для подсчёта длины кольцевого маршрута нужно ещё добавить длину между
      // последней и первой остановками
      auto [real, crow] = CalcDistance(stops.back(), stops[0]);
      route_length += real;
      crow_route_length += crow;
      break;
  }
  return BusStats{stops_count, uniq_stops.size(), route_length,
                  crow_route_length};
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
  auto stop_it = stops_by_name_.find(stop_name);
  if (stop_it == stops_by_name_.end()) {
    return nullopt;
  }
  auto buses_it = buses_for_stop_.find(stop_it->second);

  // если элемента в мапе нет, значит через остановку не проходит ни один
  // маршрут, и в этом случае возвращаем пустое множество.
  if (buses_it == buses_for_stop_.end()) {
    return BusesForStop{};
  }
  return buses_it->second;
}

/**
 * Задать реальное расстояние от остановки `from` до `to` в метрах.
 */
void TransportCatalogue::SetDistance(std::string_view from, std::string_view to,
                                     size_t distance) {
  auto from_it = stops_by_name_.find(from);
  if (from_it == stops_by_name_.end()) {
    throw invalid_argument{"unknown stop "s + string(from)};
  }
  auto to_it = stops_by_name_.find(to);
  if (to_it == stops_by_name_.end()) {
    throw invalid_argument{"unknown stop "s + string(to)};
  }
  detail::StopDisKey key{from_it->second, to_it->second};
  if (real_distances_.count(key) > 0) {
    throw invalid_argument{"distance between "s + string(from) + " and "s +
                           string(to) + " has already been set"};
  }
  real_distances_.emplace(key, distance);
}

/**
 * Возвращает вектор с указателями на все маршруты в справочнике.
 */
vector<const Bus *> TransportCatalogue::GetBuses() const {
  vector<const Bus *> result;
  result.reserve(buses_.size());
  transform(buses_.begin(), buses_.end(), back_inserter(result),
            [](const Bus &bus) { return &bus; });
  return result;
}

vector<const Stop *> TransportCatalogue::GetStops() const {
  vector<const Stop *> result;
  result.reserve(stops_.size());
  transform(stops_.begin(), stops_.end(), back_inserter(result),
            [](const Stop &stop) { return &stop; });
  return result;
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
pair<double, double> TransportCatalogue::CalcDistance(const Stop *from,
                                                      const Stop *to) const {
  // as the crow flies
  double crow_dis = geo::ComputeDistance(from->coords, to->coords);
  double real_dis = crow_dis;
  auto it = real_distances_.find({from, to});
  if (it == real_distances_.end()) {
    it = real_distances_.find({to, from});
  }
  if (it != real_distances_.end()) {
    real_dis = it->second;
  }
  return {real_dis, crow_dis};
}

double TransportCatalogue::GetRealDistance(const Stop *from,
                                           const Stop *to) const {
  return CalcDistance(from, to).first;
}

}  // namespace transport_catalogue
