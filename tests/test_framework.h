#pragma once

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace TestRunnerPrivate {

template<class Map>
std::ostream& PrintMap(std::ostream &os, const Map &m) {
  os << "{";
  bool first = true;
  for (const auto &kv : m) {
    if (!first) {
      os << ", ";
    }
    first = false;
    os << kv.first << ": " << kv.second;
  }
  return os << "}";
}

template<class Set>
std::ostream& PrintSet(std::ostream &os, const Set &s) {
  os << "{";
  bool first = true;
  for (const auto &x : s) {
    if (!first) {
      os << ", ";
    }
    first = false;
    os << x;
  }
  return os << "}";
}

}  // namespace TestRunnerPrivate

template<class T>
std::ostream& operator<<(std::ostream &os, const std::vector<T> &s) {
  os << "{";
  bool first = true;
  for (const auto &x : s) {
    if (!first) {
      os << ", ";
    }
    first = false;
    if constexpr (std::is_same_v<T, std::string>
        || std::is_same_v<T, std::string_view>) {
      os << "'";
    }
    os << x;
    if constexpr (std::is_same_v<T, std::string>
        || std::is_same_v<T, std::string_view>) {
      os << "'";
    }
  }
  return os << "}";
}

template<class T, class C>
std::ostream& operator<<(std::ostream &os, const std::set<T, C> &s) {
  return TestRunnerPrivate::PrintSet(os, s);
}

template<class T, class H, class Eq>
std::ostream& operator<<(std::ostream &os,
                         const std::unordered_set<T, H, Eq> &s) {
  return TestRunnerPrivate::PrintSet(os, s);
}

template<class K, class V, class C>
std::ostream& operator<<(std::ostream &os, const std::map<K, V, C> &m) {
  return TestRunnerPrivate::PrintMap(os, m);
}

template<class K, class V, class H, class Eq>
std::ostream& operator<<(std::ostream &os,
                         const std::unordered_map<K, V, H, Eq> &m) {
  return TestRunnerPrivate::PrintMap(os, m);
}

/**
 * Сравнивает значения `t` и `u`. Если они не равны, тест проваливается.
 * Строка `hint` содержит подсказку, которая выводится, если тест провален.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    AssertEqual("Hello "s + "world"s, "Hello world"s, "String concatenation error"s);
 *  }
 *  ```
 */
template<class T, class U>
void AssertEqual(const T &t, const U &u, const std::string &hint = { }) {
  if (!(t == u)) {
    std::ostringstream os;
    os << "Assertion failed: " << t << " != " << u;
    if (!hint.empty()) {
      os << " hint: " << hint;
    }
    throw std::runtime_error(os.str());
  }
}

/**
 * Сравнивает значения `t` и `u`. Если они равны, тест проваливается.
 * Строка `hint` содержит подсказку, которая выводится, если тест провален.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    AssertNotEqual(1.0, 1.00001, "Double comparison error"s);
 *  }
 *  ```
 */
template<class T, class U>
void AssertNotEqual(const T &t, const U &u, const std::string &hint = { }) {
  if (t == u) {
    std::ostringstream os;
    os << "Assertion failed: " << t << " == " << u;
    if (!hint.empty()) {
      os << " hint: " << hint;
    }
    throw std::runtime_error(os.str());
  }
}

/**
 * Проверяет истинность значения `b`, если нет, тест проваливается.
 * Строка `hint` содержит подсказку, которая выводится, если тест провален.
 */
inline void Assert(bool b, const std::string &hint) {
  AssertEqual(b, true, hint);
}

/**
 * Класс `TestRunner` запускает тест-функции.
 * Пример:
 *  ```
 *  void Test1() {
 *    // ...
 *  }
 *
 *  void Test2() {
 *    // ...
 *  }
 *
 *  int main() {
 *    TestRunner tr;
 *    // Запускает функцию Test1. Если тест будет провален, его имя будет выведено как
 *    // First test
 *    tr.RunTest(Test1, "First test"s);
 *    // Если имя теста, совпадает с именем тест-функции, можно использовать максро RUN_TEST:
 *    RUN_TEST(tr, Test2); // Аналогично tr.RunTest(Test2, "Test2");
 *  }
 *  ```
 */
