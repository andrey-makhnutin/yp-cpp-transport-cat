#include "../transport-catalogue/json_builder.h"
#include "json_builder.h"

#include <sstream>
#include <stdexcept>

#include "test_framework.h"

using namespace std;

namespace json::tests {

#define ASSERT_JSON_EQUAL(a, b) { \
  stringstream sout;              \
  json::Node node = (a);          \
  node.Print(sout);               \
  ASSERT_EQUAL(sout.str(), (b))   \
}

void TestPrimitives() {
  ASSERT_JSON_EQUAL(json::Builder { }.Value(123).Build(), "123"s);
  ASSERT_JSON_EQUAL(json::Builder { }.Value(1.23).Build(), "1.23"s);
  ASSERT_JSON_EQUAL(json::Builder { }.Value(true).Build(), "true"s);
  ASSERT_JSON_EQUAL(json::Builder { }.Value("hello"s).Build(), R"("hello")"s);
}

void TestDict() {
  ASSERT_JSON_EQUAL(
//@formatter:off
      json::Builder { }
        .StartDict()
          .Key("test"s).Value(123)
          .Key("test2"s).Value(124)
        .EndDict()
      .Build(),
//@formatter:on
      R"({"test":123,"test2":124})"s);
  ASSERT_JSON_EQUAL(
//@formatter:off
      json::Builder { }
        .StartDict()
          .Key("test"s).StartArray()
            .Value(123)
          .EndArray()
          .Key("test2").Value(124)
        .EndDict()
      .Build(),
//@formatter:on
      R"({"test":[123],"test2":124})"s);
  ASSERT_JSON_EQUAL(
//@formatter:off
      json::Builder { }
        .StartDict()
          .Key("test"s).StartDict()
            .Key("test2"s).Value(124)
          .EndDict()
          .Key("test3"s).Value(125)
        .EndDict()
      .Build(),
//@formatter:on
      R"({"test":{"test2":124},"test3":125})"s);
}

void TestArray() {
  ASSERT_JSON_EQUAL(
      json::Builder { }.StartArray().Value(123).EndArray().Build(), R"([123])"s);
  ASSERT_JSON_EQUAL(
      json::Builder { }.StartArray().StartArray().Value(123).EndArray().EndArray()
          .Build(),
      R"([[123]])"s);
  ASSERT_JSON_EQUAL(
      json::Builder { }.StartArray().StartArray().Value(123).EndArray().Value(
          124).EndArray().Build(),
      R"([[123],124])"s);
  ASSERT_JSON_EQUAL(
      json::Builder { }.StartArray().StartArray().Value(123).EndArray().Value(
          124).StartDict().Key("test"s).Value(125).EndDict().EndArray().Build(),
      R"([[123],124,{"test":125}])"s);
  ASSERT_JSON_EQUAL(
      json::Builder { }.StartArray().Value(124).StartDict().Key("test"s).Value(
          125).EndDict().EndArray().Build(),
      R"([124,{"test":125}])"s);
  ASSERT_JSON_EQUAL(
      json::Builder { }.StartArray().StartDict().Key("test"s).Value(125).EndDict()
          .EndArray().Build(),
      R"([{"test":125}])"s);
}

void TestInvalidUsage() {
  ASSERT_THROWS(json::Builder { }.Key("test"s), logic_error);
  ASSERT_THROWS(json::Builder { }.Value(123).Key("test"s), logic_error);

  ASSERT_THROWS(json::Builder { }.Value(123).Value(123), logic_error);

  ASSERT_THROWS(json::Builder { }.Value(123).StartDict(), logic_error);

  ASSERT_THROWS(json::Builder { }.EndDict(), logic_error);
  ASSERT_THROWS(json::Builder { }.Value(123).EndDict(), logic_error);

  ASSERT_THROWS(json::Builder { }.Value(123).StartArray(), logic_error);

  ASSERT_THROWS(json::Builder { }.EndArray(), logic_error);
  ASSERT_THROWS(json::Builder { }.Value(123).EndArray(), logic_error);

  ASSERT_THROWS(json::Builder { }.Build(), logic_error);
  ASSERT_THROWS(json::Builder { }.StartArray().StartArray().EndArray().Build(),
                logic_error);
}

void TestPartial() {
  {
    auto builder = json::Builder { };
    builder = builder.StartArray().Value(1).Value(2).EndArray();
    ASSERT_JSON_EQUAL(builder.Build(), "[1,2]"s);
  }
  {
    auto arr_builder = json::Builder { }.StartArray();
    arr_builder = move(arr_builder.Value(1).Value(2));
    ASSERT_JSON_EQUAL(arr_builder.EndArray().Build(), "[1,2]"s);
  }
  {
    auto arr_builder = json::Builder { }.StartArray();
    arr_builder.Value(1);
    arr_builder.Value(2);
    ASSERT_JSON_EQUAL(arr_builder.EndArray().Build(), "[1,2]"s);
  }
  {
    auto dict_builder = json::Builder { }.StartDict();
    dict_builder = dict_builder.Key("hello"s).Value(123);
    dict_builder = dict_builder.Key("world"s).Value(124);
    ASSERT_JSON_EQUAL(dict_builder.EndDict().Build(),
                      R"({"hello":123,"world":124})"s);
  }
  {
    auto dict_part1 = json::Builder { }.StartDict();
    auto dict_part2 = dict_part1.Key("hello"s).Value(123);
    auto dict_part3 = dict_part2.Key("world"s).Value(124);
    ASSERT_JSON_EQUAL(dict_part3.EndDict().Build(),
                      R"({"hello":123,"world":124})"s);
  }
}

}  // namespace json::tests

void TestJsonBuilder(TestRunner &tr) {
  using namespace json::tests;

  RUN_TEST(tr, TestPrimitives);
  RUN_TEST(tr, TestDict);
  RUN_TEST(tr, TestArray);
  RUN_TEST(tr, TestInvalidUsage);
  RUN_TEST(tr, TestPartial);
}
