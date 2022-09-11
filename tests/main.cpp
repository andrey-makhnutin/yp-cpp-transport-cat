#include "geo.h"
#include "input_reader.h"
#include "shapes.h"
#include "stat_reader.h"
#include "svg.h"
#include "test_framework.h"
#include "transport_catalogue.h"

int main() {
  TestRunner tr;

  TestGeo(tr);
  TestInputReader(tr);
  TestStatReader(tr);
  TestTransportCatalogue(tr);
  TestSVG(tr);
  TestShapes(tr);
}
