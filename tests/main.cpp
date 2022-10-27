#include "geo.h"
#include "input_reader.h"
#include "json.h"
#include "json_builder.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
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
  TestJSON(tr);
  TestJSONReader(tr);
  TestRequestHandler(tr);
  TestMapRenderer(tr);
  TestJsonBuilder(tr);
}
