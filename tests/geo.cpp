#include "../transport-catalogue/geo.h"
#include "geo.h"

#include <iostream>

#include "test_framework.h"

using namespace std;

namespace transport_catalogue::geo {

void TestCoordinates() {
  Coordinates c1 { 55.574371, 37.651700 };
  Coordinates c2 { 55.5743719999, 37.65170099999 };

  ASSERT_EQUAL(c1, c2);

  Coordinates c3 { 55.574372, 37.651700 };
  ASSERT_NOT_EQUAL(c1, c3);
  Coordinates c4 { 55.574371, 37.651701 };
  ASSERT_NOT_EQUAL(c1, c4);
  Coordinates c5 { 55.574370, 37.651700 };
  ASSERT_NOT_EQUAL(c1, c5);
  Coordinates c6 { 55.574371, 37.651699 };
  ASSERT_NOT_EQUAL(c1, c6);
}

}  // namespace transport_catalogue::geo

using transport_catalogue::geo::Coordinates;
ostream& operator<<(ostream &os, const Coordinates &c) {
  os << '(' << c.lat << ", " << c.lng << ')';
  return os;
}

void TestGeo(TestRunner &tr) {
  using namespace transport_catalogue::geo;

  RUN_TEST(tr, TestCoordinates);
}
