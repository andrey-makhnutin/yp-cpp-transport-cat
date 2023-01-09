#include "../transport-catalogue/svg.h"

#include <sstream>

#include "svg.h"
#include "test_framework.h"

using namespace std;

namespace svg::tests {

#define TEST_RENDER(el, text)                                                  \
  {                                                                            \
    Document doc;                                                              \
    doc.Add(el);                                                               \
    stringstream sout;                                                         \
    doc.Render(sout);                                                          \
    stringstream sin{sout.str()};                                              \
    string line;                                                               \
    getline(sin, line);                                                        \
    ASSERT_EQUAL(line, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv);       \
    getline(sin, line);                                                        \
    ASSERT_EQUAL(                                                              \
        line, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv); \
    getline(sin, line);                                                        \
    ASSERT_EQUAL(line, "  "s + text);                                          \
    getline(sin, line);                                                        \
    ASSERT_EQUAL(line, "</svg>"sv);                                            \
  }

void TestCircle() {
  TEST_RENDER(Circle(), R"(<circle cx="0" cy="0" r="1" />)"s);
  TEST_RENDER(
      Circle()
          .SetCenter({12.21, 34.43})
          .SetRadius(56.65)
          .SetFillColor("test-fill-color"s)
          .SetStrokeColor("test-stroke-color"s)
          .SetStrokeWidth(1.2)
          .SetStrokeLineCap(StrokeLineCap::ROUND)
          .SetStrokeLineJoin(StrokeLineJoin::MITER),
      R"(<circle cx="12.21" cy="34.43" r="56.65" fill="test-fill-color")"
      R"( stroke="test-stroke-color" stroke-width="1.2" stroke-linecap="round")"
      R"( stroke-linejoin="miter" />)"s);
}

void TestPolyline() {
  TEST_RENDER(Polyline(), R"(<polyline points="" />)"s);
  TEST_RENDER(Polyline().AddPoint({12.34, 56.78}),
              R"(<polyline points="12.34,56.78" />)"s);
  TEST_RENDER(
      Polyline()
          .AddPoint({12.34, 56.78})
          .AddPoint({43.21, 87.65})
          .SetFillColor("test-fill-color"s)
          .SetStrokeColor("test-stroke-color"s)
          .SetStrokeWidth(1.2)
          .SetStrokeLineCap(StrokeLineCap::ROUND)
          .SetStrokeLineJoin(StrokeLineJoin::MITER),
      R"(<polyline points="12.34,56.78 43.21,87.65" fill="test-fill-color")"
      R"( stroke="test-stroke-color" stroke-width="1.2" stroke-linecap="round")"
      R"( stroke-linejoin="miter" />)"s);
}

void TestText() {
  TEST_RENDER(Text(),
              R"(<text x="0" y="0" dx="0" dy="0" font-size="1"></text>)"s);
  TEST_RENDER(
      Text().SetPosition({12.34, 56.78}).SetData("hello text"),
      R"(<text x="12.34" y="56.78" dx="0" dy="0" font-size="1">hello text</text>)"s);
  TEST_RENDER(
      Text()
          .SetPosition({12.34, 56.78})
          .SetData("hello text")
          .SetOffset({4.3, 2.1})
          .SetFontSize(3)
          .SetFontFamily("test-fam"s)
          .SetFontWeight("test-wei"s)
          .SetFillColor("test-fill-color"s)
          .SetStrokeColor("test-stroke-color"s)
          .SetStrokeWidth(1.2)
          .SetStrokeLineCap(StrokeLineCap::ROUND)
          .SetStrokeLineJoin(StrokeLineJoin::MITER),
      R"(<text x="12.34" y="56.78" dx="4.3" dy="2.1" font-size="3")"
      R"( font-family="test-fam" font-weight="test-wei" fill="test-fill-color")"
      R"( stroke="test-stroke-color" stroke-width="1.2" stroke-linecap="round")"
      R"( stroke-linejoin="miter">hello text</text>)"s);
  TEST_RENDER(
      Text().SetData("<>\"'&"),
      R"(<text x="0" y="0" dx="0" dy="0" font-size="1">&lt;&gt;&quot;&apos;&amp;</text>)"s)
}

void TestDocument() {
  Document doc;
  doc.Add(Circle());
  doc.Add(Polyline());
  ostringstream sout;
  doc.Render(sout);
  ASSERT_EQUAL(sout.str(),
               R"(<?xml version="1.0" encoding="UTF-8" ?>
<svg xmlns="http://www.w3.org/2000/svg" version="1.1">
  <circle cx="0" cy="0" r="1" />
  <polyline points="" />
</svg>
)");
}

#define TEST_COLOR_PRINT(color_, str_) \
  {                                    \
    Color c = color_;                  \
    ostringstream sout;                \
    sout << c;                         \
    ASSERT_EQUAL(sout.str(), str_);    \
  }

void TestColor() {
  TEST_COLOR_PRINT({}, "none"s);
  TEST_COLOR_PRINT((Rgb{215, 30, 25}), "rgb(215,30,25)"s);
  TEST_COLOR_PRINT(NoneColor, "none"s);
  TEST_COLOR_PRINT((Rgba{15, 15, 25, 0.7}), "rgba(15,15,25,0.7)"s);
  TEST_COLOR_PRINT("red"s, "red"s);
  TEST_COLOR_PRINT(Rgb{}, "rgb(0,0,0)"s);
  TEST_COLOR_PRINT(Rgb(1u, 2u, 3u), "rgb(1,2,3)"s);
  TEST_COLOR_PRINT(Rgba{}, "rgba(0,0,0,1)"s);
  TEST_COLOR_PRINT(Rgba(1u, 2u, 3u, 0.7), "rgba(1,2,3,0.7)"s);
}

}  // namespace svg::tests

void TestSVG(TestRunner &tr) {
  using namespace svg::tests;

  RUN_TEST(tr, TestCircle);
  RUN_TEST(tr, TestPolyline);
  RUN_TEST(tr, TestText);
  RUN_TEST(tr, TestDocument);
  RUN_TEST(tr, TestColor);
}
