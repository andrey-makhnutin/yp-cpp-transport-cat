#include "json_builder.h"

#include <map>
#include <stdexcept>
#include <utility>

using namespace std;

namespace json {

Builder::Builder(Builder&& other)
    : stack_(move(other.stack_)), key_stack_(move(other.key_stack_)) {
  if (other.moved_out_of_) {
    throw logic_error("double move"s);
  }
  finished_ = other.finished_;
  expect_key_ = other.expect_key_;
  moved_out_of_ = exchange(other.moved_out_of_, true);
}

Builder& Builder::operator=(Builder&& lhs) {
  if (&lhs == this) {
    return *this;
  }
  Builder other(move(lhs));
  Swap(other);
  return *this;
}

void Builder::Swap(Builder& other) {
  swap(stack_, other.stack_);
  swap(key_stack_, other.key_stack_);
  swap(finished_, other.finished_);
  swap(expect_key_, other.expect_key_);
  swap(moved_out_of_, other.moved_out_of_);
}

/**
 * Указать название ключа в собираемом JSON словаре.
 */
Builder& Builder::Key(string key) {
  if (moved_out_of_) {
    throw logic_error("using a moved-out-of builder"s);
  }
  if (stack_.empty()) {
    throw logic_error("unexpected key: empty node"s);
  }
  if (!stack_.back().IsMap()) {
    throw logic_error("unexpected key: not a map"s);
  }
  if (!expect_key_) {
    throw logic_error("unexpected key"s);
  }
  key_stack_.emplace_back(move(key));
  expect_key_ = false;
  return *this;
}

/**
 * В зависимости от этапа: либо указать значение для примитивного JSON
 * (если это первый вызов в конструкторе),
 * либо значение собираемого словаря или массива.
 */
Builder& Builder::Value(Node::Value value) {
  if (moved_out_of_) {
    throw logic_error("using a moved-out-of builder"s);
  }

  // если стек пустой, значит мы не собираем сейчас какой-то словарь или массив,
  // и переданное значение будет финальным значением JSON
  if (stack_.empty()) {
    stack_.emplace_back(move(value));
    finished_ = true;
    return *this;
  }

  auto& cur_node = stack_.back();
  if (cur_node.IsArray()) {
    cur_node.AsArray().emplace_back(move(value));
    return *this;
  }

  if (cur_node.IsMap() && !expect_key_) {
    cur_node.AsMap().emplace(make_pair(move(key_stack_.back()), move(value)));
    key_stack_.pop_back();
    expect_key_ = true;
    return *this;
  }
  throw logic_error("unexpected value"s);
}

/**
 * Начать построение JSON словаря.
 */
DictKeyPart Builder::StartDict() {
  if (moved_out_of_) {
    throw logic_error("using a moved-out-of builder"s);
  }
  if (finished_) {
    throw logic_error("node is finished"s);
  }
  if (expect_key_) {
    throw logic_error("expected key, start of dict found"s);
  }
  stack_.push_back(Dict{});
  expect_key_ = true;
  return {move(*this)};
}

/**
 * Закончить построение JSON словаря.
 */
Builder& Builder::EndDict() {
  if (moved_out_of_) {
    throw logic_error("using a moved-out-of builder"s);
  }
  if (finished_) {
    throw logic_error("node is finished"s);
  }
  if (stack_.empty() || !stack_.back().IsMap()) {
    throw logic_error("trying to end a non-dict"s);
  }
  if (!expect_key_) {
    throw logic_error("expected a value, end of dict found"s);
  }
  expect_key_ = false;
  auto val = move(stack_.back());
  stack_.pop_back();
  return Value(move(val.GetValue()));
}

/**
 * Начать построение JSON массива.
 */
ArrayPart Builder::StartArray() {
  if (moved_out_of_) {
    throw logic_error("using a moved-out-of builder"s);
  }
  if (finished_) {
    throw logic_error("node is finished"s);
  }
  if (expect_key_) {
    throw logic_error("expected key"s);
  }
  stack_.push_back(Array{});
  return {move(*this)};
}

/**
 * Закончить построение JSON массива.
 */
Builder& Builder::EndArray() {
  if (moved_out_of_) {
    throw logic_error("using a moved-out-of builder"s);
  }
  if (finished_) {
    throw logic_error("node is finished"s);
  }
  if (stack_.empty() || !stack_.back().IsArray()) {
    throw logic_error("trying to end a non-array"s);
  }
  auto val = move(stack_.back());
  stack_.pop_back();
  return Value(move(val.GetValue()));
}

/**
 * Закончить сборку JSON значения, возвращает готовую JSON ноду.
 */
Node Builder::Build() {
  if (moved_out_of_) {
    throw logic_error("using a moved-out-of builder"s);
  }
  if (stack_.empty()) {
    throw logic_error("builder is empty");
  }
  if (!finished_) {
    if (stack_.back().IsMap()) {
      throw logic_error("node is not finished: dict is not ended"s);
    } else if (stack_.back().IsArray()) {
      throw logic_error("node is not finished: array is not ended"s);
    }
    throw logic_error("node is not finished"s);
  }
  auto val = move(stack_.back());
  stack_.pop_back();
  return val;
}

/**
 * Указать название ключа в собираемом JSON словаре.
 */
DictValuePart DictKeyPart::Key(std::string k) {
  builder_.Key(move(k));
  return {move(builder_)};
}

/**
 * Закончить построение JSON словаря.
 */
Builder DictKeyPart::EndDict() {
  builder_.EndDict();
  return move(builder_);
}

/**
 * Указать значение в собираемом JSON словаре.
 */
DictKeyPart DictValuePart::Value(Node::Value value) {
  builder_.Value(move(value));
  return {move(builder_)};
}

/**
 * Начать построение JSON словаря.
 */
DictKeyPart DictValuePart::StartDict() { return builder_.StartDict(); }

/**
 * Начать построение JSON массива.
 *
 * Объект, возвращаемый этим методом, можно сохранить в переменную и
 * вызывать на ней метод `Value()` несколько раз, в отличие от других
 * временных объектов-конструкторов. См. документацию к классу `Builder`.
 */
ArrayPart DictValuePart::StartArray() { return builder_.StartArray(); }

/**
 * Указать значение в собираемом JSON массиве.
 *
 * В отличие от других методов конструктора, этот не перемещает значение
 * текущего объекта-конструктора в возвращаемом значении, поэтому переменная,
 * у которой был вызван метод, остаётся в рабочем состоянии.
 * См. документацию к классу `Builder`.
 */
ArrayPart& ArrayPart::Value(Node::Value value) {
  builder_.Value(move(value));
  return *this;
}

/**
 * Начать построение JSON словаря.
 */
DictKeyPart ArrayPart::StartDict() { return builder_.StartDict(); }

/**
 * Начать построение JSON массива.
 */
ArrayPart ArrayPart::StartArray() { return builder_.StartArray(); }

/**
 * Закончить построение JSON массива.
 */
Builder ArrayPart::EndArray() {
  builder_.EndArray();
  return move(builder_);
}

}  // namespace json
