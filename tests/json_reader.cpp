#include "../transport-catalogue/json_reader.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <variant>

#include "../transport-catalogue/domain.h"
#include "../transport-catalogue/geo.h"
#include "../transport-catalogue/svg.h"
#include "json_reader.h"
#include "test_framework.h"

using namespace std;
using namespace transport_catalogue::request_handler;

namespace transport_catalogue::json_reader::tests {

void TestStopParser() {
  string stop_json =
      R"({
           "type": "Stop",
           "name": "Электросети",
           "latitude": 43.598701,
           "longitude": 39.730623,
           "road_distances": {
             "Улица Докучаева": 3000,
             "Улица Лизы Чайкиной": 4300
           }
         })"s;
  istringstream sin{R"({"base_requests":[)"s + stop_json +
                    R"(],"stat_requests":[]})"s};
  BufferingRequestReader reader{sin};
  const auto &base_requests = reader.GetBaseRequests();
  ASSERT_EQUAL(base_requests.size(), 1u);
  ASSERT(holds_alternative<AddStopCmd>(base_requests[0]));
  const auto &cmd = get<AddStopCmd>(base_requests[0]);
  ASSERT_EQUAL(cmd.name, "Электросети"s);
  ASSERT_SOFT_EQUAL(cmd.coordinates.lat, 43.598701);
  ASSERT_SOFT_EQUAL(cmd.coordinates.lng, 39.730623);

  // порядок элементов в `cmd.distances` не определён,
  // поэтому сортируем расстояния до остановок для стабильности теста
  vector<AddStopCmd::Distance> distances{cmd.distances.begin(),
                                         cmd.distances.end()};
  sort(distances.begin(), distances.end());
  ASSERT_EQUAL(distances.size(), 2u);
  ASSERT_EQUAL(distances[0].first, "Улица Докучаева"s);
  ASSERT_EQUAL(distances[0].second, 3000u);
  ASSERT_EQUAL(distances[1].first, "Улица Лизы Чайкиной"s);
  ASSERT_EQUAL(distances[1].second, 4300u);
}

void TestBusParser() {
  {
    string stop_json =
        R"({
           "type": "Bus",
           "name": "14",
           "stops": [
             "Улица Лизы Чайкиной",
             "Электросети",
             "Улица Докучаева",
             "Улица Лизы Чайкиной"
           ],
           "is_roundtrip": true
         })"s;
    istringstream sin{R"({"base_requests":[)"s + stop_json +
                      R"(],"stat_requests":[]})"s};
    BufferingRequestReader reader{sin};
    const auto &base_requests = reader.GetBaseRequests();
    ASSERT_EQUAL(base_requests.size(), 1u);
    ASSERT(holds_alternative<AddBusCmd>(base_requests[0]));
    const auto &cmd = get<AddBusCmd>(base_requests[0]);
    ASSERT_EQUAL(cmd.name, "14"s);
    ASSERT_EQUAL(cmd.route_type, RouteType::CIRCULAR);
    ASSERT_EQUAL(cmd.stop_names,
                 (vector<string>{"Улица Лизы Чайкиной"s, "Электросети"s,
                                 "Улица Докучаева"s, "Улица Лизы Чайкиной"s}));
  }
  {
    string stop_json =
        R"({
           "type": "Bus",
           "name": "15",
           "stops": [
             "Улица Лизы Чайкиной",
             "Электросети",
             "Улица Докучаева"
           ],
           "is_roundtrip": false
         })"s;
    istringstream sin{R"({"base_requests":[)"s + stop_json +
                      R"(],"stat_requests":[]})"s};
    BufferingRequestReader reader{sin};
    const auto &base_requests = reader.GetBaseRequests();
    ASSERT_EQUAL(base_requests.size(), 1u);
    ASSERT_EQUAL(get<AddBusCmd>(base_requests[0]).route_type,
                 RouteType::LINEAR);
  }
}

void TestStopStatRequestParser() {
  string stop_stat_request_json =
      R"({
         "id": 12345,
         "type": "Stop",
         "name": "Улица Докучаева"
       })"s;
  istringstream sin{R"({"base_requests":[],"stat_requests":[)"s +
                    stop_stat_request_json + R"(]})"s};
  BufferingRequestReader reader{sin};
  const auto &stat_requests = reader.GetStatRequests();
  ASSERT_EQUAL(stat_requests.size(), 1u);
  ASSERT(holds_alternative<StopStatRequest>(stat_requests[0]));
  const auto &req = get<StopStatRequest>(stat_requests[0]);
  ASSERT_EQUAL(req.id, 12345);
  ASSERT_EQUAL(req.name, "Улица Докучаева"s);
}

