#include "../transport-catalogue/json.h"
#include "json.h"

#include <limits>
#include <sstream>

#include "test_framework.h"

using namespace std;

ostream& operator<<(ostream&, const json::Node&);

namespace json::tests {

Document ParseJSON(const string &str) {
  istringstream sin { str };
  return Load(sin);
}

string Print(const Node &node) {
  ostringstream sout;
  Print(Document { node }, sout);
  return sout.str();
}

void TestNode() {
  // проверяем конструктор копирования
  string long_str(256, 'A');
  Node node(long_str);
  Node node_copy(node);
  ASSERT_EQUAL(node.AsString(), node_copy.AsString());
  ASSERT_NOT_EQUAL(node.AsString().c_str(), node_copy.AsString().c_str());
}

#define ASSERT_NOT_INT(_node) ASSERT(!_node.IsInt()); ASSERT_THROWS(_node.AsInt(), logic_error);
#define ASSERT_NOT_DOUBLE(_node) ASSERT(!_node.IsDouble()); ASSERT_THROWS(_node.AsDouble(), logic_error);
#define ASSERT_NOT_PURE_DOUBLE(_node) ASSERT(!_node.IsPureDouble());
#define ASSERT_NOT_BOOL(_node) ASSERT(!_node.IsBool()); ASSERT_THROWS(_node.AsBool(), logic_error);
#define ASSERT_NOT_STRING(_node) ASSERT(!_node.IsString()); ASSERT_THROWS(_node.AsString(), logic_error);
#define ASSERT_NOT_NULL(_node) ASSERT(!_node.IsNull());
#define ASSERT_NOT_ARRAY(_node) ASSERT(!_node.IsArray()); ASSERT_THROWS(_node.AsArray(), logic_error);
#define ASSERT_NOT_DICT(_node) ASSERT(!_node.IsMap()); ASSERT_THROWS(_node.AsMap(), logic_error);

void StripDot(string &str) {
  auto dot_pos = str.find('.');
  if (dot_pos != string::npos) {
    str.resize(dot_pos);
  }
}

void TestIntNode() {
  // проверяем правильность медотов Is* и As*
  {
    Node node { 1234 };
    ASSERT(node.IsInt());
    // int это подмножество double
    ASSERT(node.IsDouble());
    ASSERT_NOT_PURE_DOUBLE(node);
    ASSERT_NOT_BOOL(node);
    ASSERT_NOT_STRING(node);
    ASSERT_NOT_NULL(node);
    ASSERT_NOT_ARRAY(node);
    ASSERT_NOT_DICT(node);

    ASSERT_EQUAL(node.AsInt(), 1234);
    ASSERT_EQUAL(node.AsDouble(), 1234.0);
  }

  // проверяем парсинг целых чисел
  ASSERT_EQUAL(ParseJSON("1234"s).GetRoot().AsInt(), 1234);
  ASSERT_EQUAL(ParseJSON("-1234"s).GetRoot().AsInt(), -1234);
  ASSERT_EQUAL(ParseJSON("0"s).GetRoot().AsInt(), 0);
  ASSERT_EQUAL(ParseJSON("-0"s).GetRoot().AsInt(), 0);

  // числа в экспонентной записи всегда считаются double
  ASSERT(!ParseJSON("1234e3"s).GetRoot().IsInt());

  // целые числа считаются double, если записаны с точкой
  ASSERT(!ParseJSON("123.0"s).GetRoot().IsInt());

  // целые числа выходящие за пределы int парсятся как double
  {
    string json_str = to_string(
        static_cast<double>(numeric_limits<int>::max()) + 1);
    StripDot(json_str);
    ASSERT(ParseJSON(json_str).GetRoot().IsDouble());
    ASSERT(!ParseJSON(json_str).GetRoot().IsInt());
    json_str = to_string(static_cast<double>(numeric_limits<int>::min()) - 1);
    StripDot(json_str);
    ASSERT(ParseJSON(json_str).GetRoot().IsDouble());
    ASSERT(!ParseJSON(json_str).GetRoot().IsInt());
    json_str = to_string(static_cast<double>(numeric_limits<int>::max()));
    StripDot(json_str);
    ASSERT_EQUAL(ParseJSON(json_str).GetRoot().AsInt(),
                 numeric_limits<int>::max());
    json_str = to_string(static_cast<double>(numeric_limits<int>::min()));
    StripDot(json_str);
    ASSERT_EQUAL(ParseJSON(json_str).GetRoot().AsInt(),
                 numeric_limits<int>::min());
  }

  // проверяем печать
  ASSERT_EQUAL(Print(Node { 1234 }), "1234"s);
  ASSERT_EQUAL(Print(Node { -1234 }), "-1234"s);

  // проверяем оператор сравнения
  ASSERT(Node { 123 } == Node { 123 });
  ASSERT(Node { 123 } != Node { 124 });
  ASSERT(Node { 123 } != Node { 123.0 });
  ASSERT(Node { 123 } != Node { "123"s });
  ASSERT(Node { 0 } != Node { false });
  ASSERT(Node { 1 } != Node { true });
}

void TestDoubleNode() {
  // проверяем правильность медотов Is* и As*
  {
    Node node { 1234.0 };
    ASSERT(node.IsDouble());
    ASSERT(node.IsPureDouble());
    ASSERT_NOT_INT(node);
    ASSERT_NOT_BOOL(node);
    ASSERT_NOT_STRING(node);
    ASSERT_NOT_NULL(node);
    ASSERT_NOT_ARRAY(node);
    ASSERT_NOT_DICT(node);

    ASSERT_EQUAL(node.AsDouble(), 1234.0);
  }

  // проверяем парсинг чисел c плавающей точкой
  ASSERT_SOFT_EQUAL(ParseJSON("1234.5"s).GetRoot().AsDouble(), 1234.5);
  ASSERT_SOFT_EQUAL(ParseJSON("-1234.5"s).GetRoot().AsDouble(), -1234.5);
  ASSERT_EQUAL(ParseJSON("0.0"s).GetRoot(), Node { 0.0 });
  ASSERT_EQUAL(ParseJSON("-0.0"s).GetRoot(), Node { -0.0 });
  ASSERT_SOFT_EQUAL(ParseJSON(".1"s).GetRoot().AsDouble(), 0.1);
  ASSERT_SOFT_EQUAL(ParseJSON("-.1"s).GetRoot().AsDouble(), -0.1);
  ASSERT_SOFT_EQUAL(ParseJSON("1.2e3"s).GetRoot().AsDouble(), 1.2e3);
  ASSERT_SOFT_EQUAL(ParseJSON("-1.2e3"s).GetRoot().AsDouble(), -1.2e3)
  ASSERT_SOFT_EQUAL(ParseJSON("1.2E3"s).GetRoot().AsDouble(), 1.2e3);
  ASSERT_SOFT_EQUAL(ParseJSON("-1.2E3"s).GetRoot().AsDouble(), -1.2e3)
  ASSERT_SOFT_EQUAL(ParseJSON("1.2e-3"s).GetRoot().AsDouble(), 1.2e-3);
  ASSERT_SOFT_EQUAL(ParseJSON("-1.2e-3"s).GetRoot().AsDouble(), -1.2e-3)
  ASSERT_SOFT_EQUAL(ParseJSON("1.2e+3"s).GetRoot().AsDouble(), 1.2e+3);
  ASSERT_SOFT_EQUAL(ParseJSON("-1.2e+3"s).GetRoot().AsDouble(), -1.2e+3)
  ASSERT_SOFT_EQUAL(ParseJSON("1.2e0"s).GetRoot().AsDouble(), 1.2);

  // целые числа выходящие за пределы int парсятся как double
  {
    string json_str = to_string(
        static_cast<double>(numeric_limits<int>::max()) + 1);
    StripDot(json_str);
    ASSERT_SOFT_EQUAL(ParseJSON(json_str).GetRoot().AsDouble(),
                      static_cast<double>(numeric_limits<int>::max()) + 1);
    json_str = to_string(static_cast<double>(numeric_limits<int>::min()) - 1);
    StripDot(json_str);
    ASSERT_SOFT_EQUAL(ParseJSON(json_str).GetRoot().AsDouble(),
                      static_cast<double>(numeric_limits<int>::min()) - 1);
  }

  // проверяем печать
  ASSERT_EQUAL(Print(Node { 123.0 }), "123"s);
  ASSERT_EQUAL(Print(Node { -123.0 }), "-123"s);
  ASSERT_EQUAL(Print(Node { 123.4 }), "123.4"s);
  ASSERT_EQUAL(Print(Node { -123.4 }), "-123.4"s);

  // проверяем оператор сравнения
  ASSERT(Node { 123.4 } == Node { 123.4 });
  ASSERT(Node { 123.0 } != Node { 123 });
  ASSERT(Node { 123.0 } != Node { "123.0"s });
  ASSERT(Node { 0.0 } != Node { false });
  ASSERT(Node { 1.0 } != Node { true });
}

void TestBoolNode() {
  // проверяем правильность медотов Is* и As*
  {
    Node node { true };
    ASSERT(node.IsBool());
    ASSERT_NOT_INT(node);
    ASSERT_NOT_DOUBLE(node);
    ASSERT_NOT_PURE_DOUBLE(node);
    ASSERT_NOT_STRING(node);
    ASSERT_NOT_NULL(node);
    ASSERT_NOT_ARRAY(node);
    ASSERT_NOT_DICT(node);

    ASSERT_EQUAL(node.AsBool(), true);
  }

  // проверяем парсинг
  ASSERT_EQUAL(ParseJSON("true"s).GetRoot().AsBool(), true);
  ASSERT_EQUAL(ParseJSON("false"s).GetRoot().AsBool(), false);

  // проверяем печать
  ASSERT_EQUAL(Print(Node { true }), "true"s);
  ASSERT_EQUAL(Print(Node { false }), "false"s);

  // проверяем оператор сравнения
  ASSERT(Node { true } == Node { true });
  ASSERT(Node { true } != Node { false });
  ASSERT(Node { true } != Node { "true"s });
  ASSERT(Node { true } != Node { 1 });
  ASSERT(Node { true } != Node { 1.0 });
}

void TestStringNode() {
  // проверяем правильность медотов Is* и As*
  {
    Node node { "hello world"s };
    ASSERT(node.IsString());
    ASSERT_NOT_INT(node);
    ASSERT_NOT_DOUBLE(node);
    ASSERT_NOT_PURE_DOUBLE(node);
    ASSERT_NOT_BOOL(node);
    ASSERT_NOT_NULL(node);
    ASSERT_NOT_ARRAY(node);
    ASSERT_NOT_DICT(node);

    ASSERT_EQUAL(node.AsString(), "hello world"s);
  }

  // если строка содержит число, это всё равно строка
  {
    Node node { "1234"s };
    ASSERT(node.IsString());
    ASSERT_NOT_DOUBLE(node);
  }
  {
    Node node { "1234.0"s };
    ASSERT(node.IsString());
    ASSERT_NOT_DOUBLE(node);
  }

  // строка может быть пустой, а с кавычками мы ничего не делаем
  ASSERT_EQUAL(Node { ""s }.AsString(), ""s);
  ASSERT_EQUAL(Node { "\""s }.AsString(), "\""s);
  ASSERT_EQUAL(Node { "\"\n\""s }.AsString(), "\"\n\""s);

  // проверяем парсинг escape последовательностей
  ASSERT_EQUAL(ParseJSON(R"("\na\rb\tc\\\"")"s).GetRoot().AsString(),
               "\na\rb\tc\\\""s);

  // проверяем печать escape последовательностей
  ASSERT_EQUAL(Print(Node { "\na\rb\tc\\\""s }), "\"\\na\\rb\tc\\\\\\\"\""s);

  // проверяем оператор сравнения
  ASSERT(Node { "hello"s } == Node { "hello"s });
  ASSERT(Node { "hello"s } == Node { "hello" });
  ASSERT(Node { "hello"s } != Node { "world"s });
}

void TestNullNode() {
  // проверяем правильность медотов Is* и As*
  {
    Node node { nullptr };
    ASSERT(node.IsNull());
    ASSERT_NOT_INT(node);
    ASSERT_NOT_DOUBLE(node);
    ASSERT_NOT_PURE_DOUBLE(node);
    ASSERT_NOT_BOOL(node);
    ASSERT_NOT_STRING(node);
    ASSERT_NOT_ARRAY(node);
    ASSERT_NOT_DICT(node);
  }

  // проверяем парсинг
  ASSERT(ParseJSON("null"s).GetRoot().IsNull());

  // проверяем печать
  ASSERT_EQUAL(Print(Node { nullptr }), "null"s);

  // проверяем оператор сравнения
  ASSERT(Node { nullptr } == Node { nullptr });
  ASSERT(Node { nullptr } != Node { false });
  ASSERT(Node { nullptr } != Node { "null"s });
  ASSERT(Node { nullptr } != Node { 0 });
  ASSERT(Node { nullptr } != Node { 0.0 });
}

void TestArrayNode() {
  // проверяем правильность медотов Is* и As*
  {
    Node node { Array { } };
    ASSERT(node.IsArray());
    ASSERT_NOT_INT(node);
    ASSERT_NOT_DOUBLE(node);
    ASSERT_NOT_PURE_DOUBLE(node);
    ASSERT_NOT_BOOL(node);
    ASSERT_NOT_STRING(node);
    ASSERT_NOT_NULL(node);
    ASSERT_NOT_DICT(node);
  }

  // проверяем парсинг
  ASSERT_EQUAL(ParseJSON("[]"s).GetRoot(), Node { Array { } });
  ASSERT_EQUAL(ParseJSON("[ ]"s).GetRoot(), Node { Array { } });
  ASSERT_EQUAL(ParseJSON("[ \t\r\n ]"s).GetRoot(), Node { Array { } });
  ASSERT_EQUAL(ParseJSON("[1]"s).GetRoot(), Node { Array { 1 } });
  ASSERT_EQUAL(ParseJSON("[1,2]"s).GetRoot(), (Node { Array { 1, 2 } }));
  ASSERT_EQUAL(ParseJSON("[ 1 , 2 ]"s).GetRoot(), (Node { Array { 1, 2 } }));
  ASSERT_EQUAL(ParseJSON("[ \t\r\n 1 \t\r\n , \t\r\n 2 \t\r\n ]"s).GetRoot(),
               (Node { Array { 1, 2 } }));
  ASSERT_EQUAL(ParseJSON("[true,false]"s).GetRoot(), (Node {
                   Array { true, false } }));
  ASSERT_EQUAL(ParseJSON("[true]"s).GetRoot(), Node { Array { true } });
  ASSERT_EQUAL(ParseJSON("[null]"s).GetRoot(), Node { Array { nullptr } });
  ASSERT_EQUAL(ParseJSON("[true,null]"s).GetRoot(), (Node { Array { true,
      nullptr } }));
  ASSERT_EQUAL(ParseJSON("[\"hello\"]"s).GetRoot(), Node { Array { "hello"s } });
  ASSERT_EQUAL(ParseJSON("[\"hello\", \"world\"]"s).GetRoot(), (Node { Array {
      "hello"s, "world"s } }));
  ASSERT_EQUAL(ParseJSON("[+123]"s).GetRoot(), Node { Array { 123 } });
  ASSERT_EQUAL(ParseJSON("[-123]"s).GetRoot(), Node { Array { -123 } });
  ASSERT_EQUAL(ParseJSON("[.123]"s).GetRoot(), Node { Array { 0.123 } });
  ASSERT_EQUAL(ParseJSON("[-.123]"s).GetRoot(), Node { Array { -0.123 } });
  ASSERT_EQUAL(ParseJSON("[1.]"s).GetRoot(), Node { Array { 1.0 } });
  ASSERT_EQUAL(ParseJSON("[1., 123]"s).GetRoot(), (Node { Array { 1.0, 123 } }));
  ASSERT_EQUAL(ParseJSON("[0,+123]"s).GetRoot(), (Node { Array { 0, 123 } }));
  ASSERT_EQUAL(ParseJSON("[0,-123]"s).GetRoot(), (Node { Array { 0, -123 } }));
  ASSERT_EQUAL(ParseJSON("[0,.123]"s).GetRoot(), (Node { Array { 0, 0.123 } }));
  ASSERT_EQUAL(ParseJSON("[0,-.123]"s).GetRoot(),
               (Node { Array { 0, -0.123 } }));

  string complex_node_str =
      "[1,2.3,true,\"hello\",null,[1,2,3],{\"foo\":\"bar\"}]"s;
  Node complex_node { Array { 1, 2.3, true, "hello"s, nullptr,
      Array { 1, 2, 3 }, Dict { { "foo"s, "bar"s } } } };
  ASSERT_EQUAL(ParseJSON(complex_node_str).GetRoot(), complex_node);

  // проверяем печать
  ASSERT_EQUAL(Print(Node { Array { } }), "[]"s);
  ASSERT_EQUAL(Print(Node { Array { 1 } }), "[1]"s);
  ASSERT_EQUAL(Print(Node { Array { 1, 2 } }), "[1,2]"s);
  ASSERT_EQUAL(Print(complex_node), complex_node_str);

  // проверяем оператор сравнения
  ASSERT(Node { Array { } } == Node { Array { } });
  ASSERT(Node { Array { } } != Node { Array { 0 } });
  ASSERT(Node { Array { 0 } } == Node { Array { 0 } });
  ASSERT(Node { Array { 0 } } != Node { Array { 1 } });
  ASSERT(Node { Array { 0 } } != (Node { Array { 0, 1 } }));
  ASSERT(complex_node == complex_node);
}

void TestDictNode() {
  // проверяем правильность медотов Is* и As*
  {
    Node node { Dict { } };
    ASSERT(node.IsMap());
    ASSERT_NOT_INT(node);
    ASSERT_NOT_DOUBLE(node);
    ASSERT_NOT_PURE_DOUBLE(node);
    ASSERT_NOT_BOOL(node);
    ASSERT_NOT_STRING(node);
    ASSERT_NOT_NULL(node);
    ASSERT_NOT_ARRAY(node);
  }

  // проверяем парсинг
  ASSERT_EQUAL(ParseJSON("{}"s).GetRoot(), Node { Dict { } });
  ASSERT_EQUAL(ParseJSON("{ }"s).GetRoot(), Node { Dict { } });
  ASSERT_EQUAL(ParseJSON("{ \t\r\n }"s).GetRoot(), Node { Dict { } });
  ASSERT_EQUAL(ParseJSON("{\"hello\":1}"s).GetRoot(), (Node { Dict { { "hello"s,
      1 } } }));
  ASSERT_EQUAL(ParseJSON("{\"hello\" : 1}"s).GetRoot(), (Node { Dict { {
      "hello"s, 1 } } }));
  ASSERT_EQUAL(
      ParseJSON("{ \t\r\n \"hello\" \t\r\n : \t\r\n 1 \t\r\n }"s).GetRoot(),
      (Node { Dict { { "hello"s, 1 } } }));
  ASSERT_EQUAL(ParseJSON("{\"hello\":1,\"world\":2}"s).GetRoot(), (Node { Dict {
      { "hello"s, 1 }, { "world"s, 2 } } }));
  ASSERT_EQUAL(ParseJSON("{\"hello\":-1}"s).GetRoot(), (Node { Dict { {
      "hello"s, -1 } } }));
  ASSERT_EQUAL(ParseJSON("{\"hello\":+1}"s).GetRoot(), (Node { Dict { {
      "hello"s, 1 } } }));
  ASSERT_EQUAL(ParseJSON("{\"hello\":.1}"s).GetRoot(), (Node { Dict { {
      "hello"s, 0.1 } } }));
  ASSERT_EQUAL(ParseJSON("{\"hello\":1., \"world\":2}"s).GetRoot(), (Node {
                   Dict { { "hello"s, 1.0 }, { "world"s, 2 } } }));

  string complex_node_str =
      "{\"key1\":1,\"key2\":2.3,\"key3\":true,\"key4\":\"hello\",\"key5\":null,\"key6\":[1,2,3],\"key7\":{\"foo\":\"bar\"}}"s;
  Node complex_node { Dict { { "key1"s, 1 }, { "key2"s, 2.3 },
      { "key3"s, true }, { "key4"s, "hello"s }, { "key5"s, nullptr }, { "key6"s,
          Array { 1, 2, 3 } }, { "key7"s, Dict { { "foo"s, "bar"s } } } } };
  ASSERT_EQUAL(ParseJSON(complex_node_str).GetRoot(), complex_node);

  // проверяем печать
  ASSERT_EQUAL(Print(Node { Dict { } }), "{}"s);
  ASSERT_EQUAL(Print(Node { Dict { { "one"s, 1 } } }), "{\"one\":1}"s);
  ASSERT_EQUAL(Print(Node { Dict { { "one"s, 1 }, { "two"s, 2 } } }),
               "{\"one\":1,\"two\":2}"s);
  ASSERT_EQUAL(Print(complex_node), complex_node_str);

  // проверяем оператор сравнения
  ASSERT(Node { Dict { } } == Node { Dict { } });
  ASSERT(
      ParseJSON(R"({"hello":1,"world":2})"s).GetRoot()
          == ParseJSON(R"({"world":2, "hello":1})"s).GetRoot())
  ASSERT(
      ParseJSON(R"({"hello":1,"world":2})"s).GetRoot()
          != ParseJSON(R"({"world":3, "hello":1})"s).GetRoot())
  ASSERT(complex_node == complex_node);
}

#define CANT_PARSE(_str) ASSERT_THROWS(ParseJSON(_str), ParsingError)

void TestParsingErrors() {
  CANT_PARSE("-"s);
  CANT_PARSE("+"s);
  CANT_PARSE("-."s);
  CANT_PARSE("+."s);
  CANT_PARSE("1.2ea"s)
  CANT_PARSE("tru"s);
  CANT_PARSE("True"s);
  CANT_PARSE("TRUE"s);
  CANT_PARSE("tRUE"s);
  CANT_PARSE("fals"s);
  CANT_PARSE("fALSE"s);
  CANT_PARSE("FALSE"s);
  CANT_PARSE("False"s);
  CANT_PARSE("'helo'"s);
  CANT_PARSE("\"helo"s);
  CANT_PARSE(R"("helo\")"s);
  CANT_PARSE(R"("helo\\\")"s);
  CANT_PARSE("nul"s);
  CANT_PARSE("nULL"s);
  CANT_PARSE("Null"s);
  CANT_PARSE("NULL"s);
  CANT_PARSE("["s);
  CANT_PARSE("]"s);
  CANT_PARSE("[0,]"s);
  CANT_PARSE("[,0]"s);
  CANT_PARSE("{"s);
  CANT_PARSE("}"s);
  CANT_PARSE(R"({"hello:1})"s);
  CANT_PARSE(R"({"hello\":1})"s);
  CANT_PARSE(R"({"hello\\\":1})"s);
  CANT_PARSE(R"({"hello"})"s);
  CANT_PARSE(R"({"hello":})"s);
  CANT_PARSE(R"({"hello":1,})"s);
  CANT_PARSE(R"({,"hello":1})"s);
}

}  // namespace json::tests

ostream& operator<<(ostream &out, const json::Node &node) {
  throw runtime_error("not implemented"s);
}

void TestJSON(TestRunner &tr) {
  using namespace json::tests;
  RUN_TEST(tr, TestNode);
  RUN_TEST(tr, TestIntNode);
  RUN_TEST(tr, TestDoubleNode);
  RUN_TEST(tr, TestBoolNode);
  RUN_TEST(tr, TestStringNode);
  RUN_TEST(tr, TestNullNode);
  RUN_TEST(tr, TestArrayNode);
  RUN_TEST(tr, TestDictNode);
  RUN_TEST(tr, TestParsingErrors);
}
