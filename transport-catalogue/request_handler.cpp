#include "request_handler.h"

#include <algorithm>
#include <optional>

#include "transport_catalogue.h"

using namespace std;

namespace transport_catalogue::request_handler {

namespace detail {

/**
 * Обработчик всех видов запросов на пополнение базы транспортного справочника. Штука для `std::visit`
 *
 * Запросы на добавление остановок и маршрутов не обрабатываются сразу, а прикапываются в векторы,
 * т.к. запросы на добавление маршрутов нужно обработать строго после обработки всех запросов на
 * добавление остановок, т.к. маршрут может ссылаться на ещё не добавленную остановку.
 *
 * Кроме этого запросы на добавление остановок придётся обрабатывать дважды:
 * один раз для добавления остановок, и второй раз для уточнения реального расстояния между остановками
 * по маршруту.
 *
 * После обработки всех запросов этим процессором через `std::visit`, нужно вызвать методы
 * `FlushStopRequests` и `FlushBusRequests` в этом порядке.
 */
class BaseRequestVariantProcessor {
 public:
  BaseRequestVariantProcessor(TransportCatalogue &transport_catalogue)
      :
      transport_catalogue_(transport_catalogue) {
  }

  void operator()(const AddStopCmd &cmd) {
    add_stop_requests_.push_back(&cmd);
  }

  void operator()(const AddBusCmd &cmd) {
    add_bus_requests_.push_back(&cmd);
  }

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
  vector<const AddStopCmd*> add_stop_requests_;
  vector<const AddBusCmd*> add_bus_requests_;
};

/**
 * Обработчик всех вариантов запросов на получение статистики из транспортного справочника.
 * Штука для `std::visit`.
 */
class StatRequestVariantProcessor {
 public:

  StatRequestVariantProcessor(
      TransportCatalogue &transport_catalogue,
      AbstractStatResponsePrinter &stat_response_printer)
      :
      transport_catalogue_(transport_catalogue),
      stat_response_printer_(stat_response_printer) {
  }

  void operator()(const StopStatRequest &request) {
    auto stop_info = transport_catalogue_.GetStopInfo(request.name);
    if (stop_info.has_value()) {
      stat_response_printer_.PrintResponse(
          request.id, StopStatResponse { move(*stop_info) });
    } else {
      stat_response_printer_.PrintResponse(request.id, { });
    }
  }

  void operator()(const BusStatRequest &request) {
    auto bus_stats = transport_catalogue_.GetBusStats(request.name);
    if (bus_stats.has_value()) {
      stat_response_printer_.PrintResponse(
          request.id, BusStatResponse { move(*bus_stats) });
    } else {
      stat_response_printer_.PrintResponse(request.id, { });
    }
  }
 private:
  TransportCatalogue &transport_catalogue_;
  AbstractStatResponsePrinter &stat_response_printer_;

};

}  // namespace transport_catalogue::request_handler::detail

/**
 * Прочитать все запросы к транспортному справочнику с помощью `request_reader`,
 * отправить эти запросы в `transport_catalogue` и напечатать ответы на запросы
 * статистики с помощью `stat_response_printer`.
 */
void ProcessRequests(TransportCatalogue &transport_catalogue,
                     const AbstractBufferingRequestReader &request_reader,
                     AbstractStatResponsePrinter &stat_response_printer) {

  detail::BaseRequestVariantProcessor base_request_processor {
      transport_catalogue };
  for (const auto &base_request : request_reader.GetBaseRequests()) {
    visit(base_request_processor, base_request);
  }
  base_request_processor.FlushStopRequests();
  base_request_processor.FlushBusRequests();

  detail::StatRequestVariantProcessor stat_request_processor {
      transport_catalogue, stat_response_printer };
  for (const auto &stat_request : request_reader.GetStatRequests()) {
    visit(stat_request_processor, stat_request);
  }
}

}  // namespace transport_catalogue::request_handler
