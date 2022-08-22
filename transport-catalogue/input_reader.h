#pragma once

#include <deque>
#include <istream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace trans_cat::input_reader {
/**
 * Команда на добавление остановки в транспортный справочник.
 */
struct AddStopCmd {
  typedef std::pair<std::string_view, unsigned int> Distance;
  /**
   * Имя остановки.
   *
   * `string_view` ссылается на строку в родительском парсере, например в `DbFromTextStream`
   */
  std::string_view name;
  /**
   * Координаты остановки.
   */
  geo::Coordinates coordinates;
  /**
   * Расстояния до соседних остановок.
   * Первый элемент пары - название соседней остановки, второй - расстояние в метрах.
   *
   * `string_view` ссылается на строку в родительском парсере, например в `DbFromTextStream`
   */
  std::vector<Distance> distances;
};

/**
 * Команда на добавление маршрута в транспортный справочник.
 */
struct AddBusCmd {
  /**
   * Название маршрута.
   *
   * `string_view` ссылается на строку в родительском парсере, например в `DbFromTextStream`
   */
  std::string_view name;
  /**
   * Тип маршрута: кольцевой или линейный.
   */
  RouteType route_type;
  /**
   * Список названий остановок в маршруте по порядку.
   * В кольцевом маршруте первая и последняя остановка совпадают.
   *
   * `string_view` ссылается на строку в родительском парсере, например в `DbFromTextStream`
   */
  std::vector<std::string_view> stop_names;
};

namespace from_char_stream {

/**
 * Класс-парсер команд на добавление данных в текстовый справочник из символьного потока.
 *
 * Формат входных данных таков:
 * ```
 * <N>\n
 * <команда 1>\n
 * <команда 2>\n
 * ...
 * <команда N>\n
 * ```
 * где `N` - число команд на добавление данных.
 *
 * Каждая команда начинается со слова "Stop" или "Bus", которая определяет тип команды.
 *
 * Формат команды на добавление остановки такой:
 * `Stop <название остановки>: <широта>, <долгота>, <р1>m to <с1>, ...`
 * где `p1` это целое число, расстояние до соседней остановки `c1` в метрах.
 * Список расстояний до соседних остановок может отсутствовать.
 *
 * Формат команды на добавление маршрута такой:
 * `Bus <название маршрута>: <остановка 1> - <остановка 2> - ...`
 * Если маршрут кольцевой, то название первой и последней остановки должны совпадать,
 * а в качестве разделителя используется символ ">" вместо "-".
 */
class DbReader {
 public:
  /**
   * Парсит команды из символьного потока `sin`.
   */
  DbReader(std::istream &sin)
      :
      sin_(sin) {
    Parse();
  }

  /**
   * Список команд на добавление остановки.
   */
  const std::vector<AddStopCmd>& GetAddStopCmds() const {
    return add_stop_cmds_;
  }

  /**
   * Список команд на добавление маршрута.
   */
  const std::vector<AddBusCmd>& GetAddBusCmds() const {
    return add_bus_cmds_;
  }
 private:
  void Parse();

  std::istream &sin_;
  std::deque<std::string> raw_strings_;
  std::vector<AddStopCmd> add_stop_cmds_;
  std::vector<AddBusCmd> add_bus_cmds_;
};

namespace detail {

std::vector<std::string_view> SplitNoWS(std::string_view line,
                                        std::string_view by);
std::vector<std::string_view> SplitNoWS(std::string_view line, char by);

}  // namespace trans_cat::input_reader::from_char_stream::detail

}  // namespace trans_cat::input_reader::from_char_stream

}  // namespace trans_cat::input_reader
