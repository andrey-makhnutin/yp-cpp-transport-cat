#pragma once

#include <iostream>
#include <vector>

#include "request_handler.h"

namespace transport_catalogue::json_reader {

using transport_catalogue::request_handler::AbstractBufferingRequestReader;
using transport_catalogue::request_handler::BaseRequest;
using transport_catalogue::request_handler::StatRequest;

using transport_catalogue::request_handler::AbstractStatResponsePrinter;
using transport_catalogue::request_handler::StatResponse;

/**
 * Читает запросы к транспортному справочнику в JSON формате из символьного потока.
 *
 * Все запросы читаются за раз в конструкторе и складываются во временный буфер.
 *
 * Формат данных:
 * ```
 * {
 *   "base_requests": [<запросы на наполнение БД справочника>],
 *   "stat_requests": [<запросы на получение статистики из справочника>]
 * }
 * ```
 *
 * Запросы на наполнение БД справочника бывают такие.
 *
 * Добавление остановки:
 * ```
 * {
 *   "type": "Stop",
 *   "name": "Электросети",
 *   "latitude": 43.598701,
 *   "longitude": 39.730623,
 *   "road_distances": {
 *     "Улица Докучаева": 3000,
 *     "Улица Лизы Чайкиной": 4300
 *   }
 * }
 * ```
 *
 * Добавление маршрута:
 * ```
 * {
 *   "type": "Bus",
 *   "name": "14",
 *   "stops": [
 *     "Улица Лизы Чайкиной",
 *     "Электросети",
 *     "Улица Докучаева",
 *     "Улица Лизы Чайкиной"
 *   ],
 *   "is_roundtrip": true
 * }
 * ```
 *
 * Запросы на получение статистики бывают такими.
 *
 * Получить статистику по остановке:
 * ```
 * {
 *   "id": 12345,
 *   "type": "Stop",
 *   "name": "Улица Докучаева"
 * }
 * ```
 *
 * Получить статистику по маршруту:
 * ```
 * {
 *   "id": 12345678,
 *   "type": "Bus",
 *   "name": "14"
 * }
 * ```
 */
class BufferingRequestReader final : public AbstractBufferingRequestReader {
 public:

  /**
   * Парсит команды из символьного потока `in`.
   */
  BufferingRequestReader(std::istream &in) {
    Parse(in);
  }

  virtual const std::vector<BaseRequest>& GetBaseRequests() const override {
    return base_requests_;
  }
  virtual const std::vector<StatRequest>& GetStatRequests() const override {
    return stat_requests_;
  }
 private:
  std::vector<BaseRequest> base_requests_;
  std::vector<StatRequest> stat_requests_;

  void Parse(std::istream&);
};

/**
 * Печатает ответы на запросы к транспортному справочнику в JSON формате в символьный поток.
 *
 * Ответы печатаются в виде JSON-массива в потоковом режиме:
 * 1. сначала печатается открывающая квадратная скобка;
 * 2. ответ печатается во время вызова `PrintResponse`;
 * 3. закрывающая квадратная скобка печатается в деструкторе.
 *
 * Формат вывода.
 * ```
 * [<ответ>, ...]
 * ```
 *
 * JSON печатается без отступов и пробелов между элементами.
 * Здесь приведены примеры с отступами для удобства.
 *
 * Статистика по остановке:
 * ```
 * {
 *   "buses": ["14", "22к"],
 *   "request_id": 12345
 * }
 * ```
 *
 * Статистика по маршруту:
 * ```
 * {
 *   "curvature": 2.18604,
 *   "request_id": 12345678,
 *   "route_length": 9300,
 *   "stop_count": 4,
 *   "unique_stop_count": 3
 * }
 * ```
 *
 * Если был сделан запрос на статистику по несуществующему объекту,
 * печатается сообщение об ошибке:
 * ```
 * {
 *   "request_id": 12345,
 *   "error_message": "not found"
 * }
 * ```
 */
class ResponsePrinter final : public AbstractStatResponsePrinter {
 public:
  ResponsePrinter(std::ostream&);
  virtual void PrintResponse(int request_id, const StatResponse&) override;
  ~ResponsePrinter();
 private:
  std::ostream &out_;

  // выставляется в `true` после первой печати
  bool printed_something_ = false;

  void Begin();
  void End();
};

}
