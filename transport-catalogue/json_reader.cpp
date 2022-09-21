#include "json_reader.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

#include "domain.h"
#include "json.h"

using namespace std;

namespace transport_catalogue::json_reader {

namespace detail {

using namespace transport_catalogue::request_handler;

AddStopCmd ParseAddStopCmd(const json::Dict &request) {
  vector<AddStopCmd::Distance> distances;

  for (const auto& [stop_name, node] : request.at("road_distances"s).AsMap()) {
    distances.emplace_back(stop_name, node.AsInt());
  }

  return {
    request.at("name"s).AsString(),
    {
      request.at("latitude"s).AsDouble(), request.at("longitude"s).AsDouble()
    },
    move(distances),
  };
}

AddBusCmd ParseAddBusCmd(const json::Dict &request) {
  vector<string> stop_names;

  for (const auto &node : request.at("stops"s).AsArray()) {
    stop_names.emplace_back(node.AsString());
  }

  return {
    request.at("name"s).AsString(),
    request.at("is_roundtrip"s).AsBool() ? RouteType::CIRCULAR : RouteType::LINEAR,
    move(stop_names),
  };
}

vector<BaseRequest> ParseBaseRequests(const json::Array &base_requests) {
  vector<BaseRequest> result;

  for (const auto &node : base_requests) {
    const auto &request = node.AsMap();
    const auto &type = request.at("type"s).AsString();

    if (type == "Stop"s) {
      result.emplace_back(ParseAddStopCmd(request));
    } else if (type == "Bus"s) {
      result.emplace_back(ParseAddBusCmd(request));
    } else {
      throw invalid_argument("Unknown base request with type '"s + type + "'"s);
    }
  }

  return result;
}

StopStatRequest ParseStopStatRequest(const json::Dict &request) {
  return {
    request.at("id"s).AsInt(),
    request.at("name"s).AsString()
  };
}

BusStatRequest ParseBusStatRequest(const json::Dict &request) {
  return {
    request.at("id"s).AsInt(),
    request.at("name"s).AsString()
  };
}

vector<StatRequest> ParseStatRequests(const json::Array &stat_requests) {
  vector<StatRequest> result;

  for (const auto &node : stat_requests) {
    const auto &request = node.AsMap();
    const auto &type = request.at("type"s).AsString();

    if (type == "Stop"s) {
      result.emplace_back(ParseStopStatRequest(request));
    } else if (type == "Bus"s) {
      result.emplace_back(ParseBusStatRequest(request));
    } else {
      throw invalid_argument("Unknown base request with type '"s + type + "'"s);
    }
  }

  return result;
}

/**
 * Принтер разных вариантов овтетов на запросы статистики.
 * Штука для `std::visit`.
 */
struct ResponseVariantPrinter {
  int request_id;
  ostream &out;

  void operator()(std::monostate) {
    auto dict = PrepareCommonDict();
    dict["error_message"s] = "not found"s;
    json::Print(json::Document { dict }, out);
  }

  void operator()(const StopStatResponse &response) {
    auto dict = PrepareCommonDict();

    const auto &buses_internal = response.buses_for_stop;
    json::Array buses;
    buses.reserve(buses_internal.size());
    transform(buses_internal.begin(), buses_internal.end(),
              back_inserter(buses), [](string_view bus_name) {
                return json::Node { string { bus_name } };
              });

    dict["buses"s] = move(buses);

    json::Print(json::Document { dict }, out);
  }

  void operator()(const BusStatResponse &response) {
    auto dict = PrepareCommonDict();
    const auto &bus_stats = response.bus_stats;
    dict["curvature"s] = bus_stats.route_length / bus_stats.crow_route_length;
    dict["route_length"s] = bus_stats.route_length;
    dict["stop_count"s] = static_cast<int>(bus_stats.stops_count);
    dict["unique_stop_count"s] = static_cast<int>(bus_stats.unique_stops_count);
    json::Print(json::Document { dict }, out);
  }

  json::Dict PrepareCommonDict() {
    return {
      { "request_id"s, request_id}
    };
  }
};

}  // namespace transport_catalogue::json_reader::detail

/**
 * Парсит запросы к транспортному справочнику в JSON формате
 * и складывает их в соответствующих векторах.
 */
void BufferingRequestReader::Parse(istream &sin) {
  const auto document = json::Load(sin);
  const auto &root = document.GetRoot().AsMap();
  base_requests_ = detail::ParseBaseRequests(
      root.at("base_requests"s).AsArray());
  stat_requests_ = detail::ParseStatRequests(
      root.at("stat_requests"s).AsArray());
}

/**
 * `out` - символьный поток, куда будут выведены ответы на запросы
 */
ResponsePrinter::ResponsePrinter(std::ostream &out)
    :
    out_(out) {
  Begin();
}

void ResponsePrinter::PrintResponse(int request_id,
                                    const StatResponse &response) {
  if (printed_something_) {
    out_.put(',');
  }
  detail::ResponseVariantPrinter printer { request_id, out_ };
  std::visit(printer, response);
  printed_something_ = true;
}

ResponsePrinter::~ResponsePrinter() {
  End();
}

/**
 * Начинает вывод ответов на запросы к транспортному справочнику в JSON формате.
 */
void ResponsePrinter::Begin() {
  out_.put('[');
}

/**
 * Завершает вывод ответов на запросы к транспортному справочнику в JSON формате.
 */
void ResponsePrinter::End() {
  out_.put(']');
}

}
