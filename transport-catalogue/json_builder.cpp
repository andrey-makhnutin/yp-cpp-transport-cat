#include "json_builder.h"

#include <map>
#include <stdexcept>
#include <utility>

using namespace std;

namespace json {

Builder::Builder(Builder &&other) {
  if (other.moved_out_of_) {
    throw std::logic_error("double move"s);
  }
  std::swap(stack_, other.stack_);
  std::swap(key_stack_, other.key_stack_);
  finished_ = other.finished_;
  expect_key_ = other.expect_key_;
  other.moved_out_of_ = true;
}

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

Builder& Builder::Value(Node::Value value) {
  if (moved_out_of_) {
    throw logic_error("using a moved-out-of builder"s);
  }
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
  stack_.push_back(Dict { });
  expect_key_ = true;
  return {move(*this)};
}

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
  stack_.push_back(Array { });
  return {move(*this)};
}

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

DictValuePart DictKeyPart::Key(std::string k) {
  builder_.Key(move(k));
  return {move(builder_)};
}

Builder DictKeyPart::EndDict() {
  builder_.EndDict();
  return move(builder_);
}

DictKeyPart DictValuePart::Value(Node::Value value) {
  builder_.Value(move(value));
  return {move(builder_)};
}

DictKeyPart DictValuePart::StartDict() {
  return builder_.StartDict();
}

ArrayPart DictValuePart::StartArray() {
  return builder_.StartArray();
}

ArrayPart& ArrayPart::Value(Node::Value value) {
  builder_.Value(move(value));
  return *this;
}

DictKeyPart ArrayPart::StartDict() {
  return builder_.StartDict();
}

ArrayPart ArrayPart::StartArray() {
  return builder_.StartArray();
}

Builder ArrayPart::EndArray() {
  builder_.EndArray();
  return move(builder_);
}

}  // namespace json
