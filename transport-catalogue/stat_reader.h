#pragma once

#include <istream>
#include <optional>
#include <string_view>

#include "transport_catalogue.h"

namespace trans_cat::stat_reader {

namespace to_char_stream {

/**
 * Класс для печати статистики о маршрутах и остановках в символьный поток.
 * Ссылка на символьный поток для вывода указывается в конструкторе.
 */
class StatsPrinter {
 public:
  StatsPrinter(std::ostream &sout)
      :
      sout_(sout) {
  }

  void PrintBusStats(std::string_view bus_name,
                     const std::optional<BusStats> &bus_stats);
  void PrintStopInfo(std::string_view stop_name,
                     const std::optional<BusesForStop> &stop_info);
 private:
  std::ostream &sout_;
};

}  // namespace trans_cat::stat_reader::to_char_stream

namespace from_char_stream {

/**
 * Класс-обработчик запросов на получение статистики из транспортного справочника
 * из символьного потока.
 * Запросы обрабатываются методом `ProcessRequest`, который принимает ссылки
 * на объект транспортного справочника, к которому будет обращён запрос, и
 * на объект-принтер результатов.
 *
 * Формат ввода такой:
 * ```
 * N\n
 * <запрос 1>\n
 * <запрос 2>\n
 * ...
 * <запрос N>\n
 * ```
 * где N - количество запросов.
 *
 * Формат запроса такой:
 * `Bus <название маршрута>`
 * `Stop <название остановки>`
 */
class StatsRequestProcessor {
 public:
  StatsRequestProcessor(std::istream &sin)
      :
      sin_(sin) {
  }

  void ProcessRequests(
      const TransportCatalogue &tc,
      to_char_stream::StatsPrinter &stats_printer);
 private:
  std::istream &sin_;
};

}  // namespace trans_cat::stat_reader::from_char_stream

}  // namespace trans_cat::stat_reader
