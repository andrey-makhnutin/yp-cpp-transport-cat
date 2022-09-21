#include "json.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <utility>

using namespace std;

namespace json {

namespace {

/**
 * Вспомогательный класс для парсинга JSON из `istream`s
 */
class NodeParser {
 public:
  NodeParser(istream &input)
      :
      input_(input) {
  }

  /**
   * Парсит элемент JSON.
   * Ожидается, что все пробелы уже были предварительно прочитаны из потока.
   * Если элемент не удалось прочитать, кидает исключение `ParsingError`.
   */
  Node LoadNode() {
    const char c = ExpectChar();

    if (isdigit(c) || c == '+' || c == '-' || c == '.') {
      input_.putback(c);
      return LoadNumber();
    }

    switch (c) {
      case 't':
        ExpectStr("rue"sv);
        return true;
      case 'f':
        ExpectStr("alse"sv);
        return false;
      case '"':
        return LoadString();
      case 'n':
        ExpectStr("ull"sv);
        return nullptr;
      case '[':
        return LoadArray();
      case '{':
        return LoadDict();
    }

    Fail("unexpected character '"s + c + "'"s);
    // недостижимый код
    return {};
  }
 private:
  istream &input_;

  /**
   * Проверяет, что из потока можно прочитать один символ, и возвращает его.
   * Если поток закончился, кидает исключение `ParsingError`.
   */
  char ExpectChar() {
    char c = input_.get();
    if (input_.eof()) {
      Fail("reached end of stream"s);
    }
    return c;
  }

  /**
   * Проверяет, что дальше в потоке идёт строка `str`.
   * Если нет, то кидает исключение `ParsingError`.
   */
  void ExpectStr(const string_view str) {
    for (char expected_char : str) {
      char c = ExpectChar();
      if (c != expected_char) {
        Fail("unexpected character '"s + c + "'"s);
      }
    }
  }

  Node LoadNumber() {
    string number_str;

    // флажок указывает, было ли число записано как double: значит либо была точка,
    // либо запись экспонентная
    bool double_repr = false;

    // читаем из потока все символы, которые могут встретиться в записи числа,
    // но не проверяем осмысленность этих символов и порядка в котором они идут.
    while (true) {
      const char c = input_.get();
      if (input_.eof()) {
        break;
      }
      if (isdigit(c)) {
        number_str.push_back(c);
      } else {
        bool end_of_number = false;
        switch (c) {
          case '.':
          case 'e':
          case 'E':
            double_repr = true;
            number_str.push_back(c);
            break;
          case '-':
          case '+':
            number_str.push_back(c);
            break;
          default:
            end_of_number = true;
        }

        // когда нашли символ, который не может быть частью записи числа,
        // возвращаем его обратно, и пытаемся распарсить всё, что нашли до этого
        if (end_of_number) {
          input_.putback(c);
          break;
        }
      }
    }
    if (number_str.empty()) {
      Fail("reached end of stream"s);
    }

    // пытаемся превратить все прочитанные символы в число
    char *endp;
    const double number = strtod(number_str.c_str(), &endp);

    // если только часть символов была интерпретирована как число, падаем
    if (static_cast<unsigned>(endp - number_str.c_str()) != number_str.size()) {
      Fail("error parsing number");
    }

    // если в записи числа встретилась точка или экспонента, то это всегда double
    if (double_repr) {
      return number;
    }

    // здесь мы можем быть уверены, что точки не было, значит число целое.
    // но если оно не влезает в int, то делаем JSON элемент с типом double
    if (number > static_cast<double>(numeric_limits<int>::max())
        || number < static_cast<double>(numeric_limits<int>::min())) {
      return number;
    }

    // число целое, и достаточно маленькое - значит возвращаем int
    return static_cast<int>(number);
  }

