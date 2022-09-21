#include "../transport-catalogue/request_handler.h"
#include "request_handler.h"

#include <algorithm>

#include "../transport-catalogue/transport_catalogue.h"
#include "test_framework.h"

using namespace std;

namespace transport_catalogue::request_handler::tests {

struct TestRequestReader : public AbstractBufferingRequestReader {
  vector<BaseRequest> base_requests;
  vector<StatRequest> stat_requests;

  TestRequestReader(vector<BaseRequest> &&_base_requests,
                    vector<StatRequest> &&_stat_requests)
      :
      base_requests(move(_base_requests)),
      stat_requests(move(_stat_requests)) {
  }

  virtual const vector<BaseRequest>& GetBaseRequests() const override {
    return base_requests;
  }
  virtual const vector<StatRequest>& GetStatRequests() const override {
    return stat_requests;
  }
};

struct TestResponsePrinter : public AbstractStatResponsePrinter {
  vector<pair<int, StatResponse>> collected_responses;

  virtual void PrintResponse(int request_id, const StatResponse &response)
      override {
    collected_responses.emplace_back(request_id, response);
  }
};

void TestProcessRequests() {
  const TestRequestReader requests { { AddBusCmd { "114"s, RouteType::LINEAR, {
      "Морской вокзал"s, "Ривьерский мост"s } }, AddStopCmd {
      "Ривьерский мост"s, { 43.587795, 39.716901 },
      { { "Морской вокзал"s, 850 } } }, AddStopCmd { "Морской вокзал"s, {
      43.581969, 39.719848 }, { { "Ривьерский мост"s, 850 } } } }, {
      StopStatRequest { 1, "Ривьерский мост"s }, BusStatRequest { 2, "114"s }, } };
  TestResponsePrinter response_collector;
  TransportCatalogue transport_catalogue;

  ProcessRequests(transport_catalogue, requests, response_collector);
  const auto &responses = response_collector.collected_responses;
  ASSERT_EQUAL(responses.size(), 2u);
  {
    ASSERT_EQUAL(responses[0].first, 1);
    ASSERT(holds_alternative<StopStatResponse>(responses[0].second));
    const auto &response = get<StopStatResponse>(responses[0].second);
    ASSERT_EQUAL(response.buses_for_stop, (set<string_view> { "114"sv }));
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
}

}  // namespace transport_catalogue::request_handler::tests

void TestRequestHandler(TestRunner &tr) {
  using namespace transport_catalogue::request_handler::tests;

  RUN_TEST(tr, TestProcessRequests);
}

