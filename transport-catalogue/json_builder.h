#pragma once

#include <string>
#include <vector>

#include "json.h"

namespace json {

class Builder {
 public:
  Builder& Key(std::string);
  Builder& Value(Node::Value);
  Builder& StartDict();
  Builder& EndDict();
  Builder& StartArray();
  Builder& EndArray();
  Node Build();
 private:
  std::vector<Node> stack_;
  bool expect_key_ = false;
  bool finished_ = false;
  std::vector<std::string> key_stack_;
};

} // namespace json
