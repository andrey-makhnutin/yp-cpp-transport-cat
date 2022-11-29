#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "graph.h"
#include "router.h"

namespace transport_catalogue {
struct Stop;
struct Bus;
} /* namespace transport_catalogue */

namespace transport_catalogue {
class TransportCatalogue;
} /* namespace transport_catalogue */

namespace transport_catalogue::router {

struct RouterSettings {
  double bus_velocity = 0;
  double bus_wait_time = 0;
};

struct WaitAction {
  std::string_view stop_name;
  double time;
};

struct BusAction {
  std::string_view bus_name;
  size_t stop_count;
  double time;
};

using RouteAction = std::variant<WaitAction, BusAction>;

struct WaitEdge {
  const Stop *stop;
};

struct BusEdge {
  const Bus *bus;
  size_t span_len;
};

using Edge = std::variant<WaitEdge, BusEdge>;

struct RouteResult {
  double time = 0;
  std::vector<RouteAction> steps;
};

class Router {
 public:
  Router(const RouterSettings &settings,
         const TransportCatalogue &transport_catalogue);
  std::optional<RouteResult> CalcRoute(std::string_view from,
                                       std::string_view to) const;
 private:
  RouterSettings settings_;
  const TransportCatalogue &transport_catalogue_;
  graph::DirectedWeightedGraph<double> stop_graph_;
  std::unique_ptr<graph::Router<double>> router_;

  std::vector<Edge> edges_;
  std::unordered_map<std::string_view, graph::VertexId> vertex_by_stop_name_;

  void BuildStopGraph();
};

}  // namespace transport_catalogue::router