  /**
   * Читает строку из потока. Считается, что открывающая двойная кавычка уже прочитана.
   *
   * Поддерживаются escape-последовательности \r, \n, \t, \\ и \".
   * Последовательности \x?? не поддерживаются.
   */
  Node LoadString() {
    string result;
    bool escaping = false;
    while (true) {
      char c = ExpectChar();
      if (escaping) {
        switch (c) {
          case 'r':
            c = '\r';
            break;
          case 'n':
            c = '\n';
            break;
          case 't':
            c = '\t';
            break;
          case 'x':
            throw runtime_error("hex escapes are not supported"s);
        }
        escaping = false;
      } else if (c == '"') {
        break;
      } else if (c == '\\') {
        escaping = true;
        continue;
      }
      result.push_back(c);
    }
    return result;
  }

  /**
   * Читает JSON массив. Считается, что открывающая квадратная скобка уже прочитана.
   */
  Node LoadArray() {
    Array result;
    while (true) {
      input_ >> ws;

      // Сразу после '[' может быть ']', тогда массив пустой.
      if (result.empty()) {
        char c = ExpectChar();
        if (c == ']') {
          break;
        }
        input_.putback(c);
      }

      // рекурсивно читаем JSON элемент
      result.push_back(LoadNode());

      input_ >> ws;
      char c = ExpectChar();
      if (c == ']') {
        break;
      }
      if (c != ',') {
        Fail("error reading array: expected comma, got '"s + c + "'"s);
      }
    }
    return result;
  }

  /**
   * Читает JSON словарик. Считается, что открывающая фигурная скобка уже прочитана.
   */
  Node LoadDict() {
    Dict result;

    while (true) {
      input_ >> ws;

      // элементы JSON словарика всегда начинаются с двойной кавычки.
      // Исключение - в пустом словаре сразу после открывающей идёт закрывающая
      // фигурная скобка
      char c = ExpectChar();
      if (result.empty() && c == '}') {
        break;
      }
      if (c != '"') {
        Fail("error reading dict: expected double quotes, got '"s + c + "'"s);
      }

      // читаем строку-ключ
      string key = move(LoadString().AsString());

      // после ключа должно идти двоеточие
      input_ >> ws;
      c = ExpectChar();
      if (c != ':') {
        Fail("error reading dict: expected colon, got '"s + c + "'"s);
      }

      // рекурсивно читаем JSON-элемент
      input_ >> ws;
      result.emplace(move(key), LoadNode());

      input_ >> ws;
      c = ExpectChar();
      if (c == '}') {
        break;
      }
      if (c != ',') {
        Fail("error reading dict: expected comma, got '"s + c + "'"s);
      }
    }

    return result;
  }

  /**
   * Кидает исключение `ParsingError` и помечает поток флажком `failbit`.
   */
  void Fail(const string &what) {
    input_.setstate(istream::failbit);
    throw ParsingError(what);
  }
};

/**
 * Вспомогательная структура для печати в `ostream` варианта со значением JSON элемента
 * с помощью `visit`.
 */
struct NodePrinter {
  ostream &output_;

  void operator()(int value) {
    output_ << value;
  }
  void operator()(double value) {
    output_ << value;
  }
  void operator()(bool value) {
    output_ << (value ? "true"sv : "false"sv);
  }
  void operator()(const string &value) {
    PrintString(value);
  }
  void operator()(nullptr_t) {
    output_ << "null"sv;
  }
  void operator()(const Array &value) {
    bool first = true;
    output_.put('[');
    for (const auto &node : value) {
      if (!first) {
        output_.put(',');
      }
      first = false;
      node.Print(output_);
    }
    output_.put(']');
  }
  void operator()(const Dict &value) {
    bool first = true;
    output_.put('{');
    for (const auto& [key, node] : value) {
      if (!first) {
        output_.put(',');
      }
      first = false;
      PrintString(key);
      output_.put(':');
      node.Print(output_);
    }
    output_.put('}');
  }

