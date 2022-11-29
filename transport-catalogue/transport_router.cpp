#include "transport_router.h"

#include <cassert>
#include <string>

#include "domain.h"
#include "transport_catalogue.h"

using namespace std;

namespace transport_catalogue::router {

Router::Router(const RouterSettings &settings,
               const TransportCatalogue &transport_catalogue)
    :
    settings_(settings),
    transport_catalogue_(transport_catalogue) {
  BuildStopGraph();
}

void Router::BuildStopGraph() {
  unordered_map<const Stop*, graph::VertexId> id_by_stop;
  auto all_stops = transport_catalogue_.GetStops();
  {
    graph::VertexId id = 0;
    for (const Stop *stop : all_stops) {
      id_by_stop[stop] = id;
      vertex_by_stop_name_[string_view(stop->name)] = id;
      id += 2;
    }
  }

  double v = settings_.bus_velocity * 1000 / 3600;
  double w = settings_.bus_wait_time * 60;

  stop_graph_ = graph::DirectedWeightedGraph<double>(all_stops.size() * 2);

  auto buses = transport_catalogue_.GetBuses();
  auto add_bus_edge = [this, &id_by_stop, v](const Bus *bus, const Stop *from,
                                             const Stop *to, size_t span_len,
                                             double route_len) {
    auto v_from = id_by_stop.at(from) + 1;
    auto v_to = id_by_stop.at(to);
    auto time = route_len / v;
    auto edge_id = stop_graph_.AddEdge( { v_from, v_to, time });
    assert(edge_id == edges_.size());
    edges_.push_back(BusEdge { bus, span_len });
  };
  auto calc_part_lengths = [this](const vector<const Stop*> &stops,
                                  bool inverse) {
    vector<double> part_lengths(stops.size() - 1);
    for (size_t i = 0; i < stops.size() - 1; ++i) {
      part_lengths[i] =
          inverse ?
              transport_catalogue_.GetRealDistance(stops[i + 1], stops[i]) :
              transport_catalogue_.GetRealDistance(stops[i], stops[i + 1]);
    }
    return part_lengths;
  };
  auto calc_route_length = [](const vector<double> &part_lens, size_t from,
                              size_t to) {
    double length = 0;
    for (size_t i = from; i != to; ++i) {
      length += part_lens[i];
    }
    return length;
  };
  for (const Bus *bus : buses) {
    const auto &stops = bus->stops;
    if (stops.size() < 2) {
      continue;
    }
    vector<double> part_lens = calc_part_lengths(stops, false);
    vector<double> inv_part_lens;

    if (bus->route_type == RouteType::LINEAR) {
      inv_part_lens = calc_part_lengths(stops, true);
    }
    for (size_t i = 0; i < stops.size() - 1; ++i) {
      for (size_t j = i + 1; j < stops.size(); ++j) {
        add_bus_edge(bus, stops[i], stops[j], j - i,
                     calc_route_length(part_lens, i, j));
        if (bus->route_type == RouteType::LINEAR) {
          add_bus_edge(bus, stops[j], stops[i], j - i,
                       calc_route_length(inv_part_lens, i, j));
        }
      }
    }
    if (bus->route_type == RouteType::CIRCULAR) {
      double depo_len = transport_catalogue_.GetRealDistance(stops.back(),
                                                             stops[0]);
      for (size_t i = 1; i < stops.size(); ++i) {
        add_bus_edge(
            bus, stops[i], stops[0], stops.size() - i,
            calc_route_length(part_lens, i, stops.size() - 1) + depo_len);
      }
    }
  }
  for (const Stop *stop : all_stops) {
    auto v_wait = id_by_stop.at(stop);
    auto v_bus = v_wait + 1;
    auto edge_id = stop_graph_.AddEdge( { v_wait, v_bus, w });
    assert(edge_id == edges_.size());
    edges_.push_back(WaitEdge { stop });
  }

  router_ = make_unique<graph::Router<double>>(stop_graph_);
}

optional<RouteResult> Router::CalcRoute(string_view from,
                                        string_view to) const {
  if (vertex_by_stop_name_.count(from) == 0
      || vertex_by_stop_name_.count(to) == 0) {
    return nullopt;
  }
  graph::VertexId v_from = vertex_by_stop_name_.at(from);
  graph::VertexId v_to = vertex_by_stop_name_.at(to);
  auto route_opt = router_->BuildRoute(v_from, v_to);
  if (!route_opt) {
    return nullopt;
  }
  const auto &route = *route_opt;
  RouteResult result;
  for (auto edge_id : route.edges) {
    const auto &edge = edges_[edge_id];
    const auto &graph_edge = stop_graph_.GetEdge(edge_id);
    if (holds_alternative<WaitEdge>(edge)) {
      const auto &wait_edge = get<WaitEdge>(edge);
      result.steps.push_back(WaitAction { string_view { wait_edge.stop->name },
          graph_edge.weight });
    } else {
      const auto &bus_edge = get<BusEdge>(edge);
      result.steps.push_back(BusAction { string_view { bus_edge.bus->name },
          bus_edge.span_len, graph_edge.weight });
    }
    result.time += graph_edge.weight;
  }

  return result;
}

}  // namespace transport_catalogue::router
