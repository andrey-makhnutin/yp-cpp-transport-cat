#include "json_reader.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>

#include "domain.h"
#include "json.h"
#include "json_builder.h"
#include "svg.h"

using namespace std;

namespace transport_catalogue::json_reader {

namespace detail {

using namespace transport_catalogue::request_handler;

AddStopCmd ParseStopCmd(const json::Dict &request) {
  vector<AddStopCmd::Distance> distances;

  for (const auto &[stop_name, node] : request.at("road_distances"s).AsMap()) {
    distances.emplace_back(stop_name, node.AsInt());
  }

  return {
      request.at("name"s).AsString(),
      {request.at("latitude"s).AsDouble(), request.at("longitude"s).AsDouble()},
      move(distances),
  };
}

AddBusCmd ParseBusCmd(const json::Dict &request) {
  vector<string> stop_names;

  for (const auto &node : request.at("stops"s).AsArray()) {
    stop_names.emplace_back(node.AsString());
  }

  return {
      request.at("name"s).AsString(),
      request.at("is_roundtrip"s).AsBool() ? RouteType::CIRCULAR
                                           : RouteType::LINEAR,
      move(stop_names),
  };
}

vector<BaseRequest> ParseBaseRequests(const json::Array &base_requests) {
  vector<BaseRequest> result;

  for (const auto &node : base_requests) {
    const auto &request = node.AsMap();
    const auto &type = request.at("type"s).AsString();

    if (type == "Stop"s) {
      result.emplace_back(ParseStopCmd(request));
    } else if (type == "Bus"s) {
      result.emplace_back(ParseBusCmd(request));
    } else {
      throw invalid_argument("Unknown base request with type '"s + type + "'"s);
    }
  }

  return result;
}

StopStatRequest ParseStopStatRequest(const json::Dict &request) {
  return {request.at("id"s).AsInt(), request.at("name"s).AsString()};
}

BusStatRequest ParseBusStatRequest(const json::Dict &request) {
  return {request.at("id"s).AsInt(), request.at("name"s).AsString()};
}

RouteRequest ParseRouteRequest(const json::Dict &request) {
  return {
      request.at("id"s).AsInt(),
      request.at("from"s).AsString(),
      request.at("to"s).AsString(),
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
    } else if (type == "Map"s) {
      result.emplace_back(MapRequest{request.at("id"s).AsInt()});
    } else if (type == "Route"s) {
      result.emplace_back(ParseRouteRequest(request));
    } else {
      throw invalid_argument("Unknown stat request with type '"s + type + "'"s);
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
    // clang-format off
    json::Print(json::Document {
      StartCommonJsonDict()
          .Key("error_message"s).Value("not found"s)
      .EndDict().Build()
    }, out);
    // clang-format on
  }

  void operator()(const StopStatResponse &response) {
    auto buses = StartCommonJsonDict().Key("buses"s).StartArray();
    for (const auto &bus_name : response.buses_for_stop) {
      buses.Value(string{bus_name});
    }
    json::Print(json::Document{buses.EndArray().EndDict().Build()}, out);
  }

  void operator()(const BusStatResponse &response) {
    const auto &bus_stats = response.bus_stats;
    // clang-format off
    json::Print(json::Document {
      StartCommonJsonDict()
          .Key("curvature"s)
            .Value(bus_stats.route_length / bus_stats.crow_route_length)
          .Key("route_length"s).Value(bus_stats.route_length)
          .Key("stop_count"s).Value(static_cast<int>(bus_stats.stops_count))
          .Key("unique_stop_count"s)
            .Value(static_cast<int>(bus_stats.unique_stops_count))
      .EndDict().Build()
    }, out);
    // clang-format on
  }

  void operator()(const MapResponse &response) {
    // clang-format off
    json::Print(json::Document {
      StartCommonJsonDict()
          .Key("map"s).Value(response.svg_map)
      .EndDict().Build()
    }, out);
    // clang-format on
  }

  void operator()(const router::RouteResult &route_result) {
    auto items = StartCommonJsonDict().Key("items"s).StartArray();

    for (const auto &action : route_result.steps) {
      items.Value(GetRouteActionJson(action));
    }
    json::Print(json::Document{items.EndArray()
                                   .Key("total_time"s)
                                   .Value(route_result.time / 60)
                                   .EndDict()
                                   .Build()},
                out);
  }

  static json::Dict GetRouteActionJson(const router::RouteAction &step) {
    if (holds_alternative<router::WaitAction>(step)) {
      const auto &wait_step = get<router::WaitAction>(step);
      return
          // clang-format off
          json::Builder{}.StartDict()
            .Key("type"s).Value("Wait"s)
            .Key("stop_name"s).Value(string{wait_step.stop_name})
            .Key("time"s).Value(wait_step.time / 60)
          .EndDict().Build().AsMap();
      // clang-format on
    } else {
      const auto &bus_step = get<router::BusAction>(step);
      return
          // clang-format off
          json::Builder{}.StartDict()
            .Key("type"s).Value("Bus"s)
            .Key("bus"s).Value(string{bus_step.bus_name})
            .Key("span_count"s).Value(static_cast<int>(bus_step.stop_count))
            .Key("time"s).Value(bus_step.time / 60)
          .EndDict().Build().AsMap();
      // clang-format on
    }
  }

  /**
   * Возвращает конструктор JSON с открытым словарём, в который уже
   * добавлены значения, общие для всех ответов.
   */
  json::DictKeyPart StartCommonJsonDict() {
    return json::Builder{}.StartDict().Key("request_id"s).Value(request_id);
  }
};

/**
 * Парсит координату в JSON формате (массив из двух чисел с плавающей точкой)
 */
svg::Point ParsePoint(const json::Array &arr) {
  if (arr.size() == 2) {
    return {arr[0].AsDouble(), arr[1].AsDouble()};
  }
  throw runtime_error(
      "Error parsing JSON array as an SVG point. It must have 2 elements"s);
}

/**
 * Парсит SVG цвет в JSON формате.
 * Строки интерпретируются как название цвета,
 * RGB цвет - это массив с тремя целыми числами
 * RGBA цвет - это массив с тремя целыми и одним числом с плавающей точкой
 */
svg::Color ParseColor(const json::Node &node) {
  if (node.IsString()) {
    return node.AsString();
  }
  if (node.IsArray()) {
    const auto &arr = node.AsArray();
    if (arr.size() == 3) {
      return svg::Rgb{static_cast<unsigned int>(arr[0].AsInt()),
                      static_cast<unsigned int>(arr[1].AsInt()),
                      static_cast<unsigned int>(arr[2].AsInt())};
    } else if (arr.size() == 4) {
      return svg::Rgba{static_cast<unsigned int>(arr[0].AsInt()),
                       static_cast<unsigned int>(arr[1].AsInt()),
                       static_cast<unsigned int>(arr[2].AsInt()),
                       arr[3].AsDouble()};
    }
    throw runtime_error(
        "Error parsing JSON array as a color. It must have 3 or 4 elements"s);
  }
  throw runtime_error(
      "Error parsing JSON node as color. It can be an array or a string"s);
}

/**
 * Парсит настройки отрисовки карты в SVG формате
 */
RenderSettings ParseRenderSettings(const json::Dict &rs) {
  RenderSettings result;
  result.bus_label_font_size = rs.at("bus_label_font_size"s).AsInt();
  result.bus_label_offset = ParsePoint(rs.at("bus_label_offset"s).AsArray());
  for (const auto &node : rs.at("color_palette"s).AsArray()) {
    result.color_palette.emplace_back(ParseColor(node));
  }
  result.height = rs.at("height"s).AsDouble();
  result.line_width = rs.at("line_width"s).AsDouble();
  result.padding = rs.at("padding"s).AsDouble();
  result.stop_label_font_size = rs.at("stop_label_font_size"s).AsInt();
  result.stop_label_offset = ParsePoint(rs.at("stop_label_offset"s).AsArray());
  result.stop_radius = rs.at("stop_radius"s).AsDouble();
  result.underlayer_color = ParseColor(rs.at("underlayer_color"s));
  result.underlayer_width = rs.at("underlayer_width"s).AsDouble();
  result.width = rs.at("width"s).AsDouble();
  return result;
}

RouterSettings ParseRouterSettings(const json::Dict &map) {
  RouterSettings result;
  result.bus_velocity = map.at("bus_velocity"s).AsDouble();
  result.bus_wait_time = map.at("bus_wait_time"s).AsDouble();
  return result;
}

}  // namespace detail

/**
 * Парсит запросы к транспортному справочнику в JSON формате
 * и складывает их в соответствующих векторах.
 */
void BufferingRequestReader::Parse(istream &sin) {
  const auto document = json::Load(sin);
  const auto &root = document.GetRoot().AsMap();
  base_requests_ =
      detail::ParseBaseRequests(root.at("base_requests"s).AsArray());
  stat_requests_ =
      detail::ParseStatRequests(root.at("stat_requests"s).AsArray());
  if (root.count("render_settings"s) > 0) {
    render_settings_ =
        detail::ParseRenderSettings(root.at("render_settings"s).AsMap());
  }
  if (root.count("routing_settings"s) > 0) {
    router_settings_ =
        detail::ParseRouterSettings(root.at("routing_settings"s).AsMap());
  }
}

/**
 * `out` - символьный поток, куда будут выведены ответы на запросы
 */
ResponsePrinter::ResponsePrinter(std::ostream &out) : out_(out) {}

void ResponsePrinter::PrintResponse(int request_id,
                                    const StatResponse &response) {
  if (printed_something_) {
    out_.put(',');
  } else {
    Begin();
  }
  detail::ResponseVariantPrinter printer{request_id, out_};
  std::visit(printer, response);
  printed_something_ = true;
}

ResponsePrinter::~ResponsePrinter() {
  if (printed_something_) {
    End();
  }
}

/**
 * Начинает вывод ответов на запросы к транспортному справочнику в JSON формате.
 */
void ResponsePrinter::Begin() { out_.put('['); }

/**
 * Завершает вывод ответов на запросы к транспортному справочнику в JSON
 * формате.
 */
void ResponsePrinter::End() { out_.put(']'); }

}  // namespace transport_catalogue::json_reader
