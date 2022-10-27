#include "json_builder.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>

using namespace std;

namespace json {

Builder& Builder::Key(string key) {
  if (stack_.empty() || !stack_.back().IsMap() || !expect_key_) {
    throw logic_error("unexpected key"s);
  }
  key_stack_.emplace_back(move(key));
  expect_key_ = false;
  return *this;
}

Builder& Builder::Value(Node::Value value) {
  if (stack_.empty()) {
    stack_.emplace_back(move(value));
    finished_ = true;
    return *this;
  }
  auto &cur_node = stack_.back();
  if (cur_node.IsArray()) {
    cur_node.AsArray().emplace_back(move(value));
    return *this;
  }
  if (cur_node.IsMap() && !expect_key_) {
    cur_node.AsMap().emplace(make_pair(move(key_stack_.back()), value));
    key_stack_.pop_back();
    expect_key_ = true;
    return *this;
  }
  throw logic_error("unexpected value"s);
}

Builder& Builder::StartDict() {
  if (finished_) {
    throw logic_error("node is finished"s);
  }
  if (expect_key_) {
    throw logic_error("expected key"s);
  }
  stack_.push_back(Dict { });
  expect_key_ = true;
  return *this;
}

Builder& Builder::EndDict() {
  if (finished_) {
    throw logic_error("node is finished"s);
  }
  if (stack_.empty() || !stack_.back().IsMap()) {
    throw logic_error("trying to end a non-dict"s);
  }
  if (!expect_key_) {
    throw logic_error("expected key or end of dict"s);
  }
  expect_key_ = false;
  auto val = move(stack_.back());
  stack_.pop_back();
  return Value(move(val.GetValue()));
}

Builder& Builder::StartArray() {
  if (finished_) {
    throw logic_error("node is finished"s);
  }
  if (expect_key_) {
    throw logic_error("expected key"s);
  }
  stack_.push_back(Array { });
  return *this;
}

Builder& Builder::EndArray() {
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

Node Builder::Build() {
  if (!finished_) {
    throw logic_error("node is not finished"s);
  }
  auto val = move(stack_.back());
  stack_.pop_back();
  return val;
}

}  // namespace json
