#include "../transport-catalogue/shapes.h"
#include "shapes.h"

#include <sstream>
#include <string>
#include <vector>

#include "test_framework.h"

using namespace std;
using namespace svg;

namespace shapes {

#define TEST_RENDER(obj_, texts_...) {                                          \
  Document doc;                                                                 \
  auto obj = obj_;                                                              \
  obj.Draw(doc);                                                                \
  ostringstream sout;                                                           \
  doc.Render(sout);                                                             \
  stringstream sin { sout.str() };                                              \
  string line;                                                                  \
  getline(sin, line);                                                           \
  ASSERT_EQUAL(line, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv);          \
  getline(sin, line);                                                           \
  ASSERT_EQUAL(line,                                                            \
               "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv); \
  vector<string> texts { texts_ };                                              \
  for (const auto& text : texts) {                                              \
    getline(sin, line);                                                         \
    ASSERT_EQUAL(line, "  "s + text);                                           \
  }                                                                             \
  getline(sin, line);                                                           \
  ASSERT_EQUAL(line, "</svg>"sv);                                               \
}

void TestStar() {
  TEST_RENDER(Star(Point { 50.0, 20.0 }, 10.0, 4.0, 5),
              R"(<polyline points="50,10 52.3511,16.7639 59.5106,16.9098)"
              R"( 53.8042,21.2361 55.8779,28.0902 50,24 44.1221,28.0902)"
              R"( 46.1958,21.2361 40.4894,16.9098 47.6489,16.7639 50,10")"
              R"( fill="red" stroke="black" />)"s);
}

void TestSnowman() {
  TEST_RENDER(Snowman(Point { 30, 20 }, 10.0),
              R"(<circle cx="30" cy="70" r="20")"
              R"eos( fill="rgb(240,240,240)" stroke="black" />)eos"s,
              R"(<circle cx="30" cy="40" r="15")"
              R"eos( fill="rgb(240,240,240)" stroke="black" />)eos"s,
              R"(<circle cx="30" cy="20" r="10")"
              R"eos( fill="rgb(240,240,240)" stroke="black" />)eos"s,
              );
}

void TestTriangle() {
  TEST_RENDER(Triangle(Point { 100, 20 }, Point { 120, 50 }, Point { 80, 40 }),
              R"(<polyline points="100,20 120,50 80,40 100,20" />)"s);
}

}

void TestShapes(TestRunner &tr) {
  using namespace shapes;

  RUN_TEST(tr, TestStar);
  RUN_TEST(tr, TestSnowman);
  RUN_TEST(tr, TestTriangle);
}