void TestBusStatRequestParser() {
  string bus_stat_request_json =
      R"({
         "id": 12345678,
         "type": "Bus",
         "name": "14"
       })"s;
  istringstream sin{R"({"base_requests":[],"stat_requests":[)"s +
                    bus_stat_request_json + R"(]})"s};
  BufferingRequestReader reader{sin};
  const auto &stat_requests = reader.GetStatRequests();
  ASSERT_EQUAL(stat_requests.size(), 1u);
  ASSERT(holds_alternative<BusStatRequest>(stat_requests[0]));
  const auto &req = get<BusStatRequest>(stat_requests[0]);
  ASSERT_EQUAL(req.id, 12345678);
  ASSERT_EQUAL(req.name, "14"s);
}

void TestRenderSettings() {
  {
    string render_settings_json =
        R"({
           "width": 1200.0,
           "height": 1200.1,

           "padding": 50.0,

           "line_width": 14.0,
           "stop_radius": 5.0,

           "bus_label_font_size": 20,
           "bus_label_offset": [7.0, 15.0],

           "stop_label_font_size": 21,
           "stop_label_offset": [7.1, -3.1],

           "underlayer_color": [255, 255, 255, 0.85],
           "underlayer_width": 3.0,

           "color_palette": [
             "green",
             [255, 160, 0],
             "red"
           ]
         })"s;
    istringstream sin{
        R"({"base_requests":[],"stat_requests":[],"render_settings":)"s +
        render_settings_json + R"(})"s};
    BufferingRequestReader reader{sin};
    ASSERT(reader.GetRenderSettings());
    const auto &rs = *reader.GetRenderSettings();
    ASSERT_SOFT_EQUAL(rs.width, 1200.0);
    ASSERT_SOFT_EQUAL(rs.height, 1200.1);
    ASSERT_SOFT_EQUAL(rs.padding, 50.0);
    ASSERT_SOFT_EQUAL(rs.line_width, 14.0);
    ASSERT_SOFT_EQUAL(rs.stop_radius, 5.0);
    ASSERT_EQUAL(rs.bus_label_font_size, 20u);
    ASSERT_EQUAL(rs.bus_label_offset, (svg::Point{7.0, 15.0}));
    ASSERT_EQUAL(rs.stop_label_font_size, 21u);
    ASSERT_EQUAL(rs.stop_label_offset, (svg::Point{7.1, -3.1}));
    ASSERT_EQUAL(rs.underlayer_color,
                 (svg::Color{svg::Rgba{255, 255, 255, 0.85}}));
    ASSERT_SOFT_EQUAL(rs.underlayer_width, 3.0);
    ASSERT_EQUAL(rs.color_palette,
                 (vector<svg::Color>{"green"s, svg::Rgb{255, 160, 0}, "red"s}));
  }
  {
    istringstream sin{R"({"base_requests":[],"stat_requests":[]})"s};
    BufferingRequestReader reader{sin};
    ASSERT(!reader.GetRenderSettings().has_value());
  }
}

void TestBusStatResponsePrinter() {
  ostringstream sout;

  {
    ResponsePrinter printer{sout};
    printer.PrintResponse(
        12345678, BusStatResponse{BusStats{4, 3, 9300.0, 4254.267991437}});
  }
  ASSERT_EQUAL(
      sout.str(),
      R"([{"curvature":2.18604,"request_id":12345678,"route_length":9300,"stop_count":4,"unique_stop_count":3}])"s);
}

void TestStopStatResponsePrinter() {
  ostringstream sout;

  {
    ResponsePrinter printer{sout};
    printer.PrintResponse(12345,
                          StopStatResponse{BusesForStop{"14"sv, "22к"sv}});
  }
  ASSERT_EQUAL(sout.str(), R"([{"buses":["14","22к"],"request_id":12345}])"s);
}

void TestEmptyResponsePrinter() {
  ostringstream sout;

  {
    ResponsePrinter printer{sout};
    printer.PrintResponse(12345, {});
    printer.PrintResponse(12346, {});
  }

  ASSERT_EQUAL(
      sout.str(),
      R"([{"error_message":"not found","request_id":12345},{"error_message":"not found","request_id":12346}])"s);
}

}  // namespace transport_catalogue::json_reader::tests

void TestJSONReader(TestRunner &tr) {
  using namespace transport_catalogue::json_reader::tests;

  RUN_TEST(tr, TestStopParser);
  RUN_TEST(tr, TestBusParser);
  RUN_TEST(tr, TestStopStatRequestParser);
  RUN_TEST(tr, TestBusStatRequestParser);
  RUN_TEST(tr, TestRenderSettings);

  RUN_TEST(tr, TestBusStatResponsePrinter);
  RUN_TEST(tr, TestStopStatResponsePrinter);
  RUN_TEST(tr, TestEmptyResponsePrinter);
}
