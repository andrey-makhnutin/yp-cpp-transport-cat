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
class Node {
  using variant_t = std::variant<int, double, bool, std::string, std::nullptr_t, Array, Dict>;
 public:
  Node() = default;
  Node(int);
  Node(double);
  Node(bool);
  Node(std::string);
  Node(const char*);
  Node(nullptr_t);
  Node(Array);
  Node(Dict);

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

  bool operator==(const Node &other) const;
  bool operator!=(const Node &other) const;

  void Print(std::ostream&) const;
 private:
  // не могу унаследоваться от std::variant, не получается завести это под gcc 9.4
  variant_t value_ = nullptr;
};

class Document {
 public:
  explicit Document(Node root);

  const Node& GetRoot() const;

  bool operator==(const Document &other) const;
  bool operator!=(const Document &other) const;
 private:
  Node root_;
};

Document Load(std::istream &input);

void Print(const Document &doc, std::ostream &output);

}  // namespace json