class TestRunner {
 public:
  template<class TestFunc>
  void RunTest(TestFunc func, const std::string &test_name) {
    try {
      func();
      std::cerr << test_name << " OK" << std::endl;
    } catch (std::exception &e) {
      ++fail_count;
      std::cerr << test_name << " fail: " << e.what() << std::endl;
    } catch (...) {
      ++fail_count;
      std::cerr << "Unknown exception caught" << std::endl;
    }
  }

  ~TestRunner() {
    std::cerr.flush();
    if (fail_count > 0) {
      std::cerr << fail_count << " unit tests failed. Terminate" << std::endl;
      exit(1);
    }
  }

 private:
  int fail_count = 0;
};

#ifndef _FILE_NAME
#define _FILE_NAME __FILE__
#endif

/**
 * Макрос `ASSERT_EQUAL_HINT` проверяет значения выражений `x` и `y` на равенство.
 * Если значения не равны, тест считается проваленным.
 * Строка `hint` будет включена в текст ошибки.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    ASSERT_EQUAL_HINT(2 + 2, 4, "Math is working");
 *    // Эта проверка не сработает, и тест будет провален.
 *    ASSERT_EQUAL_HINT(2 + 2, 5, "Math is not working");
 *  }
 *  ```
 */
#define ASSERT_EQUAL_HINT(x, y, hint)                                                   \
  {                                                                                     \
    std::ostringstream __assert_private_os;                                             \
    __assert_private_os << #x << " != " << #y << ", " << _FILE_NAME << ":" << __LINE__; \
    if (!std::string(hint).empty()) {                                                   \
      __assert_private_os << " (" << hint << ")";                                       \
    }                                                                                   \
    AssertEqual(x, y, __assert_private_os.str());                                       \
  }

/**
 * Макрос `ASSERT_EQUAL` проверяет значения выражений `x` и `y` на равенство.
 * Если значения не равны, тест считается проваленным.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    ASSERT_EQUAL(2 + 2, 4);
 *    ASSERT_EQUAL(2 + 2, 5); // Эта проверка не сработает, и тест будет провален
 *  }
 *  ```
 */
#define ASSERT_EQUAL(x, y) ASSERT_EQUAL_HINT(x, y, "")

/**
 * Макрос `ASSERT_NOT_EQUAL` проверяет значения выражений `x` и `y` на неравенство.
 * Если значения равны, тест считается проваленным.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    ASSERT_NOT_EQUAL(2 + 2, 5);
 *    // Эта проверка не сработает, и тест будет провален.
 *    ASSERT_NOT_EQUAL(2 + 2, 4);
 *  }
 *  ```
 */
#define ASSERT_NOT_EQUAL(x, y)                                                          \
  {                                                                                     \
    std::ostringstream __assert_private_os;                                             \
    __assert_private_os << #x << " == " << #y << ", " << _FILE_NAME << ":" << __LINE__; \
    AssertNotEqual(x, y, __assert_private_os.str());                                    \
  }

/**
 * Макрос `ASSERT_HINT` проверяет истинность выражения `x`. Выражение `x` должно
 * конвертироваться к типу `bool`.
 * Если выражение `x` ложно, тест считается проваленным. Если выражение `x` истинно,
 * выполнение теста продолжается.
 * Строка `hint` будет включена в текст ошибки.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    ASSERT_HINT(2 + 2 == 4, "Math works");
 *    ASSERT_HINT(2, "Boolean cast works"); // число 2 при преобразовании к bool станет значением true
 *    ASSERT_HINT(false, "False is true"); // здесь тест провалится
 *    string user_name = "Harry Potter"s;
 *    // Если раскомментировать следующую строку, программа не скомпилируется,
 *    // так как string не может быть преобразован к типу bool.
 *    // ASSERT_HINT(user_name, "String to bool cast works");
 *  }
 *  ```
 */
