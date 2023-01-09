#pragma once

#include <stddef.h>

#include <iostream>
#include <vector>

#include "svg.h"

namespace transport_catalogue {
class TransportCatalogue;
} /* namespace transport_catalogue */

namespace transport_catalogue::map_renderer {

/**
 * Настройки отрисовки карты маршрутов в SVG формате.
 */
struct RenderSettings {
  double width = 0.0;
  double height = 0.0;
  double padding = 0.0;
  double line_width = 0.0;
  double stop_radius = 0.0;
  size_t bus_label_font_size = 0;
  svg::Point bus_label_offset = {0.0, 0.0};
  double stop_label_font_size = 0.0;
  svg::Point stop_label_offset = {0.0, 0.0};
  svg::Color underlayer_color = {};
  double underlayer_width = 0.0;
  std::vector<svg::Color> color_palette;
};

/**
 * Абстрактный класс для отрисовки карты.
 */
class MapRenderer {
 public:
  /**
   * Отрисовать карту с указанными настройками.
   */
  virtual void RenderMap(const RenderSettings &) = 0;

 protected:
  ~MapRenderer() = default;
};

/**
 * Отрисовщик карты в SVG формате
 */
class SvgMapRenderer final : public MapRenderer {
 public:
  SvgMapRenderer(const TransportCatalogue &transport_catalogue,
                 std::ostream &out)
      : transport_catalogue_(transport_catalogue), out_(out) {}
  virtual void RenderMap(const RenderSettings &) override;

 private:
  const TransportCatalogue &transport_catalogue_;
  std::ostream &out_;
};

}  // namespace transport_catalogue::map_renderer
