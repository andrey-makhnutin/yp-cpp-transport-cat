#pragma once

#include <stddef.h>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "domain.h"
#include "geo.h"

namespace transport_catalogue {
class TransportCatalogue;
} /* namespace transport_catalogue */

namespace transport_catalogue::request_handler {

//TODO: убрать дублирующую структуру из input_reader.h
/**
 * Команда на добавление остановки в транспортный справочник.
 */
struct AddStopCmd {

  /**
   * Первый элемент - название остановки
   * второй - расстояние до неё в метрах
   */
  using Distance = std::pair<std::string, size_t>;

  /**
   * Имя остановки.
   */
  std::string name;

  /**
   * Координаты остановки.
   */
  geo::Coordinates coordinates;

  /**
   * Расстояния до соседних остановок.
   */
  std::vector<Distance> distances;
};

//TODO: убрать дублирующую структуру из input_reader.h
/**
 * Команда на добавление маршрута в транспортный справочник.
 */
struct AddBusCmd {

  /**
   * Название маршрута.
   */
  std::string name;

  /**
   * Тип маршрута: кольцевой или линейный.
   */
  RouteType route_type = RouteType::LINEAR;

  /**
   * Список названий остановок в маршруте по порядку.
   * В кольцевом маршруте первая и последняя остановка совпадают.
   */
  std::vector<std::string> stop_names;
};

/**
 * Базовый класс для всех запросов статистики к транспортному справочнику.
 */
struct BaseStatRequest {
  int id = 0;
};

/**
 * Запрос на статистику по маршруту.
 */
struct BusStatRequest : public BaseStatRequest {
  std::string name;
};

/**
 * Запрос на статистику по остановке.
 */
struct StopStatRequest : public BaseStatRequest {
  std::string name;
};

/**
 * Все возможные типы запросов на наполнеие базы транспортного справочника.
 */
using BaseRequest = std::variant<AddStopCmd, AddBusCmd>;

/**
 * Все возможные типы запросов на получение статистики из транспортного справочника.
 */
using StatRequest = std::variant<BusStatRequest, StopStatRequest>;

/**
 * Базовый класс для получения запросов к транспортному справочнику.
 *
 * Классы-наследники получают все запросы от пользователя и складывают во внутренний буфер.
 */
class AbstractBufferingRequestReader {
 public:
  /**
   * Запросы на наполнение базы транспортного справочника.
   */
  virtual const std::vector<BaseRequest>& GetBaseRequests() const = 0;

  /**
   * Запросы на получение статистики из транспортного справочника.
   */
  virtual const std::vector<StatRequest>& GetStatRequests() const = 0;

 protected:
  // не разрешаем полиморфное владение наследниками этого класса. Незачем
  ~AbstractBufferingRequestReader() = default;
};

/**
 * Ответ на запрос на получение статистики по маршруту.
 */
struct BusStatResponse {
  BusStats bus_stats;
};

/**
 * Ответ на запрос на получение статистики по остановке.
 */
struct StopStatResponse {
  BusesForStop buses_for_stop;
};

/**
 * Все возможные типы ответов на запросы на получение статистики.
 *
 * `std::monostate` - значит, что сущность, по которой была запрошена статистика, не существует.
 */
using StatResponse = std::variant<std::monostate, BusStatResponse, StopStatResponse>;

/**
 * Базовый класс для печати ответов на запросы к транспортному справочнику.
 */
class AbstractStatResponsePrinter {
 public:
  virtual void PrintResponse(int request_id, const StatResponse&) = 0;
 protected:
  ~AbstractStatResponsePrinter() = default;
};

void ProcessRequests(TransportCatalogue &transport_catalogue,
                     const AbstractBufferingRequestReader &request_reader,
                     AbstractStatResponsePrinter &stat_response_printer);

}  // namespace transport_catalogue::request_handler