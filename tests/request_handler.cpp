#include "../transport-catalogue/request_handler.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string_view>

#include "../transport-catalogue/transport_catalogue.h"
#include "request_handler.h"
#include "test_framework.h"

using namespace std;

namespace transport_catalogue::request_handler::tests {

struct TestRequestReader : public AbstractBufferingRequestReader {
  vector<BaseRequest> base_requests;
  vector<StatRequest> stat_requests;
  optional<RenderSettings> render_settings;
  optional<RouterSettings> router_settings;

  TestRequestReader(vector<BaseRequest> &&_base_requests,
                    vector<StatRequest> &&_stat_requests,
                    optional<RenderSettings> &&_render_settings = nullopt,
                    optional<RouterSettings> &&_router_settings = nullopt)
      : base_requests(move(_base_requests)),
        stat_requests(move(_stat_requests)),
        render_settings(move(_render_settings)),
        router_settings(move(_router_settings)) {}

  virtual const vector<BaseRequest> &GetBaseRequests() const override {
    return base_requests;
  }
  virtual const vector<StatRequest> &GetStatRequests() const override {
    return stat_requests;
  }
  virtual const optional<RenderSettings> &GetRenderSettings() const override {
    return render_settings;
  }
  virtual const optional<RouterSettings> &GetRouterSettings() const override {
    return router_settings;
  }
};

struct TestResponsePrinter : public AbstractStatResponsePrinter {
  vector<pair<int, StatResponse>> collected_responses;

  virtual void PrintResponse(int request_id,
                             const StatResponse &response) override {
    collected_responses.emplace_back(request_id, response);
  }
};

map_renderer::RenderSettings GetTestRenderSettings() {
  map_renderer::RenderSettings rs;
  rs.width = 600;
  rs.height = 400;
  rs.padding = 50;
  rs.stop_radius = 5;
  rs.line_width = 14;
  rs.bus_label_font_size = 20;
  rs.bus_label_offset = svg::Point{7, 15};
  rs.stop_label_font_size = 20;
  rs.stop_label_offset = svg::Point{7, -3};
  rs.underlayer_color = svg::Rgba{255, 255, 255, 0.85};
  rs.underlayer_width = 3;
  rs.color_palette =
      vector<svg::Color>{"green"s, svg::Rgb{255, 160, 0}, "red"s};
  return rs;
}

void TestProcessRequests() {
  const TestRequestReader requests{
      {AddBusCmd{
           "114"s, RouteType::LINEAR, {"Морской вокзал"s, "Ривьерский мост"s}},
       AddStopCmd{"Ривьерский мост"s,
                  {43.587795, 39.716901},
                  {{"Морской вокзал"s, 850}}},
       AddStopCmd{"Морской вокзал"s,
                  {43.581969, 39.719848},
                  {{"Ривьерский мост"s, 850}}}},
      {StopStatRequest{1, "Ривьерский мост"s}, BusStatRequest{2, "114"s},
       MapRequest{3}},
      GetTestRenderSettings()};
  TestResponsePrinter response_collector;
  TransportCatalogue transport_catalogue;
  BufferingRequestHandler request_handler{transport_catalogue, requests};

  request_handler.ProcessRequests(response_collector);
  const auto &responses = response_collector.collected_responses;
  ASSERT_EQUAL(responses.size(), 3u);
  {
    ASSERT_EQUAL(responses[0].first, 1);
    ASSERT(holds_alternative<StopStatResponse>(responses[0].second));
    const auto &response = get<StopStatResponse>(responses[0].second);
    ASSERT_EQUAL(response.buses_for_stop, (set<string_view>{"114"sv}));
  }
  {
    ASSERT_EQUAL(responses[1].first, 2);
    ASSERT(holds_alternative<BusStatResponse>(responses[1].second));
    const auto &response = get<BusStatResponse>(responses[1].second);
    ASSERT_SOFT_EQUAL(response.bus_stats.crow_route_length, 1379.877);
    ASSERT_SOFT_EQUAL(response.bus_stats.route_length, 1700.0);
    ASSERT_EQUAL(response.bus_stats.stops_count, 3u);
    ASSERT_EQUAL(response.bus_stats.unique_stops_count, 2u);
  }
  {
    ASSERT_EQUAL(responses[2].first, 3);
    ASSERT(holds_alternative<MapResponse>(responses[2].second));
    const auto &response = get<MapResponse>(responses[2].second);
    ASSERT_EQUAL(response.svg_map,
                 R"eox(<?xml version="1.0" encoding="UTF-8" ?>
<svg xmlns="http://www.w3.org/2000/svg" version="1.1">
  <polyline points="201.751,350 50,50 201.751,350" fill="none" stroke="green" stroke-width="14" stroke-linecap="round" stroke-linejoin="round" />
  <text x="201.751" y="350" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">114</text>
  <text x="201.751" y="350" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="green">114</text>
  <text x="50" y="50" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">114</text>
  <text x="50" y="50" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="green">114</text>
  <circle cx="201.751" cy="350" r="5" fill="white" />
  <circle cx="50" cy="50" r="5" fill="white" />
  <text x="201.751" y="350" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">Морской вокзал</text>
  <text x="201.751" y="350" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="black">Морской вокзал</text>
  <text x="50" y="50" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">Ривьерский мост</text>
  <text x="50" y="50" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="black">Ривьерский мост</text>
</svg>
)eox"s);
  }
}

using map_renderer::MapRenderer;
using map_renderer::RenderSettings;
struct TestMapRenderer final : public MapRenderer {
  vector<const RenderSettings *> render_calls;

  virtual void RenderMap(const RenderSettings &render_settings) override {
    render_calls.push_back(&render_settings);
  }
};

void TestRenderMap() {
  {
    const TestRequestReader requests{{}, {}, nullopt};
    TransportCatalogue transport_catalogue;
    BufferingRequestHandler request_handler{transport_catalogue, requests};
    TestMapRenderer map_renderer;
    ASSERT_THROWS(request_handler.RenderMap(map_renderer), runtime_error);
    ASSERT_EQUAL(map_renderer.render_calls.size(), 0u);
  }
  {
    RenderSettings render_settings;
    const TestRequestReader requests{{}, {}, render_settings};
    TransportCatalogue transport_catalogue;
    BufferingRequestHandler request_handler{transport_catalogue, requests};
    TestMapRenderer map_renderer;
    request_handler.RenderMap(map_renderer);
    ASSERT_EQUAL(map_renderer.render_calls.size(), 1u);
    ASSERT_EQUAL(map_renderer.render_calls[0],
                 &(*requests.GetRenderSettings()));
  }
}

}  // namespace transport_catalogue::request_handler::tests

void TestRequestHandler(TestRunner &tr) {
  using namespace transport_catalogue::request_handler::tests;

  RUN_TEST(tr, TestProcessRequests);
  RUN_TEST(tr, TestRenderMap);
}
