#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "json.h"

namespace json {

class DictKeyPart;
class DictValuePart;
class ArrayPart;

/**
 * Конструктор JSON.
 *
 * Конструкция начинается с пустого объекта, и заканчивается
 * вызовом метода `Build()`.
 *
 * Для построения примитивного JSON, используется один вызов `Value()`:
 * ```
 * json::Node string_node = json::Builder{}.Value("hello"s).Build();
 * ```
 *
 * Словарь строится методом `StartDict()`, за которым следуют пары вызовов
 * `Key(...).Value(...)`.
 * Конструкция словаря должна обязательно заканчиваться вызовом `EndDict()`:
 * ```
 * json::Node dict_node = json::Builder{}.StartDict()
 *     .Key("hello"s).Value(123)
 *   .EndDict().Build();
 * ```
 * Чтобы добавить вложенный словарь или массив в качестве значения в словаре,
 * вместо `Value()` можно сделать вызов `StartDict()` или `StartArray()`:
 * ```
 * json::Node dict_node = json::Builder{}.StartDict()
 *     .Key("array value"s).StartArray()
 *       .Value(1234)
 *       .Value("abcd"s)
 *     .EndArray()
 *     .Key("dict value"s).StartDict()
 *       .Key("hello"s).Value(123)
 *     .EndDict()
 *   .EndDict().Build();
 * ```
 *
 * Построение массива начинается с вызова `StartArray()`, за которым может
 * следовать несколько вызовов `Value()`, оканчивающихся на `EndArray()`:
 * ```
 * json::Node arr_node = json::Builder{}.StartArray()
 *     .Value(123)
 *     .Value(124)
 *     .Value("abc"s)
 *   .EndArray().Build();
 * ```
 *
 * Для построения простых JSON объектов обычно достаточно цепочки вызовов,
 * оканчивающихся на `Build()`.
 * Однако в более сложных случаях допускается сохранение недостроенного JSON
 * в отдельную переменную. При этом следует иметь в виду, что следующий же
 * вызов метода у этой переменной оставит её в "заброшенном" ("moved-out-of")
 * состоянии, и последующее переиспользование этой переменной приведёт к
 * выбросу исключения:
 * ```
 * auto dict = json::Builder{}.StartDict();
 * dict.Key("hello"s).Value(123);
 * dict.Key("world"s).Value(124); // вызов `Key` бросит исключение
 * cout << dict.EndDict().Build() << endl;
 * ```
 * Чтобы достичь желаемого в примере выше эффекта, можно обновлять переменную
 * в промежуточных точках:
 * ```
 * auto dict = json::Builder{}.StartDict();
 * dict = dict.Key("hello"s).Value(123);
 * dict = dict.Key("world"s).Value(124);
 * cout << dict.EndDict().Build() << endl;
 * ```
 *
 * Исключением из этого правила является конструктор массива - его можно
 * сохранить в переменную, и вызывать метод `Value()` несколько раз,
 * т.к. тот не конструирует новый временный объект, а возвращает ссылку
 * на конструктор массива:
 * ```
 * auto arr = json::Builder{}.StartArray();
 * arr.Value(1);
 * arr.Value(2).Value(3);
 * cout << arr.EndArray().Build() << endl;
 * ```
 */
class Builder {
 public:
  Builder() = default;
  Builder(Builder&& other);
  Builder& operator=(Builder&&);

  Builder(const Builder&) = delete;
  Builder& operator=(const Builder&) = delete;

  Builder& Key(std::string);
  Builder& Value(Node::Value);
  DictKeyPart StartDict();
  Builder& EndDict();
  ArrayPart StartArray();
  Builder& EndArray();
  Node Build();

 private:
  // Верхняя нода в стеке - текущий собираемый массив или словарь.
  std::vector<Node> stack_;

  // `true`, если сейчас собирается словарь, и ожидается что будет либо ключ,
  // либо конец словаря.
  bool expect_key_ = false;

  // `true` после сборки ноды верхнего уровня. При этом в `stack_` должен быть
  // единственный элемент.
  bool finished_ = false;

  // Стек названий ключей во вложенных собираемых словарях
  // Размер этого стека не равен размеру `stack_`, но меньше или равен
  // количеству словарей в `stack_`.
  std::vector<std::string> key_stack_;

  // `true`, если значение данного конструктора было перемещено
  // в другую переменную. После этого конструктором пользоваться нельзя.
  bool moved_out_of_ = false;

  void Swap(Builder& other);
};

/**
 * Общий класс для вспомогательных классов - конструкторов частей JSON нод.
 * Они все конструируются перемещением `Builder` внутрь вспомогательного класса.
 * Таким образом единственный рабочий конструктор либо находится внутри
 * какого-то одного объекта вспомогательного класса,
 * либо сам по себе в какой-то переменной.
 */
class PartBuilder {
 protected:
  PartBuilder(Builder&& builder) : builder_(std::move(builder)) {}
  Builder builder_;

  friend Builder;
  friend DictKeyPart;
  friend DictValuePart;
};

/**
 * Часть конструктора JSON. На этом этапе собирается JSON словарь,
 * и ожидаются либо пары `Key()` - `Value()`, либо окончание словаря.
 */
class DictKeyPart : private PartBuilder {
  using PartBuilder::PartBuilder;

 public:
  DictKeyPart(DictKeyPart&&) = default;
  DictKeyPart& operator=(DictKeyPart&&) = default;

  DictKeyPart(const DictKeyPart&) = delete;
  DictKeyPart& operator=(const DictKeyPart&) = delete;

  DictValuePart Key(std::string);
  Builder EndDict();
};

/**
 * Часть конструктора JSON. На этом этапе собирается JSON словарь,
 * и ожидается значение: либо примитив через `Value()`,
 * либо начало вложенного словаря или массива.
 */
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

/**
 * Часть конструктора JSON. На этом этапе собирается JSON массив,
 * и ожидается либо элемент массива, либо окончание массива.
 */
class ArrayPart : private PartBuilder {
  using PartBuilder::PartBuilder;

 public:
  ArrayPart(ArrayPart&&) = default;
  ArrayPart& operator=(ArrayPart&&) = default;

  ArrayPart(const ArrayPart&) = delete;
  ArrayPart& operator=(const ArrayPart&) = delete;

  ArrayPart& Value(Node::Value);
  DictKeyPart StartDict();
  ArrayPart StartArray();
  Builder EndArray();
};

}  // namespace json
