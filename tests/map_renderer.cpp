#include "../transport-catalogue/map_renderer.h"
#include "map_renderer.h"

#include <sstream>
#include <string>
#include <variant>

#include "../transport-catalogue/domain.h"
#include "../transport-catalogue/geo.h"
#include "../transport-catalogue/transport_catalogue.h"
#include "test_framework.h"

using namespace std;

namespace transport_catalogue::map_renderer::tests {

void TestRenderSvgMap() {
  TransportCatalogue tc;
  tc.AddStop("Rivierskiy most"s, { 43.587795, 39.716901 });
  tc.AddStop("Morskoy vokzal"s, { 43.581969, 39.719848 });
  tc.AddStop("Elektroseti"s, { 43.598701, 39.730623 });
  tc.AddStop("Ulitsa Dokuchaeva"s, { 43.585586, 39.733879 });
  tc.AddStop("Ulitsa Lizy Chaikinoi"s, { 43.590317, 39.746833 });
  tc.AddBus("14"s, RouteType::CIRCULAR, vector<string> {
                "Ulitsa Lizy Chaikinoi"s, "Elektroseti"s, "Ulitsa Dokuchaeva"s,
                "Ulitsa Lizy Chaikinoi"s, });
  tc.AddBus("114"s, RouteType::LINEAR, vector<string> { "Morskoy vokzal"s,
                "Rivierskiy most"s, });

  RenderSettings rs;
  rs.width = 600;
  rs.height = 400;
  rs.padding = 50;
  rs.stop_radius = 5;
  rs.line_width = 14;
  rs.bus_label_font_size = 20;
  rs.bus_label_offset = svg::Point { 7, 15 };
  rs.stop_label_font_size = 20;
  rs.stop_label_offset = svg::Point { 7, -3 };
  rs.underlayer_color = svg::Rgba { 255, 255, 255, 0.85 };
  rs.underlayer_width = 3;
  rs.color_palette = vector<svg::Color> { "green"s, svg::Rgb { 255, 160, 0 },
      "red"s };

  ostringstream sout;
  SvgMapRenderer map_renderer(tc, sout);
  map_renderer.RenderMap(rs);
  ASSERT_EQUAL(
      sout.str(),
      R"eox(<?xml version="1.0" encoding="UTF-8" ?>
<svg xmlns="http://www.w3.org/2000/svg" version="1.1">
  <polyline points="99.2283,329.5 50,232.18 99.2283,329.5" fill="none" stroke="green" stroke-width="14" stroke-linecap="round" stroke-linejoin="round" />
  <polyline points="550,190.051 279.22,50 333.61,269.08 550,190.051" fill="none" stroke="rgb(255,160,0)" stroke-width="14" stroke-linecap="round" stroke-linejoin="round" />
  <text x="99.2283" y="329.5" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">114</text>
  <text x="99.2283" y="329.5" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="green">114</text>
  <text x="50" y="232.18" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">114</text>
  <text x="50" y="232.18" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="green">114</text>
  <text x="550" y="190.051" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">14</text>
  <text x="550" y="190.051" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold" fill="rgb(255,160,0)">14</text>
  <circle cx="279.22" cy="50" r="5" fill="white" />
  <circle cx="99.2283" cy="329.5" r="5" fill="white" />
  <circle cx="50" cy="232.18" r="5" fill="white" />
  <circle cx="333.61" cy="269.08" r="5" fill="white" />
  <circle cx="550" cy="190.051" r="5" fill="white" />
  <text x="279.22" y="50" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">Elektroseti</text>
  <text x="279.22" y="50" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="black">Elektroseti</text>
  <text x="99.2283" y="329.5" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">Morskoy vokzal</text>
  <text x="99.2283" y="329.5" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="black">Morskoy vokzal</text>
  <text x="50" y="232.18" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">Rivierskiy most</text>
  <text x="50" y="232.18" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="black">Rivierskiy most</text>
  <text x="333.61" y="269.08" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">Ulitsa Dokuchaeva</text>
  <text x="333.61" y="269.08" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="black">Ulitsa Dokuchaeva</text>
  <text x="550" y="190.051" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">Ulitsa Lizy Chaikinoi</text>
  <text x="550" y="190.051" dx="7" dy="-3" font-size="20" font-family="Verdana" fill="black">Ulitsa Lizy Chaikinoi</text>
</svg>
)eox"s);
}

}  // namespace transport_catalogue::map_renderer::tests

void TestMapRenderer(TestRunner &tr) {
  using namespace transport_catalogue::map_renderer::tests;

  RUN_TEST(tr, TestRenderSvgMap);
}
