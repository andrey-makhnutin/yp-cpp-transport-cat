#pragma once

#include <string>
#include <vector>

#include "json.h"

namespace json {

class DictBuilder;
class ArrayBuilder;

class Builder {
 private:
  class PartBuilder {
   private:
    PartBuilder(Builder &builder)
        :
        builder_(builder) {
    }
    Builder &builder_;

    friend Builder;
  };
 public:
  class DictValuePart;
  class ArrayPart;

  class DictKeyPart : private PartBuilder {
    using PartBuilder::PartBuilder;
   public:
    DictValuePart Key(std::string);
    Builder& EndDict();
  };

  class DictValuePart : private PartBuilder {
    using PartBuilder::PartBuilder;
   public:
    DictKeyPart Value(Node::Value);
    DictKeyPart StartDict();
    ArrayPart StartArray();
  };

  class ArrayPart : private PartBuilder {
    using PartBuilder::PartBuilder;
   public:
    ArrayPart& Value(Node::Value);
    DictKeyPart StartDict();
    ArrayPart StartArray();
    Builder& EndArray();
  };

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
};

}  // namespace json