#define ASSERT_HINT(x, hint)                                                     \
  {                                                                              \
    std::ostringstream __assert_private_os;                                      \
    __assert_private_os << #x << " is false, " << _FILE_NAME << ":" << __LINE__; \
    if (!std::string(hint).empty()) {                                            \
      __assert_private_os << " (" << hint << ")";                                \
    }                                                                            \
    Assert(static_cast<bool>(x), __assert_private_os.str());                     \
  }

/**
 * Макрос `ASSERT` проверяет истинность выражения `x`. Выражение `x` должно
 * конвертироваться к типу `bool`.
 * Если выражение `x` ложно, тест считается проваленным. Если выражение `x` истинно,
 * выполнение теста продолжается.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    ASSERT(2 + 2 == 4);
 *    ASSERT(2); // число 2 при преобразовании к bool станет значением true
 *    ASSERT(false); // здесь тест провалится
 *    string user_name = "Harry Potter"s;
 *    // Если раскомментировать следующую строку, программа не скомпилируется,
 *    // так как string не может быть преобразован к типу bool.
 *    // ASSERT(user_name);
 *  }
 *  ```
 */
#define ASSERT(x)                                                                \
  {                                                                              \
    std::ostringstream __assert_private_os;                                      \
    __assert_private_os << #x << " is false, " << _FILE_NAME << ":" << __LINE__; \
    Assert(static_cast<bool>(x), __assert_private_os.str());                     \
  }

/**
 * Макрос `RUN_TEST` служит для удобного запуска тест-функции `func`.
 * Параметр `tr` задаёт имя переменной типа `TestRunner`.
 *
 * Пример:
 *  ```
 *  void Test1() {
 *    // Содержимое тест-функции ...
 *  }
 *
 *  void Test2() {
 *    // Содержимое тест-функции ...
 *  }
 *
 *  int main() {
 *    TestRunner tr;
 *    RUN_TEST(tr, Test1);
 *    RUN_TEST(tr, Test2);
 *  }
 *  ```
 */
#define RUN_TEST(tr, func) tr.RunTest(func, #func)

/**
 * Макрос `ASSERT_THROWS_HINT` проверяет, что при вычислении выражения `expr` будет
 * выброшено исключение типа `expected_exception`.
 * Если исключение выброшено не будет, либо выбросится исключение другого типа,
 * тест считается проваленным.
 * Строка `hint` будет включена в текст ошибки.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    using namespace std;
 *    ASSERT_THROWS_HINT(stoi("not-a-number"s), invalid_argument, "stoi(string) throws");
 *  }
 *  ```
 */
#define ASSERT_THROWS_HINT(expr, expected_exception, hint)                                   \
  {                                                                                          \
    bool __assert_private_flag = true;                                                       \
    try {                                                                                    \
      expr;                                                                                  \
      __assert_private_flag = false;                                                         \
    } catch (expected_exception&) {                                                          \
    } catch (...) {                                                                          \
      std::ostringstream __assert_private_os;                                                \
      __assert_private_os << "Expression " #expr                                             \
                             " threw an unexpected exception"                                \
                             " " _FILE_NAME ":"                                              \
                          << __LINE__;                                                       \
      if (!std::string(hint).empty()) {                                                      \
        __assert_private_os << " (" << hint << ")";                                          \
      }                                                                                      \
      Assert(false, __assert_private_os.str());                                              \
    }                                                                                        \
    if (!__assert_private_flag) {                                                            \
      std::ostringstream __assert_private_os;                                                \
      __assert_private_os << "Expression " #expr                                             \
                             " is expected to throw " #expected_exception " " _FILE_NAME ":" \
                          << __LINE__;                                                       \
      if (!std::string(hint).empty()) {                                                      \
        __assert_private_os << " (" << hint << ")";                                          \
      }                                                                                      \
      Assert(false, __assert_private_os.str());                                              \
    }                                                                                        \
  }

/**
 * Макрос `ASSERT_THROWS` проверяет, что при вычислении выражения `expr` будет
 * выброшено исключение типа `expected_exception`.
 * Если исключение выброшено не будет, либо выбросится исключение другого типа,
 * тест считается проваленным.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    using namespace std;
 *    ASSERT_THROWS(stoi("not-a-number"s), invalid_argument);
 *  }
 *  ```
 */
