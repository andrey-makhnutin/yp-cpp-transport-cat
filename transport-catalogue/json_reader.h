#pragma once

#include <iostream>
#include <optional>
#include <vector>

#include "map_renderer.h"
#include "request_handler.h"

namespace transport_catalogue::json_reader {

using transport_catalogue::request_handler::AbstractBufferingRequestReader;
using transport_catalogue::request_handler::BaseRequest;
using transport_catalogue::request_handler::StatRequest;
using transport_catalogue::map_renderer::RenderSettings;

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
 *   "stat_requests": [<запросы на получение статистики из справочника>],
 *   "render_settings": { <настройки отрисовки карты в SVG формате> }, // может отсутствовать
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
 * Установить настройки отрисовки карты в SVG формате:
 * ```
 * {
 *   "width": 1200.0,       // ширина и высота изображения в пикселях.
 *   "height": 1200.1,
 *   "padding": 50.0,       // отступ краёв карты от границ SVG-документа.
 *   "line_width": 14.0,    // толщина линий, которыми рисуются автобусные маршруты.
 *   "stop_radius": 5.0,    // радиус окружностей, которыми обозначаются остановки.
 *   "bus_label_font_size": 20,       // размер текста, которым написаны названия автобусных маршрутов.
 *   "bus_label_offset": [7.0, 15.0], // смещение надписи с названием маршрута относительно координат конечной остановки на карте.
 *                                    // Задаёт значения свойств dx и dy SVG-элемента <text>.
 *   "stop_label_font_size": 21,      // размер текста, которым отображаются названия остановок.
 *   "stop_label_offset": [7.1, -3.1],// смещение названия остановки относительно её координат на карте. Массив из двух элементов типа double.
 *                                    // Задаёт значения свойств dx и dy SVG-элемента <text>.
 *   "underlayer_color": [255, 255, 255, 0.85], // цвет подложки под названиями остановок и маршрутов.
 *   "underlayer_width": 3.0,    // толщина подложки под названиями остановок и маршрутов.
 *   "color_palette": [          // цветовая палитра.
 *     "green",
 *     [255, 160, 0],
 *     "red"
 *   ]
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
 *
 * Получить карту маршрутов в SVG формате:
 * ```
 * {
 *   "id": 12346,
 *   "type": "Map"
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
  /**
   * Возвращает настройки отрисовки карты в SVG формате. Может отсутствовать
   */
  virtual const std::optional<RenderSettings>& GetRenderSettings() const
      override {
    return render_settings_;
  }
 private:
  std::vector<BaseRequest> base_requests_;
  std::vector<StatRequest> stat_requests_;
  std::optional<RenderSettings> render_settings_;

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
