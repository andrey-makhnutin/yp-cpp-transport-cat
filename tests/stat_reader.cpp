#include "../transport-catalogue/stat_reader.h"

#include <iomanip>
#include <sstream>
#include <string>

#include "../transport-catalogue/transport_catalogue.h"
#include "stat_reader.h"
#include "test_framework.h"

using namespace std;

namespace transport_catalogue::stat_reader::from_char_stream::tests {

void TestStatRequestProcessor() {
  istringstream sin{
      "5\n"
      "Bus 750\n"
      "Stop A\n"
      "Bus 751\n"
      "Bus 752\n"
      "Stop B\n"
      "Hello Rest\n"};
  StatsRequestProcessor stat_processor{sin};
  TransportCatalogue tc;
  ostringstream sout;
  to_char_stream::StatsPrinter stat_printer{sout};
  stat_processor.ProcessRequests(tc, stat_printer);
  istringstream sin2{sout.str()};
  string line;
  getline(sin2, line);
  ASSERT_EQUAL(line, "Bus 750: not found");
  getline(sin2, line);
  ASSERT_EQUAL(line, "Stop A: not found");
  getline(sin2, line);
  ASSERT_EQUAL(line, "Bus 751: not found");
  getline(sin2, line);
  ASSERT_EQUAL(line, "Bus 752: not found");
  getline(sin2, line);
  ASSERT_EQUAL(line, "Stop B: not found");
  // проверяем, что `stat_reader` не "съел" весь поток
  string rest;
  getline(sin, rest);
  ASSERT_EQUAL(rest, "Hello Rest"s);
}

}  // namespace transport_catalogue::stat_reader::from_char_stream::tests

namespace transport_catalogue::stat_reader::to_char_stream::tests {

void TestBusStatPrinter() {
  {
    ostringstream sout;
    BusStats bus_stats{6, 5, 12345.67, 10000.0};
    StatsPrinter stats_printer{sout};

    sout << setprecision(7);
    sout << 1.234567 << endl;
    stats_printer.PrintBusStats("750"sv, bus_stats);
    sout << 1.234567 << endl;

    istringstream sin{sout.str()};
    string line;
    getline(sin, line);
    ASSERT_EQUAL(line, "1.234567"s);
    getline(sin, line);
    ASSERT_EQUAL(line,
                 "Bus 750: 6 stops on route, 5 unique stops, 12345.7 route "
                 "length, 1.23457 curvature")
    // проверяем, что принтер вернул настройку точности вывода обратно на 7
    getline(sin, line);
    ASSERT_EQUAL(line, "1.234567"s);
  }
  {
    ostringstream sout;
    StatsPrinter stats_printer{sout};

    sout << setprecision(7);
    sout << 1.234567 << endl;
    stats_printer.PrintBusStats("750"sv, std::nullopt);
    sout << 1.234567 << endl;

    istringstream sin{sout.str()};
    string line;
    getline(sin, line);
    ASSERT_EQUAL(line, "1.234567"s);
    getline(sin, line);
    ASSERT_EQUAL(line, "Bus 750: not found")
    // проверяем, что принтер вернул настройку точности вывода обратно на 7
    getline(sin, line);
    ASSERT_EQUAL(line, "1.234567"s);
  }
}

void TestStopInfoPrinter() {
  ostringstream sout;
  StatsPrinter stats_printer{sout};

  stats_printer.PrintStopInfo("Stop1"sv, BusesForStop{"BusA"sv, "BusB"sv});
  stats_printer.PrintStopInfo("Stop2"sv, nullopt);
  stats_printer.PrintStopInfo("Stop3"sv, BusesForStop{});

  istringstream sin{sout.str()};
  string line;
  getline(sin, line);
  ASSERT_EQUAL(line, "Stop Stop1: buses BusA BusB"s);
  getline(sin, line);
  ASSERT_EQUAL(line, "Stop Stop2: not found")
  getline(sin, line);
  ASSERT_EQUAL(line, "Stop Stop3: no buses")
}

}  // namespace transport_catalogue::stat_reader::to_char_stream::tests

void TestStatReader(TestRunner &tr) {
  {
    using namespace transport_catalogue::stat_reader::from_char_stream::tests;

    RUN_TEST(tr, TestStatRequestProcessor);
  }
  {
    using namespace transport_catalogue::stat_reader::to_char_stream::tests;

    RUN_TEST(tr, TestBusStatPrinter);
    RUN_TEST(tr, TestStopInfoPrinter);
  }
}
