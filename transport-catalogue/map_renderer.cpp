#include "map_renderer.h"

#include <algorithm>
#include <cassert>
#include <cmath>  // IWYU pragma: keep
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "domain.h"
#include "geo.h"
#include "transport_catalogue.h"

using namespace std;

namespace transport_catalogue::map_renderer {

namespace detail {

const string BUS_LABEL_FONT_FAMILY = "Verdana"s;
const string BUS_LABEL_FONT_WEIGHT = "bold"s;
const string STOP_CIRCLE_FILL_COLOR = "white"s;
const string STOP_LABEL_FONT_FAMILY = BUS_LABEL_FONT_FAMILY;
const string STOP_LABEL_FILL_COLOR = "black"s;

inline const double EPSILON = 1e-6;
bool IsZero(double value) { return abs(value) < EPSILON; }

class SphereProjector {
 public:
  // points_begin и points_end задают начало и конец интервала элементов
  // geo::Coordinates
  template <typename PointInputIt>
  SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                  double max_width, double max_height, double padding)
      : padding_(padding)  //
  {
    // Если точки поверхности сферы не заданы, вычислять нечего
    if (points_begin == points_end) {
      return;
    }

    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] =
        minmax_element(points_begin, points_end,
                       [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] =
        minmax_element(points_begin, points_end,
                       [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    // Вычисляем коэффициент масштабирования вдоль координаты x
    optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
      width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    // Вычисляем коэффициент масштабирования вдоль координаты y
    optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
      height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
      // Коэффициенты масштабирования по ширине и высоте ненулевые,
      // берём минимальный из них
      zoom_coeff_ = min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
      // Коэффициент масштабирования по ширине ненулевой, используем его
      zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
      // Коэффициент масштабирования по высоте ненулевой, используем его
      zoom_coeff_ = *height_zoom;
    }
  }

  // Проецирует широту и долготу в координаты внутри SVG-изображения
  svg::Point operator()(geo::Coordinates coords) const {
    return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
  }

 private:
  double padding_;
  double min_lon_ = 0;
  double max_lat_ = 0;
  double zoom_coeff_ = 0;
};

struct SvgRenderContext {
  const SphereProjector sphere_projector;
  svg::Document document;
  const RenderSettings &render_settings;
};

SphereProjector MakeSphereProjector(const vector<const Bus *> &buses,
                                    const RenderSettings &rs) {
  vector<geo::Coordinates> all_coords;
  for (const Bus *bus : buses) {
    for (const Stop *stop : bus->stops) {
      all_coords.push_back(stop->coords);
    }
  }
  return SphereProjector{all_coords.begin(), all_coords.end(), rs.width,
                         rs.height, rs.padding};
}

/**
 * Отрисовать линию маршрута
 */
void RenderRoute(SvgRenderContext &ctx, const Bus *bus,
                 svg::Color route_color) {
  vector<geo::Coordinates> coords;
  const auto &rs = ctx.render_settings;
  coords.reserve(bus->route_type == RouteType::LINEAR ? bus->stops.size() * 2
                                                      : bus->stops.size() + 1);
  for (const Stop *stop : bus->stops) {
    coords.push_back(stop->coords);
  }
  switch (bus->route_type) {
    case RouteType::LINEAR: {
      auto reverse_begin = make_reverse_iterator(bus->stops.end());
      auto reverse_end = make_reverse_iterator(bus->stops.begin());
      for (auto it = next(reverse_begin); it != reverse_end; ++it) {
        coords.push_back((*it)->coords);
      }
      break;
    }
    case RouteType::CIRCULAR:
      coords.push_back(bus->stops[0]->coords);
      break;
    default:
      throw runtime_error("Unsupported route type"s);
  }

  vector<svg::Point> points;
  auto route_line = make_unique<svg::Polyline>();
  for (auto coord : coords) {
    route_line->AddPoint(ctx.sphere_projector(coord));
  }
  route_line->SetFillColor({})
      .SetStrokeColor(route_color)
      .SetStrokeWidth(rs.line_width)
      .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
      .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
  ctx.document.AddPtr(move(route_line));
}

/**
 * Отрисовать название маршрута по координате `stop_coords`
 */
void RenderBusName(SvgRenderContext &ctx, const Bus *bus,
                   svg::Color route_color, geo::Coordinates stop_coords) {
  const auto &rs = ctx.render_settings;
  auto name_undertitle = make_unique<svg::Text>();
  name_undertitle->SetData(bus->name)
      .SetOffset(rs.bus_label_offset)
      .SetFontSize(rs.bus_label_font_size)
      .SetFontFamily(BUS_LABEL_FONT_FAMILY)
      .SetFontWeight(BUS_LABEL_FONT_WEIGHT)
      .SetPosition(ctx.sphere_projector(stop_coords));

  auto name_label = make_unique<svg::Text>(*name_undertitle);

  name_undertitle->SetFillColor(rs.underlayer_color)
      .SetStrokeColor(rs.underlayer_color)
      .SetStrokeWidth(rs.underlayer_width)
      .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
      .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

  name_label->SetFillColor(route_color);

  ctx.document.AddPtr(move(name_undertitle));
  ctx.document.AddPtr(move(name_label));
}

/**
 * Отрисовать названия маршрутов возле первой и последнй остановками маршрута.
 * Если маршрут кольцевой, или линейный, но первая и последняя остановки
 * совпадают, название отрисовывается только один раз.
 */
void RenderBusLabels(SvgRenderContext &ctx, const Bus *bus,
                     svg::Color route_color) {
  assert(!bus->stops.empty());
  switch (bus->route_type) {
    case RouteType::LINEAR: {
      RenderBusName(ctx, bus, route_color, bus->stops[0]->coords);
      const auto last_stop = bus->stops.back();
      if (last_stop->name != bus->stops[0]->name) {
        RenderBusName(ctx, bus, route_color, bus->stops.back()->coords);
      }
      break;
    }
    case RouteType::CIRCULAR:
      RenderBusName(ctx, bus, route_color, bus->stops[0]->coords);
      break;
    default:
      throw runtime_error("Unsupported route type"s);
  }
}

/**
 * Выполнить функцию `fn` для всех маршрутов из вектора `buses`.
 * В функцию передаются параметры `ctx`, `bus` (очередной элемент вектора)
 * и `svg::Color` из цветовой палитры. Цвет берётся из палитры `color_palette`,
 * и перебирается в цикле.
 */
template <class BusRenderer>
void DoForEachBus(SvgRenderContext &ctx, const vector<const Bus *> &buses,
                  BusRenderer fn) {
  size_t color_index = 0;
  for (const Bus *bus : buses) {
    if (bus->stops.empty()) {
      continue;
    }
    const auto &color = ctx.render_settings.color_palette[color_index];
    fn(ctx, bus, color);
    color_index = (color_index + 1) % ctx.render_settings.color_palette.size();
  }
}

/**
 * Отрисовывает маршруты на SVG карте.
 */
void RenderBuses(SvgRenderContext &ctx, const vector<const Bus *> &buses) {
  DoForEachBus(ctx, buses, RenderRoute);
  DoForEachBus(ctx, buses, RenderBusLabels);
}

/**
 * Собирает уникальные остановки, входящие в маршруты `buses`, сортирует их по
 * названию.
 */
vector<const Stop *> CollectStops(const vector<const Bus *> &buses) {
  vector<const Stop *> result;
  for (const Bus *bus : buses) {
    for (const Stop *stop : bus->stops) {
      result.push_back(stop);
    }
  }
  sort(result.begin(), result.end(),
       [](const Stop *a, const Stop *b) { return a->name < b->name; });
  auto last = unique(result.begin(), result.end());
  result.erase(last, result.end());
  return result;
}

/**
 * Отрисовывает кружок в том месте, где находится остановка.
 */
void RenderStopCircle(SvgRenderContext &ctx, const Stop *stop) {
  const auto &rs = ctx.render_settings;
  auto circle = make_unique<svg::Circle>();
  circle->SetCenter(ctx.sphere_projector(stop->coords))
      .SetRadius(rs.stop_radius)
      .SetFillColor(STOP_CIRCLE_FILL_COLOR);
  ctx.document.AddPtr(move(circle));
}

/**
 * Отрисовывает название остановки.
 */
void RenderStopTitle(SvgRenderContext &ctx, const Stop *stop) {
  const auto &rs = ctx.render_settings;
  auto name_undertitle = make_unique<svg::Text>();
  name_undertitle->SetData(stop->name)
      .SetOffset(rs.stop_label_offset)
      .SetFontSize(rs.stop_label_font_size)
      .SetFontFamily(STOP_LABEL_FONT_FAMILY)
      .SetPosition(ctx.sphere_projector(stop->coords));

  auto name_label = make_unique<svg::Text>(*name_undertitle);

  name_undertitle->SetFillColor(rs.underlayer_color)
      .SetStrokeColor(rs.underlayer_color)
      .SetStrokeWidth(rs.underlayer_width)
      .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
      .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

  name_label->SetFillColor(STOP_LABEL_FILL_COLOR);

  ctx.document.AddPtr(move(name_undertitle));
  ctx.document.AddPtr(move(name_label));
}

/**
 * Отрисовывает все остановки.
 */
void RenderStops(SvgRenderContext &ctx, const vector<const Stop *> &stops) {
  for_each(stops.begin(), stops.end(),
           [&ctx](const Stop *stop) { RenderStopCircle(ctx, stop); });
  for_each(stops.begin(), stops.end(),
           [&ctx](const Stop *stop) { RenderStopTitle(ctx, stop); });
}

}  // namespace detail

/**
 * Отрисовать SVG карту с настройками `render_settings`.
 * SVG текст выводится в поток `out_`, переданный в конструкторе.
 * Маршруты и остановки берутся из `transport_catalogue_`, переданного в
 * конструкторе.
 */
void SvgMapRenderer::RenderMap(const RenderSettings &render_settings) {
  auto buses = transport_catalogue_.GetBuses();
  detail::SvgRenderContext ctx{
      detail::MakeSphereProjector(buses, render_settings), {}, render_settings};
  sort(buses.begin(), buses.end(),
       [](const Bus *a, const Bus *b) { return a->name < b->name; });
  detail::RenderBuses(ctx, buses);
  detail::RenderStops(ctx, detail::CollectStops(buses));

  ctx.document.Render(out_);
}

}  // namespace transport_catalogue::map_renderer
