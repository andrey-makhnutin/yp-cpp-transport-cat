#include "../transport-catalogue/json_builder.h"
#include "json_builder.h"

#include <stdexcept>

#include "test_framework.h"

using namespace std;

namespace json::tests {

void TestPrimitives() {
  ASSERT_EQUAL(json::Builder { }.Value(123).Build(), Node { 123 });
  ASSERT_EQUAL(json::Builder { }.Value(1.23).Build(), Node { 1.23 });
  ASSERT_EQUAL(json::Builder { }.Value(true).Build(), Node { true });
  ASSERT_EQUAL(json::Builder { }.Value("hello"s).Build(), Node { "hello"s });
}

void TestDict() {
  ASSERT_EQUAL(
//@formatter:off
      json::Builder {}
        .StartDict()
          .Key("test"s).Value(123)
        .EndDict()
      .Build(),
//@formatter:on
      (Dict { {"test"s, 123}}));
  ASSERT_EQUAL(
//@formatter:off
      json::Builder { }.StartDict().Key("test"s).StartArray().Value(123)
          .EndArray().EndDict().Build(),
//@formatter:on
      (Dict { { "test"s, Array { 123 } } }));
  ASSERT_EQUAL(
      json::Builder { }.StartDict().Key("test"s).StartDict().Key("test2"s).Value(
          124).EndDict().EndDict().Build(),
      (Dict { { "test"s, Dict { { "test2"s, 124 } } } }));
}

void TestArray() {
  ASSERT_EQUAL(json::Builder { }.StartArray().Value(123).EndArray().Build(),
               (Array { 123 }));
  ASSERT_EQUAL(
      json::Builder { }.StartArray().StartArray().Value(123).EndArray().EndArray()
          .Build(),
      (Array { Array { 123 } }));
  ASSERT_EQUAL(
      json::Builder { }.StartArray().StartArray().Value(123).EndArray().Value(
          124).EndArray().Build(),
      (Array { Array { 123 }, 124 }));
  ASSERT_EQUAL(
      json::Builder { }.StartArray().StartArray().Value(123).EndArray().Value(
          124).StartDict().Key("test"s).Value(125).EndDict().EndArray().Build(),
      (Array { Array { 123 }, 124, Dict { { "test"s, 125 } } }));
  ASSERT_EQUAL(
      json::Builder { }.StartArray().Value(124).StartDict().Key("test"s).Value(
          125).EndDict().EndArray().Build(),
      (Array { 124, Dict { { "test"s, 125 } } }));
  ASSERT_EQUAL(
      json::Builder { }.StartArray().StartDict().Key("test"s).Value(125).EndDict()
          .EndArray().Build(),
      (Array { Dict { { "test"s, 125 } } }));
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

}  // namespace json::tests

void TestJsonBuilder(TestRunner &tr) {
  using namespace json::tests;

  RUN_TEST(tr, TestPrimitives);
  RUN_TEST(tr, TestDict);
  RUN_TEST(tr, TestArray);
  RUN_TEST(tr, TestInvalidUsage);
}