#define ASSERT_THROWS(expr, expected_exception) ASSERT_THROWS_HINT(expr, expected_exception, "")

/**
 * Макрос `ASSERT_DOESNT_THROW_HINT` проверяет, что при вычислении выражения `expr`
 * не будет выброшено никаких исключений.
 * Если при вычислении выражения `expr` выбросится исключение, тест будет провален.
 * Строка `hint` будет включена в текст ошибки.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    vector<int> v;
 *    v.push_back(1);
 *    ASSERT_DOESNT_THROW_HINT(v.at(0)), "At within bounds doesn't throw");
 *  }
 *  ```
 */
#define ASSERT_DOESNT_THROW_HINT(expr, hint)                    \
    try {                                                       \
        expr;                                                   \
    } catch (std::exception &e) {                               \
        std::ostringstream __assert_private_os;                 \
        __assert_private_os << "Expression " #expr              \
                               " threw an unexpected exception" \
                               " "                              \
                            << e.what()                         \
                            << " " _FILE_NAME ":"               \
                            << __LINE__;                        \
        if (!std::string(hint).empty()) {                       \
          __assert_private_os << " (" << hint << ")";           \
        }                                                       \
        Assert(false, __assert_private_os.str());               \
    } catch (...) {                                             \
        std::ostringstream __assert_private_os;                 \
        __assert_private_os << "Expression " #expr              \
                               " threw an unexpected exception" \
                               " " _FILE_NAME ":"               \
                            << __LINE__;                        \
        if (!std::string(hint).empty()) {                       \
          __assert_private_os << " (" << hint << ")";           \
        }                                                       \
        Assert(false, __assert_private_os.str());               \
    }

/**
 * Макрос `ASSERT_DOESNT_THROW` проверяет, что при вычислении выражения `expr`
 * не будет выброшено никаких исключений.
 * Если при вычислении выражения `expr` выбросится исключение, тест будет провален.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    vector<int> v;
 *    v.push_back(1);
 *    ASSERT_DOESNT_THROW(v.at(0))
 *  }
 *  ```
 */
#define ASSERT_DOESNT_THROW(expr) ASSERT_DOESNT_THROW_HINT(expr, "")

/**
 * Проверяет, что элементы контейнера `container` имеют строго убывающий порядок.
 */
template<typename Container>
inline void AssertItemsDescImpl(const Container &container,
                                const std::string &hint) {
  if (container.empty()) {
    return;
  }
  auto prev = container.cbegin();
  auto cur = std::next(prev);
  while (cur != container.cend()) {
    if (*cur >= *prev) {
      std::ostringstream os;
      os << "Assertion failed: expected container items to be in strict descending order.";
      if (!hint.empty()) {
        os << " hint: " << hint;
      }
      throw std::runtime_error(os.str());
    }
    std::advance(cur, 1);
    std::advance(prev, 1);
  }
}

/**
 * Макрос `ASSERT_ITEMS_DESC_HINT` проверяет, что элементы `container`
 * находятся в строго убывающем порядке.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    // Упадёт: элементы не убывают
 *    ASSERT_ITEMS_DESC_HINT((std::list<int> {3, 3}), "Bad");
 *    // Упадёт: элементы возрастают
 *    ASSERT_ITEMS_DESC_HINT((std::list<int> {3, 4}), "Bad");
 *    // Не упадёт
 *    ASSERT_ITEMS_DESC_HINT((std::list<int> {3, 2, 1}), "Good");
 *  }
 *  ```
 */
#define ASSERT_ITEMS_DESC_HINT(container, hint)                \
  {                                                            \
    std::ostringstream __assert_private_os;                    \
    __assert_private_os << "Container " #container ", "        \
                           _FILE_NAME  ":"                     \
                        << __LINE__ ;                          \
    if (!std::string(hint).empty()) {                          \
      __assert_private_os << " (" << hint << ")";              \
    }                                                          \
    AssertItemsDescImpl(container, __assert_private_os.str()); \
  }

