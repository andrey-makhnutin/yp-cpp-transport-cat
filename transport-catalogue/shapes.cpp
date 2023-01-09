#include "shapes.h"

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <string>

using namespace std;

namespace shapes {

namespace detail {

svg::Polyline CreateStar(svg::Point center, double outer_rad, double inner_rad,
                         int num_rays) {
  using namespace svg;
  Polyline polyline;
  assert(num_rays > 0);
  for (int i = 0; i <= num_rays; ++i) {
    double angle = 2 * M_PI * (i % num_rays) / num_rays;
    polyline.AddPoint(
        {center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle)});
    if (i == num_rays) {
      break;
    }
    angle += M_PI / num_rays;
    polyline.AddPoint(
        {center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle)});
  }
  return polyline;
}

}  // namespace detail

const svg::Color STAR_FILL_COLOR{"red"};
const svg::Color STAR_STROKE_COLOR{"black"};
const svg::Color SNOWMAN_FILL_COLOR{"rgb(240,240,240)"};
const svg::Color SNOWMAN_STROKE_COLOR{"black"};

// -------------- Star --------------------------

Star::Star(svg::Point center, double outer_radius, double inner_radius,
           int num_rays)
    : center_(center),
      outer_radius_(outer_radius),
      inner_radius_(inner_radius),
      num_rays_(num_rays) {}

void Star::Draw(svg::ObjectContainer &obj_container) const {
  obj_container.Add(
      detail::CreateStar(center_, outer_radius_, inner_radius_, num_rays_)
          .SetFillColor(STAR_FILL_COLOR)
          .SetStrokeColor(STAR_STROKE_COLOR));
}

// -------------- Snowman -----------------------

Snowman::Snowman(svg::Point center, double head_radius)
    : center_(center), head_radius_(head_radius) {}

void Snowman::Draw(svg::ObjectContainer &obj_container) const {
  obj_container.Add(svg::Circle()
                        .SetCenter({center_.x, center_.y + 5 * head_radius_})
                        .SetRadius(2 * head_radius_)
                        .SetFillColor(SNOWMAN_FILL_COLOR)
                        .SetStrokeColor(SNOWMAN_STROKE_COLOR));
  obj_container.Add(svg::Circle()
                        .SetCenter({center_.x, center_.y + 2 * head_radius_})
                        .SetRadius(1.5 * head_radius_)
                        .SetFillColor(SNOWMAN_FILL_COLOR)
                        .SetStrokeColor(SNOWMAN_STROKE_COLOR));
  obj_container.Add(svg::Circle()
                        .SetCenter({center_.x, center_.y})
                        .SetRadius(head_radius_)
                        .SetFillColor(SNOWMAN_FILL_COLOR)
                        .SetStrokeColor(SNOWMAN_STROKE_COLOR));
}

// -------------- Triangle ---------------------

Triangle::Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
    : points_{p1, p2, p3} {}

void Triangle::Draw(svg::ObjectContainer &obj_container) const {
  auto polyline = make_unique<svg::Polyline>();
  for (auto p : points_) {
    polyline->AddPoint(p);
  }
  polyline->AddPoint(points_[0]);
  obj_container.AddPtr(move(polyline));
}

}  // namespace shapes
