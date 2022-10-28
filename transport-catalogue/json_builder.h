#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "json.h"

namespace json {

class DictKeyPart;
class DictValuePart;
class ArrayPart;

class Builder {
 public:
  Builder() = default;
  Builder(Builder &&other);

  Builder(const Builder&) = delete;
  Builder& operator=(const Builder&) = delete;
  Builder& operator=(Builder&&) = delete;

  Builder& Key(std::string);
  Builder& Value(Node::Value);
  DictKeyPart StartDict();
  Builder& EndDict();
  ArrayPart StartArray();
  Builder& EndArray();
  Node Build();

 private:
  std::vector<Node> stack_;
  bool expect_key_ = false;
  bool finished_ = false;
  std::vector<std::string> key_stack_;
  bool moved_out_of_ = false;
};

class PartBuilder {
 protected:
  PartBuilder(Builder &&builder)
      :
      builder_(std::move(builder)) {
  }
  Builder builder_;

  friend Builder;
  friend DictKeyPart;
  friend DictValuePart;
};

class DictKeyPart : private PartBuilder {
  using PartBuilder::PartBuilder;
 public:
  DictKeyPart(DictKeyPart&&) = default;

  DictKeyPart(const DictKeyPart&) = delete;
  DictKeyPart& operator=(const DictKeyPart&) = delete;
  DictKeyPart& operator=(DictKeyPart&&) = delete;

  DictValuePart Key(std::string);
  Builder EndDict();
};

class DictValuePart : private PartBuilder {
  using PartBuilder::PartBuilder;
 public:
  DictValuePart(DictValuePart&&) = default;

  DictValuePart(const DictValuePart&) = delete;
  DictValuePart& operator=(const DictValuePart&) = delete;
  DictValuePart& operator=(DictValuePart&&) = delete;

  DictKeyPart Value(Node::Value);
  DictKeyPart StartDict();
  ArrayPart StartArray();
};

class ArrayPart : private PartBuilder {
  using PartBuilder::PartBuilder;
 public:
  ArrayPart(ArrayPart&&) = default;

  ArrayPart(const ArrayPart&) = delete;
  ArrayPart& operator=(const ArrayPart&) = delete;
  ArrayPart& operator=(ArrayPart&&) = delete;

  ArrayPart& Value(Node::Value);
  DictKeyPart StartDict();
  ArrayPart StartArray();
  Builder EndArray();
};

}  // namespace json