/**
 * Проверяет, что все элементы контейнера `container` равны
 */
template<typename Container>
inline void AssertItemsEqualImpl(const Container &container,
                                 const std::string &hint) {
  if (container.empty()) {
    return;
  }
  auto prev = container.cbegin();
  auto cur = std::next(prev);
  while (cur != container.cend()) {
    if (!(*cur == *prev)) {
      std::ostringstream os;
      os << "Assertion failed: expected all items of container to be equal.";
      if (!hint.empty()) {
        os << " hint: " << hint;
      }
      throw std::runtime_error(os.str());
    }
    std::advance(cur, 1);
    std::advance(prev, 1);
  }
}

/**
 * Макрос `ASSERT_ITEMS_EQUAL` проверяет, что все элементы `container` равны
 *
 * Пример:
 *  ```
 *  void Test() {
 *    ASSERT_ITEMS_EQUAL((std::list<int> {3, 3}));
 *    // Упадёт
 *    ASSERT_ITEMS_EQUAL((std::list<int> {3, 4}));
 *  }
 *  ```
 */
#define ASSERT_ITEMS_EQUAL(container)                            \
   {                                                             \
     std::ostringstream __assert_private_os;                     \
     __assert_private_os << "Container " #container ", "         \
                            _FILE_NAME  ":"                      \
                         << __LINE__ ;                           \
     AssertItemsEqualImpl(container, __assert_private_os.str()); \
   }

/**
 * Сравнивает значения t и u. Если различаются больше чем на 0.0001%, тест проваливается.
 * Строка hint содержит подсказку, которая выводится, если тест провален.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    AssertSoftEqual(1ull, 1.000001, "ok");
 *  }
 *  ```
 */
template<class T, class U>
void AssertSoftEqual(const T &t, const U &u, const std::string &hint = { }) {
  double diff = std::abs(static_cast<double>(t) - static_cast<double>(u))
      / std::abs(static_cast<double>(t));
  const double cutoff = 1.001e-6;
  if (diff > cutoff) {
    std::ostringstream os;
    os << std::setprecision(7);
    os << "Assertion failed: " << t << " !≈ " << u;
    os << ", difference is " << (diff * 100) << "%";
    if (!hint.empty()) {
      os << " hint: " << hint;
    }
    throw std::runtime_error(os.str());
  }
}

/**
 * Макрос `ASSERT_SOFT_EQUAL_HINT` проверяет значения выражений `x` и `y` на мягкое равенство.
 * Если значения различаются больше чем на 0.000001%, тест считается проваленным.
 * Строка `hint` будет включена в текст ошибки.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    ASSERT_SOFT_EQUAL_HINT(1ull, 1.000001, "ok");
 *    // Эта проверка не сработает, и тест будет провален.
 *    ASSERT_SOFT_EQUAL_HINT(1ull, 1.00001, "bad");
 *  }
 *  ```
 */
#define ASSERT_SOFT_EQUAL_HINT(x, y, hint)                                              \
  {                                                                                     \
    std::ostringstream __assert_private_os;                                             \
    __assert_private_os << #x << " !≈ " << #y << ", " << _FILE_NAME << ":" << __LINE__; \
    if (!std::string(hint).empty()) {                                                   \
      __assert_private_os << " (" << hint << ")";                                       \
    }                                                                                   \
    AssertSoftEqual(x, y, __assert_private_os.str());                                   \
  }

/**
 * Макрос `ASSERT_SOFT_EQUAL` проверяет значения выражений `x` и `y` на мягкое равенство.
 * Если значения различаются больше чем на 0.000001%, тест считается проваленным.
 * Строка `hint` будет включена в текст ошибки.
 *
 * Пример:
 *  ```
 *  void Test() {
 *    ASSERT_SOFT_EQUAL(1ull, 1.000001);
 *    // Эта проверка не сработает, и тест будет провален.
 *    ASSERT_SOFT_EQUAL(1ull, 1.00001);
 *  }
 *  ```
 */
#define ASSERT_SOFT_EQUAL(x, y) ASSERT_SOFT_EQUAL_HINT(x, y, "")
