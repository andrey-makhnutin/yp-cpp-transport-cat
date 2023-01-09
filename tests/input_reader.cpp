#include "../transport-catalogue/input_reader.h"

#include <iterator>
#include <optional>
#include <sstream>

#include "../transport-catalogue/transport_catalogue.h"
#include "input_reader.h"
#include "test_framework.h"

using namespace std;

namespace transport_catalogue::input_reader::from_char_stream::tests {

#define TEST_SPLIT_NO_WS(line, by, expected_parts...)           \
  {                                                             \
    auto parts = detail::SplitNoWS(line, by);                   \
    ASSERT_EQUAL(parts, (vector<string_view>{expected_parts})); \
  }
void TestSplitNoWS() {
  TEST_SPLIT_NO_WS("hello > world"sv, '>', "hello"sv, "world"sv);
  TEST_SPLIT_NO_WS(" hello > world"sv, '>', "hello"sv, "world"sv);
  TEST_SPLIT_NO_WS("hello>world"sv, '>', "hello"sv, "world"sv);
  TEST_SPLIT_NO_WS("     hello>world"sv, '>', "hello"sv, "world"sv);
  TEST_SPLIT_NO_WS("hello     >world"sv, '>', "hello"sv, "world"sv);
  TEST_SPLIT_NO_WS("hello>     world"sv, '>', "hello"sv, "world"sv);
  TEST_SPLIT_NO_WS("hello>world     "sv, '>', "hello"sv, "world"sv);

  TEST_SPLIT_NO_WS("hello"sv, '>', "hello"sv);
  TEST_SPLIT_NO_WS("   hello"sv, '>', "hello"sv);
  TEST_SPLIT_NO_WS("hello   "sv, '>', "hello"sv);

  TEST_SPLIT_NO_WS("hello>>world"sv, '>', "hello"sv, ""sv, "world"sv);
  TEST_SPLIT_NO_WS("hello> >world"sv, '>', "hello"sv, ""sv, "world"sv);
  TEST_SPLIT_NO_WS("hello>  >world"sv, '>', "hello"sv, ""sv, "world"sv);
  TEST_SPLIT_NO_WS("hello>   >world"sv, '>', "hello"sv, ""sv, "world"sv);
  TEST_SPLIT_NO_WS(">world"sv, '>', ""sv, "world"sv);
  TEST_SPLIT_NO_WS(" >world"sv, '>', ""sv, "world"sv);
  TEST_SPLIT_NO_WS("  >world"sv, '>', ""sv, "world"sv);
  TEST_SPLIT_NO_WS("hello>"sv, '>', "hello"sv, ""sv);
  TEST_SPLIT_NO_WS("hello> "sv, '>', "hello"sv, ""sv);
  TEST_SPLIT_NO_WS("hello>  "sv, '>', "hello"sv, ""sv);

  TEST_SPLIT_NO_WS("hello"sv, '>', "hello"sv);
  TEST_SPLIT_NO_WS(" hello"sv, '>', "hello"sv);
  TEST_SPLIT_NO_WS("hello "sv, '>', "hello"sv);
  TEST_SPLIT_NO_WS("  hello"sv, '>', "hello"sv);
  TEST_SPLIT_NO_WS("hello  "sv, '>', "hello"sv);

  TEST_SPLIT_NO_WS(""sv, '>', ""sv);
  TEST_SPLIT_NO_WS(" "sv, '>', ""sv);
  TEST_SPLIT_NO_WS("  "sv, '>', ""sv);
  TEST_SPLIT_NO_WS("   "sv, '>', ""sv);

  TEST_SPLIT_NO_WS("hello > world"sv, ':', "hello > world"sv);

  TEST_SPLIT_NO_WS("hello   world", ' ', "hello"sv, "world"sv);
  TEST_SPLIT_NO_WS("  hello to world  ", ' ', "hello"sv, "to"sv, "world"sv);
  TEST_SPLIT_NO_WS("  hello    to    world  ", ' ', "hello"sv, "to"sv,
                   "world"sv);
  TEST_SPLIT_NO_WS(
      "\t \n\r \t \n \r hello\t \n\r \t \n \r "
      "to\t \n\r \t \n \r world\t \n\r \t \n \r ",
      ' ', "hello"sv, "to"sv, "world"sv);
  TEST_SPLIT_NO_WS(
      "\t \n\r \t \n \r hello\t \n\r \t \n \r "
      "to\t \n\r \t \n \r world\t \n\r \t \n \r ",
      '\t', "hello"sv, "to"sv, "world"sv);
  TEST_SPLIT_NO_WS(
      "\t \n\r \t \n \r hello\t \n\r \t \n \r "
      "to\t \n\r \t \n \r world\t \n\r \t \n \r ",
      '\n', "hello"sv, "to"sv, "world"sv);

  TEST_SPLIT_NO_WS("hello to world", " to "sv, "hello"sv, "world"sv);
  TEST_SPLIT_NO_WS("  hello    to    world  ", " to "sv, "hello"sv, "world"sv);
}

#define TEST_STOP_PARSER(str, _name, _lat, _lng)  \
  {                                               \
    istringstream sin{"1\n" str};                 \
    DbReader input{sin};                          \
    const auto &stops = input.GetAddStopCmds();   \
    ASSERT_EQUAL(stops.size(), 1u);               \
    const AddStopCmd &cmd = *stops.begin();       \
    ASSERT_EQUAL(cmd.name, _name);                \
    ASSERT_SOFT_EQUAL(cmd.coordinates.lat, _lat); \
    ASSERT_SOFT_EQUAL(cmd.coordinates.lng, _lng); \
    ASSERT_EQUAL(cmd.distances.size(), 0u);       \
  }

#define TEST_STOP_DIS_PARSER(str, _dises...)                             \
  {                                                                      \
    istringstream sin{"1\nStop A:1.2,3.4," str};                         \
    DbReader input{sin};                                                 \
    const auto &stops = input.GetAddStopCmds();                          \
    ASSERT_EQUAL(stops.size(), 1u);                                      \
    const AddStopCmd &cmd = *stops.begin();                              \
    ASSERT_EQUAL(cmd.name, "A"sv);                                       \
    ASSERT_SOFT_EQUAL(cmd.coordinates.lat, 1.2);                         \
    ASSERT_SOFT_EQUAL(cmd.coordinates.lng, 3.4);                         \
    ASSERT_EQUAL(cmd.distances, (vector<AddStopCmd::Distance>{_dises})); \
  }

void TestStopParser() {
  TEST_STOP_PARSER("Stop Biryulyovo Zapadnoye:55.574371,37.651700"s,
                   "Biryulyovo Zapadnoye"s, 55.574371, 37.651700);
  TEST_STOP_PARSER(
      "Stop    Biryulyovo Zapadnoye    : 55.574371    ,    37.651700"s,
      "Biryulyovo Zapadnoye"s, 55.574371, 37.651700);
  TEST_STOP_PARSER("Stop Biryulyovo Zapadnoye: -55.574371, -37.651700"s,
                   "Biryulyovo Zapadnoye"s, -55.574371, -37.651700);
  TEST_STOP_PARSER("Stop Biryulyovo Zapadnoye: 55, 37"s,
                   "Biryulyovo Zapadnoye"s, 55.0, 37.0);
  TEST_STOP_DIS_PARSER("123m to C", {"C"s, 123});
  TEST_STOP_DIS_PARSER("123m to C e", {"C e"s, 123});
  TEST_STOP_DIS_PARSER("123m to C, 432m to D", {"C"s, 123}, {"D"s, 432});
  TEST_STOP_DIS_PARSER("   123m    to    C   ,    432m   to   D   ",
                       {"C"s, 123}, {"D"s, 432});
}

#define TEST_BUS_PARSER(str, _name, _route_type, _stops...) \
  {                                                         \
    istringstream sin{"1\n" str};                           \
    DbReader input{sin};                                    \
    const auto &cmds = input.GetAddBusCmds();               \
    ASSERT_EQUAL(cmds.size(), 1u);                          \
    const AddBusCmd &cmd = *cmds.begin();                   \
    ASSERT_EQUAL(cmd.name, _name);                          \
    ASSERT_EQUAL(cmd.route_type, _route_type);              \
    ASSERT_EQUAL(cmd.stop_names, (vector<string>{_stops})); \
  }

void TestBusParser() {
  TEST_BUS_PARSER("Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"s, "750"s,
                  RouteType::LINEAR, "Tolstopaltsevo"s, "Marushkino"s,
                  "Rasskazovka"s);
  TEST_BUS_PARSER("Bus 750:Tolstopaltsevo-Marushkino-Rasskazovka"s, "750"s,
                  RouteType::LINEAR, "Tolstopaltsevo"s, "Marushkino"s,
                  "Rasskazovka"s);
  TEST_BUS_PARSER(
      "   Bus    750   :   Tolstopaltsevo   -   Marushkino   -   Rasskazovka   "s,
      "750"s, RouteType::LINEAR, "Tolstopaltsevo"s, "Marushkino"s,
      "Rasskazovka"s);
  TEST_BUS_PARSER(
      "Bus 751: Tolstopaltsevo > Marushkino > Rasskazovka > Tolstopaltsevo"s,
      "751"s, RouteType::CIRCULAR, "Tolstopaltsevo"s, "Marushkino"s,
      "Rasskazovka"s, "Tolstopaltsevo"s);
}

void TestDbReader() {
  {
    istringstream sin{
        "3\n"
        "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
        "Stop Biryusinka: 55.581065, 37.648390\n"
        "Stop Universam: 55.587655, 37.645687\n"
        "1\n"
        "Bus 751\n"s};
    DbReader input{sin};
    auto stop_cmds = input.GetAddStopCmds();
    ASSERT_EQUAL(stop_cmds.size(), 2u);
    ASSERT_EQUAL(stop_cmds[0].name, "Biryusinka"s);
    ASSERT_EQUAL(stop_cmds[1].name, "Universam"s);
    auto bus_cmds = input.GetAddBusCmds();
    ASSERT_EQUAL(bus_cmds[0].name, "750"s);
    ASSERT_EQUAL(
        bus_cmds[0].stop_names,
        (vector<string>{"Tolstopaltsevo"s, "Marushkino"s, "Rasskazovka"s}));
    // проверяем, что `input` не "съел" весь поток ввода
    string next_line;
    getline(sin, next_line);
    ASSERT_EQUAL(next_line, "1"s);
  }
  {
    istringstream sin{
        "3\n"
        "Bus 1: A - B\n"
        "Stop A: 55.581065, 37.648390\n"
        "Stop B: 55.587655, 37.645687\n"
        "1\n"
        "Bus 1\n"s};
    DbReader input{sin};
    auto stop_cmds = input.GetAddStopCmds();
    ASSERT_EQUAL(stop_cmds.size(), 2u);
    ASSERT_EQUAL(stop_cmds[0].name, "A"s);
    ASSERT_EQUAL(stop_cmds[1].name, "B"s);
    auto bus_cmds = input.GetAddBusCmds();
    ASSERT_EQUAL(bus_cmds[0].name, "1"s);
    ASSERT_EQUAL(bus_cmds[0].stop_names, (vector<string>{"A"s, "B"s}));
    // проверяем, что `input` не "съел" весь поток ввода
    string next_line;
    getline(sin, next_line);
    ASSERT_EQUAL(next_line, "1"s);
  }
}

void TestReadDB() {
  istringstream sin{
      "3\n"
      "Bus 1: A - B\n"
      "Stop A: 55.581065, 37.648390, 123m to B\n"
      "Stop B: 55.587655, 37.645687\n"
      "1\n"
      "Bus 1\n"s};
  TransportCatalogue tc;
  ReadDB(tc, sin);
  auto bi = tc.GetBusStats("1"sv);
  ASSERT(bi.has_value());
  ASSERT_SOFT_EQUAL(bi->route_length, 123 * 2);
  // проверяем, что `input` не "съел" весь поток ввода
  string next_line;
  getline(sin, next_line);
  ASSERT_EQUAL(next_line, "1"s);
}

}  // namespace transport_catalogue::input_reader::from_char_stream::tests

using transport_catalogue::input_reader::AddStopCmd;
ostream &operator<<(ostream &os, const AddStopCmd::Distance &dis) {
  return os << '{' << dis.first << ", " << dis.second << '}';
}

void TestInputReader(TestRunner &tr) {
  {
    using namespace transport_catalogue::input_reader::from_char_stream::tests;

    RUN_TEST(tr, TestSplitNoWS);
    RUN_TEST(tr, TestStopParser);
    RUN_TEST(tr, TestBusParser);
    RUN_TEST(tr, TestDbReader);
    RUN_TEST(tr, TestReadDB);
  }
}
