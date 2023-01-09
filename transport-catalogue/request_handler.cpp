#include "request_handler.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "transport_catalogue.h"

using namespace std;

namespace transport_catalogue::request_handler {

namespace detail {

/**
 * Обработчик всех видов запросов на пополнение базы транспортного справочника.
 * Штука для `std::visit`
 *
 * Запросы на добавление остановок и маршрутов не обрабатываются сразу, а
 * прикапываются в векторы, т.к. запросы на добавление маршрутов нужно
 * обработать строго после обработки всех запросов на добавление остановок, т.к.
 * маршрут может ссылаться на ещё не добавленную остановку.
 *
 * Кроме этого запросы на добавление остановок придётся обрабатывать дважды:
 * один раз для добавления остановок, и второй раз для уточнения реального
 * расстояния между остановками по маршруту.
 *
 * После обработки всех запросов этим процессором через `std::visit`, нужно
 * вызвать методы `FlushStopRequests` и `FlushBusRequests` в этом порядке.
 */
class BaseRequestVariantProcessor {
 public:
  BaseRequestVariantProcessor(TransportCatalogue &transport_catalogue)
      : transport_catalogue_(transport_catalogue) {}

  void operator()(const AddStopCmd &cmd) { add_stop_requests_.push_back(&cmd); }

  void operator()(const AddBusCmd &cmd) { add_bus_requests_.push_back(&cmd); }

  void FlushStopRequests() {
    for (const AddStopCmd *cmd : add_stop_requests_) {
      transport_catalogue_.AddStop(cmd->name, cmd->coordinates);
    }

    for (const AddStopCmd *cmd : add_stop_requests_) {
      for (auto distance_pair : cmd->distances) {
        transport_catalogue_.SetDistance(cmd->name, distance_pair.first,
                                         distance_pair.second);
      }
    }
  }

  void FlushBusRequests() {
    for (const AddBusCmd *cmd : add_bus_requests_) {
      transport_catalogue_.AddBus(cmd->name, cmd->route_type, cmd->stop_names);
    }
  }

 private:
  TransportCatalogue &transport_catalogue_;
  vector<const AddStopCmd *> add_stop_requests_;
  vector<const AddBusCmd *> add_bus_requests_;
};

/**
 * Обработчик всех вариантов запросов на получение статистики из транспортного
 * справочника. Штука для `std::visit`.
 */
class StatRequestVariantProcessor {
 public:
  StatRequestVariantProcessor(
      TransportCatalogue &transport_catalogue,
      AbstractStatResponsePrinter &stat_response_printer,
      const optional<map_renderer::RenderSettings> &render_settings,
      const router::Router *router)
      : transport_catalogue_(transport_catalogue),
        stat_response_printer_(stat_response_printer),
        render_settings_(render_settings),
        router_(router) {}

  void operator()(const StopStatRequest &request) {
    auto stop_info = transport_catalogue_.GetStopInfo(request.name);
    if (stop_info.has_value()) {
      stat_response_printer_.PrintResponse(request.id,
                                           StopStatResponse{move(*stop_info)});
    } else {
      stat_response_printer_.PrintResponse(request.id, {});
    }
  }

  void operator()(const BusStatRequest &request) {
    auto bus_stats = transport_catalogue_.GetBusStats(request.name);
    if (bus_stats.has_value()) {
      stat_response_printer_.PrintResponse(request.id,
                                           BusStatResponse{move(*bus_stats)});
    } else {
      stat_response_printer_.PrintResponse(request.id, {});
    }
  }

  void operator()(const MapRequest &request) {
    if (!render_settings_) {
      stat_response_printer_.PrintResponse(request.id, {});
    } else {
      ostringstream sout;
      map_renderer::SvgMapRenderer map_renderer{transport_catalogue_, sout};
      map_renderer.RenderMap(*render_settings_);
      stat_response_printer_.PrintResponse(request.id, MapResponse{sout.str()});
    }
  }

  void operator()(const RouteRequest &request) {
    if (router_ == nullptr) {
      stat_response_printer_.PrintResponse(request.id, {});
      return;
    }
    auto route =
        router_->CalcRoute(string_view{request.from}, string_view{request.to});
    if (!route) {
      stat_response_printer_.PrintResponse(request.id, {});
      return;
    }
    stat_response_printer_.PrintResponse(request.id, *route);
  }

 private:
  TransportCatalogue &transport_catalogue_;
  AbstractStatResponsePrinter &stat_response_printer_;
  const optional<map_renderer::RenderSettings> &render_settings_;
  const router::Router *router_ = nullptr;
};

}  // namespace detail

/**
 * Прочитать все запросы к транспортному справочнику из `request_reader_`,
 * отправить эти запросы в `transport_catalogue_` и напечатать ответы на запросы
 * статистики с помощью `stat_response_printer`.
 */
void BufferingRequestHandler::ProcessRequests(
    AbstractStatResponsePrinter &stat_response_printer) {
  detail::BaseRequestVariantProcessor base_request_processor{
      transport_catalogue_};
  for (const auto &base_request : request_reader_.GetBaseRequests()) {
    visit(base_request_processor, base_request);
  }
  base_request_processor.FlushStopRequests();
  base_request_processor.FlushBusRequests();
  unique_ptr<router::Router> router = nullptr;
  if (request_reader_.GetRouterSettings()) {
    router = make_unique<router::Router>(*request_reader_.GetRouterSettings(),
                                         transport_catalogue_);
  }

  detail::StatRequestVariantProcessor stat_request_processor{
      transport_catalogue_, stat_response_printer,
      request_reader_.GetRenderSettings(), router.get()};
  for (const auto &stat_request : request_reader_.GetStatRequests()) {
    visit(stat_request_processor, stat_request);
  }
}

/**
 * Отрисовать карту маршрутов справочника с помощью `map_renderer`.
 * Если во входных данных к справочнику не было настроек отрисовки карты,
 * кидается исключение.
 */
void BufferingRequestHandler::RenderMap(MapRenderer &map_renderer) {
  const auto &render_settings = request_reader_.GetRenderSettings();
  if (!render_settings) {
    throw runtime_error(
        "Can't render map: render settings were not specified"s);
  }
  map_renderer.RenderMap(*render_settings);
}

}  // namespace transport_catalogue::request_handler
