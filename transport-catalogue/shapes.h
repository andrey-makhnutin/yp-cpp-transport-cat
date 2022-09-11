#pragma once

#include "svg.h"

namespace shapes {

class Star final : public svg::Drawable {
 public:
  Star(svg::Point center, double outer_radius, double inner_radius,
       int num_rays);
  virtual void Draw(svg::ObjectContainer &obj_container) const override;

 private:
  svg::Point center_;
  double outer_radius_ = 0;
  double inner_radius_ = 0;
  int num_rays_ = 0;
};

class Snowman final : public svg::Drawable {
 public:
  Snowman(svg::Point center, double head_radius);
  virtual void Draw(svg::ObjectContainer &obj_container) const override;

 private:
  svg::Point center_;
  double head_radius_;
};

class Triangle final : public svg::Drawable {
 public:
  Triangle(svg::Point p1, svg::Point p2, svg::Point p3);
  virtual void Draw(svg::ObjectContainer &obj_container) const override;

 private:
  svg::Point points_[3];
};

}  // namespace shapes