 private:
  void PrintString(const string &str) {
    output_.put('"');
    for (const char c : str) {
      switch (c) {
        case '\r':
          output_ << "\\r"sv;
          break;
        case '\n':
          output_ << "\\n"sv;
          break;
        case '\\':
          output_ << "\\\\"sv;
          break;
        case '"':
          output_ << "\\\""sv;
          break;
        default:
          output_.put(c);
      }
    }
    output_.put('"');
  }
};

}  // namespace

Node::Node(int value)
    :
    value_(value) {
}

Node::Node(double value)
    :
    value_(value) {
}

Node::Node(bool value)
    :
    value_(value) {
}

Node::Node(string value)
    :
    value_(move(value)) {
}

Node::Node(const char *value)
    :
    value_(string { value }) {
}

Node::Node(nullptr_t value)
    :
    value_(value) {
}

Node::Node(Array array)
    :
    value_(move(array)) {
}

Node::Node(Dict map)
    :
    value_(move(map)) {
}

bool Node::IsInt() const {
  return holds_alternative<int>(value_);
}

bool Node::IsDouble() const {
  return holds_alternative<int>(value_) || holds_alternative<double>(value_);
}

bool Node::IsPureDouble() const {
  return holds_alternative<double>(value_);
}

bool Node::IsBool() const {
  return holds_alternative<bool>(value_);
}

bool Node::IsString() const {
  return holds_alternative<string>(value_);
}

bool Node::IsNull() const {
  return holds_alternative<nullptr_t>(value_);
}

bool Node::IsArray() const {
  return holds_alternative<Array>(value_);
}

bool Node::IsMap() const {
  return holds_alternative<Dict>(value_);
}

int Node::AsInt() const {
  if (IsInt()) {
    return get<int>(value_);
  }
  throw logic_error("not an int node"s);
}

double Node::AsDouble() const {
  if (IsInt()) {
    return static_cast<double>(get<int>(value_));
  } else if (IsPureDouble()) {
    return get<double>(value_);
  }
  throw logic_error("not a double node"s);
}

bool Node::AsBool() const {
  if (IsBool()) {
    return get<bool>(value_);
  }
  throw logic_error("not a bool node"s);
}

const string& Node::AsString() const {
  if (IsString()) {
    return get<string>(value_);
  }
  throw logic_error("not a string node"s);
}

const Array& Node::AsArray() const {
  if (IsArray()) {
    return get<Array>(value_);
  }
  throw logic_error("not an array node"s);
}

const Dict& Node::AsMap() const {
  if (IsMap()) {
    return get<Dict>(value_);
  }
  throw logic_error("not a dict node"s);
}

void Node::Print(ostream &output) const {
  visit(NodePrinter { output }, value_);
}

bool Node::operator ==(const Node &other) const {
  return value_ == other.value_;
}

bool Node::operator !=(const Node &other) const {
  return !(*this == other);
}

Document::Document(Node root)
    :
    root_(move(root)) {
}

const Node& Document::GetRoot() const {
  return root_;
}

bool Document::operator ==(const Document &other) const {
  return root_ == other.root_;
}

bool Document::operator !=(const Document &other) const {
  return !(*this == other);
}

/**
 * Парсит JSON документ из потока `istream`.
 *
 * Предварительно проверяется, что поток находится в хорошем состоянии.
 * Если при чтении JSON возникает ошибка, кидается исключение `ParsingError`,
 * а поток помечается флагом `failbit`.
 */
Document Load(istream &input) {
  istream::sentry sentry { input };
  if (!sentry) {
    if (input.eof()) {
      throw ParsingError("reached end of stream"s);
    }
    throw ParsingError("input stream is in bad state"s);
  }
  NodeParser parser { input };
  return Document { parser.LoadNode() };
}

/**
 * Печатает JSON документ в поток `ostream`
 */
void Print(const Document &doc, ostream &output) {
  doc.GetRoot().Print(output);
}

}  // namespace json
