#pragma once

#include <cstddef>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

/**
 * Выкидывается при ошибке парсинга JSON
 */
class ParsingError : public std::runtime_error {
 public:
  using runtime_error::runtime_error;
};

/**
 * Элемент JSON. Может хранить данные разных типов.
 * Тип элемента определяется методами `Is*`, а значения - методами `As*`.
 */
class Node : private std::variant<int, double, bool, std::string,
                                  std::nullptr_t, Array, Dict> {
  using variant::variant;

 public:
  using Value = variant;

  // Вообще мы привезли все конструкторы из `variant`,
  // но сверху ещё определяем явный конструктор для C-строк, для удобства.
  // Иначе `Node("hello")` неявно превратится в элемент целочисленного типа.
  explicit Node(const char*);

  explicit Node(Value&& val);

  bool IsInt() const;
  bool IsDouble() const;
  bool IsPureDouble() const;
  bool IsBool() const;
  bool IsString() const;
  bool IsNull() const;
  bool IsArray() const;
  bool IsMap() const;

  int AsInt() const;
  double AsDouble() const;
  bool AsBool() const;
  const std::string& AsString() const;
  const Array& AsArray() const;
  const Dict& AsMap() const;

  Array& AsArray();
  Dict& AsMap();

  bool operator==(const Node& other) const;
  bool operator!=(const Node& other) const;

  void Print(std::ostream&) const;

  const Value& GetValue() const;
  Value& GetValue();
};

class Document {
 public:
  explicit Document(Node root);

  const Node& GetRoot() const;

  bool operator==(const Document& other) const;
  bool operator!=(const Document& other) const;

 private:
  Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json

std::ostream& operator<<(std::ostream&, const json::Node&);
