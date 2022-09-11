#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <stdint.h>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace svg {

struct Rgb {
  Rgb() = default;
  Rgb(unsigned int _red, unsigned int _green, unsigned int _blue)
      :
      red(_red),
      green(_green),
      blue(_blue) {
  }
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
};

struct Rgba {
  Rgba() = default;
  Rgba(unsigned int _red, unsigned int _green, unsigned int _blue,
       double _opacity)
      :
      red(_red),
      green(_green),
      blue(_blue),
      opacity(_opacity) {
  }
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
  double opacity = 1.0;
};

using Color = std::variant<std::monostate,std::string,Rgb,Rgba>;
inline const Color NoneColor = { };

enum class StrokeLineCap {
  BUTT,
  ROUND,
  SQUARE,
};

enum class StrokeLineJoin {
  ARCS,
  BEVEL,
  MITER,
  MITER_CLIP,
  ROUND,
};

}  // namespace svg

std::ostream& operator<<(std::ostream &out, svg::StrokeLineCap);
std::ostream& operator<<(std::ostream &out, svg::StrokeLineJoin);
std::ostream& operator<<(std::ostream &out, svg::Color);

namespace svg {

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
  RenderContext(std::ostream &out)
      :
      out(out) {
  }

  RenderContext(std::ostream &out, int indent_step, int indent = 0)
      :
      out(out),
      indent_step(indent_step),
      indent(indent) {
  }

  RenderContext Indented() const {
    return {out, indent_step, indent + indent_step};
  }

  void RenderIndent() const {
    for (int i = 0; i < indent; ++i) {
      out.put(' ');
    }
  }

  std::ostream &out;
  int indent_step = 0;
  int indent = 0;
};

/*
 * Базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
 public:
  virtual ~Object() = default;
  void Render(const RenderContext &context) const;
 private:
  virtual void RenderObject(const RenderContext &context) const = 0;
};

struct Point {
  Point() = default;
  Point(double x, double y)
      :
      x(x),
      y(y) {
  }
  double x = 0;
  double y = 0;
};

/**
 * Наследование от этого класса разрешает выставить объекту свойства,
 * применимые к фигурам с контуром.
 *
 * Шаблонный параметр `Owner` должен быть равен типу, который наследует
 * этот класс (Curiously Recurring Template Pattern):
 * ```
 * class Circle final : public Object, public PathProps<Circle> {
 *   ...
 * }
 * ```
 */
template<typename Owner>
class PathProps {
 public:
  /**
   * Цвет заполнения (атрибут fill)
   */
  Owner& SetFillColor(Color color) {
    fill_color_ = std::move(color);
    return AsOwner();
  }
  /**
   * Цвет контура (атрибут stroke)
   */
  Owner& SetStrokeColor(Color color) {
    stroke_color_ = std::move(color);
    return AsOwner();
  }
  /**
   * Толщина линии контура (атрибут stroke-width)
   */
  Owner& SetStrokeWidth(double width) {
    stroke_width_ = width;
    return AsOwner();
  }
  /**
   * Тип формы конца линии (атрибут stroke-linecap)
   */
  Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
    stroke_line_cap_ = line_cap;
    return AsOwner();
  }
  /**
   * Тип формы соединения линий (атрибут stroke-linejoin)
   */
  Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
    stroke_line_join_ = line_join;
    return AsOwner();
  }

 protected:
  ~PathProps() = default;

  /**
   * Выводит выставленные свойства заливки и контура в поток.
   * По-умолчанию свойства не выводятся.
   */
  void RenderAttrs(std::ostream &out) const {
    using namespace std::literals;

    if (fill_color_) {
      out << " fill=\""sv << *fill_color_ << "\""sv;
    }
    if (stroke_color_) {
      out << " stroke=\""sv << *stroke_color_ << "\""sv;
    }
    if (stroke_width_) {
      out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
    }
    if (stroke_line_cap_) {
      out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
    }
    if (stroke_line_join_) {
      out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
    }
  }

 private:
  Owner& AsOwner() {
    // static_cast безопасно преобразует *this к Owner&,
    // если класс Owner — наследник PathProps
    return static_cast<Owner&>(*this);
  }

  std::optional<Color> fill_color_;
  std::optional<Color> stroke_color_;
  std::optional<double> stroke_width_;
  std::optional<StrokeLineCap> stroke_line_cap_;
  std::optional<StrokeLineJoin> stroke_line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
 public:
  Circle& SetCenter(Point center);
  Circle& SetRadius(double radius);

 private:
  virtual void RenderObject(const RenderContext &context) const override;

  Point center_;
  double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
 public:
  // Добавляет очередную вершину к ломаной линии
  Polyline& AddPoint(Point point);

 private:
  virtual void RenderObject(const RenderContext &context) const override;

  std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
 public:
  // Задаёт координаты опорной точки (атрибуты x и y)
  Text& SetPosition(Point pos);

  // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
  Text& SetOffset(Point offset);

  // Задаёт размеры шрифта (атрибут font-size)
  Text& SetFontSize(uint32_t size);

  // Задаёт название шрифта (атрибут font-family)
  Text& SetFontFamily(std::string font_family);

  // Задаёт толщину шрифта (атрибут font-weight)
  Text& SetFontWeight(std::string font_weight);

  // Задаёт текстовое содержимое объекта (отображается внутри тега text)
  Text& SetData(std::string data);

 private:
  virtual void RenderObject(const RenderContext &context) const override;

  Point pos_;
  Point offset_;
  uint32_t font_size_ = 1;
  std::optional<std::string> font_family_;
  std::optional<std::string> font_weight_;
  std::string data_;
};

/**
 * Задаёт интерфейс для доступа к контейнеру SVG-объектов.
 * Через этот интерфейс `Drawable`-объекты могут визуализировать себя, добавляя в контейнер SVG-примитивы.
 */
class ObjectContainer {
 public:
  /*
   Метод Add добавляет в контейнер любой объект-наследник svg::Object.
   Пример использования:
   Document doc;
   doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
   */
  template<typename T>
  void Add(T obj) {
    AddPtr(std::make_unique<T>(std::move(obj)));
  }

  // Добавляет в контейнер объект-наследник svg::Object
  virtual void AddPtr(std::unique_ptr<Object> &&obj) = 0;
 protected:
  // Кажется, особенной пользы в полиморфическом владении нет.
  // Запрещаем.
  ~ObjectContainer() = default;
};

class Document final : public ObjectContainer {
 public:
  // Выводит в ostream svg-представление документа
  void Render(std::ostream &out) const;

  virtual void AddPtr(std::unique_ptr<Object> &&obj) override;
 private:
  std::vector<std::unique_ptr<Object>> objects_;
};

/**
 * Объект, который можно нарисовать с помощью `ObjectContainer`
 */
class Drawable {
 public:
  virtual ~Drawable() = default;
  virtual void Draw(ObjectContainer &obj_container) const = 0;
};

}  // namespace svg
