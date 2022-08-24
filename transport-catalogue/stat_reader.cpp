#include "stat_reader.h"

#include <stddef.h>
#include <cassert>
#include <iomanip>
#include <set>
#include <string>

using namespace std;

namespace transport_catalogue::stat_reader {

namespace from_char_stream {

namespace detail {

/**
 * Удаляет пробельные символы в конце строки.
 */
void RTrimStr(string &str) {
  size_t last_nonws = str.find_last_not_of(" \t\r\n");
  if (last_nonws != str.npos) {
    str.resize(last_nonws + 1);
  }
}

}  // namespace transport_catalogue::stat_reader::from_char_stream::detail

/**
 * Парсит запросы на получение статистики из потока `sin_`,
 * отправляет их в транспортный справочник, и печатает результат
 * с помощью `stats_printer`.
 */
void StatsRequestProcessor::ProcessRequests(
    const TransportCatalogue &transport_catalogue,
    to_char_stream::StatsPrinter &stats_printer) {
  int req_count;
  sin_ >> req_count;
  while (req_count--) {
    string cmd;
    sin_ >> cmd;
    if (cmd == "Bus"s) {
      string bus_name;
      getline(sin_ >> ws, bus_name);
      detail::RTrimStr(bus_name);
      const auto &bus_stats = transport_catalogue.GetBusStats(bus_name);
      stats_printer.PrintBusStats(bus_name, bus_stats);
    }
    if (cmd == "Stop"s) {
      string stop_name;
      getline(sin_ >> ws, stop_name);
      detail::RTrimStr(stop_name);
      const auto &buses_for_stop = transport_catalogue.GetStopInfo(stop_name);
      stats_printer.PrintStopInfo(stop_name, buses_for_stop);
    }
  }
}

}  // namespace transport_catalogue::stat_reader::from_char_stream

namespace to_char_stream {

void StatsPrinter::PrintBusStats(string_view bus_name,
                                 const optional<BusStats> &bus_stats) {
  sout_ << "Bus " << bus_name << ": ";
  if (bus_stats.has_value()) {
    assert(bus_stats->crow_route_length > 0);
    auto old_precision = sout_.precision();
    sout_ << setprecision(6) << bus_stats->stops_count << " stops on route, "
          << bus_stats->unique_stops_count << " unique stops, "
          << bus_stats->route_length << " route length, "
          << (bus_stats->route_length / bus_stats->crow_route_length)
          << " curvature" << setprecision(old_precision) << endl;
  } else {
    sout_ << "not found" << endl;
  }
}

void StatsPrinter::PrintStopInfo(
    std::string_view stop_name,
    const std::optional<BusesForStop> &buses_for_stop) {
  sout_ << "Stop " << stop_name << ": ";
  if (!buses_for_stop.has_value()) {
    sout_ << "not found" << endl;
    return;
  }
  if (buses_for_stop->size() == 0) {
    sout_ << "no buses" << endl;
    return;
  }
  sout_ << "buses";
  for (const auto bus_name : *buses_for_stop) {
    sout_ << ' ' << bus_name;
  }
  sout_ << endl;
}

}  // namespace transport_catalogue::stat_reader::to_char_stream

}  // namespace transport_catalogue::stat_reader
