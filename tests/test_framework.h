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

template <class Map>
std::ostream &PrintMap(std::ostream &os, const Map &m) {
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

template <class Set>
std::ostream &PrintSet(std::ostream &os, const Set &s) {
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

template <class T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &s) {
  os << "{";
  bool first = true;
  for (const auto &x : s) {
    if (!first) {
      os << ", ";
    }
    first = false;
    if constexpr (std::is_same_v<T, std::string> ||
                  std::is_same_v<T, std::string_view>) {
      os << "'";
    }
    os << x;
    if constexpr (std::is_same_v<T, std::string> ||
                  std::is_same_v<T, std::string_view>) {
      os << "'";
    }
  }
  return os << "}";
}

template <class T, class C>
std::ostream &operator<<(std::ostream &os, const std::set<T, C> &s) {
  return TestRunnerPrivate::PrintSet(os, s);
}

template <class T, class H, class Eq>
std::ostream &operator<<(std::ostream &os,
                         const std::unordered_set<T, H, Eq> &s) {
  return TestRunnerPrivate::PrintSet(os, s);
}

template <class K, class V, class C>
std::ostream &operator<<(std::ostream &os, const std::map<K, V, C> &m) {
  return TestRunnerPrivate::PrintMap(os, m);
}

template <class K, class V, class H, class Eq>
std::ostream &operator<<(std::ostream &os,
                         const std::unordered_map<K, V, H, Eq> &m) {
  return TestRunnerPrivate::PrintMap(os, m);
}

/**
 * ???????????????????? ???????????????? `t` ?? `u`. ???????? ?????? ???? ??????????, ???????? ??????????????????????????.
 * ???????????? `hint` ???????????????? ??????????????????, ?????????????? ??????????????????, ???????? ???????? ????????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    AssertEqual("Hello "s + "world"s, "Hello world"s,
 *                "String concatenation error"s);
 *  }
 *  ```
 */
template <class T, class U>
void AssertEqual(const T &t, const U &u, const std::string &hint = {}) {
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
 * ???????????????????? ???????????????? `t` ?? `u`. ???????? ?????? ??????????, ???????? ??????????????????????????.
 * ???????????? `hint` ???????????????? ??????????????????, ?????????????? ??????????????????, ???????? ???????? ????????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    AssertNotEqual(1.0, 1.00001, "Double comparison error"s);
 *  }
 *  ```
 */
template <class T, class U>
void AssertNotEqual(const T &t, const U &u, const std::string &hint = {}) {
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
 * ?????????????????? ???????????????????? ???????????????? `b`, ???????? ??????, ???????? ??????????????????????????.
 * ???????????? `hint` ???????????????? ??????????????????, ?????????????? ??????????????????, ???????? ???????? ????????????????.
 */
inline void Assert(bool b, const std::string &hint) {
  AssertEqual(b, true, hint);
}

/**
 * ?????????? `TestRunner` ?????????????????? ????????-??????????????.
 * ????????????:
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
 *    // ?????????????????? ?????????????? Test1. ???????? ???????? ?????????? ????????????????, ?????? ?????? ??????????
 *    // ???????????????? ?????? First test
 *    tr.RunTest(Test1, "First test"s);
 *    // ???????? ?????? ??????????, ?????????????????? ?? ???????????? ????????-??????????????, ?????????? ????????????????????????
 *    // ???????????? RUN_TEST:
 *    RUN_TEST(tr, Test2); // ???????????????????? tr.RunTest(Test2, "Test2");
 *  }
 *  ```
 */
class TestRunner {
 public:
  template <class TestFunc>
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
 * ???????????? `ASSERT_EQUAL_HINT` ?????????????????? ???????????????? ?????????????????? `x` ?? `y` ????
 * ??????????????????. ???????? ???????????????? ???? ??????????, ???????? ?????????????????? ??????????????????????. ???????????? `hint`
 * ?????????? ???????????????? ?? ?????????? ????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    ASSERT_EQUAL_HINT(2 + 2, 4, "Math is working");
 *    // ?????? ???????????????? ???? ??????????????????, ?? ???????? ?????????? ????????????????.
 *    ASSERT_EQUAL_HINT(2 + 2, 5, "Math is not working");
 *  }
 *  ```
 */
#define ASSERT_EQUAL_HINT(x, y, hint)                                      \
  {                                                                        \
    std::ostringstream __assert_private_os;                                \
    __assert_private_os << #x << " != " << #y << ", " << _FILE_NAME << ":" \
                        << __LINE__;                                       \
    if (!std::string(hint).empty()) {                                      \
      __assert_private_os << " (" << hint << ")";                          \
    }                                                                      \
    AssertEqual(x, y, __assert_private_os.str());                          \
  }

/**
 * ???????????? `ASSERT_EQUAL` ?????????????????? ???????????????? ?????????????????? `x` ?? `y` ???? ??????????????????.
 * ???????? ???????????????? ???? ??????????, ???????? ?????????????????? ??????????????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    ASSERT_EQUAL(2 + 2, 4);
 *    // ?????? ???????????????? ???? ??????????????????, ?? ???????? ?????????? ????????????????
 *    ASSERT_EQUAL(2 + 2, 5);
 *  }
 *  ```
 */
#define ASSERT_EQUAL(x, y) ASSERT_EQUAL_HINT(x, y, "")

/**
 * ???????????? `ASSERT_NOT_EQUAL` ?????????????????? ???????????????? ?????????????????? `x` ?? `y` ????
 * ??????????????????????. ???????? ???????????????? ??????????, ???????? ?????????????????? ??????????????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    ASSERT_NOT_EQUAL(2 + 2, 5);
 *    // ?????? ???????????????? ???? ??????????????????, ?? ???????? ?????????? ????????????????.
 *    ASSERT_NOT_EQUAL(2 + 2, 4);
 *  }
 *  ```
 */
#define ASSERT_NOT_EQUAL(x, y)                                             \
  {                                                                        \
    std::ostringstream __assert_private_os;                                \
    __assert_private_os << #x << " == " << #y << ", " << _FILE_NAME << ":" \
                        << __LINE__;                                       \
    AssertNotEqual(x, y, __assert_private_os.str());                       \
  }

/**
 * ???????????? `ASSERT_HINT` ?????????????????? ???????????????????? ?????????????????? `x`. ?????????????????? `x` ????????????
 * ???????????????????????????????? ?? ???????? `bool`.
 * ???????? ?????????????????? `x` ??????????, ???????? ?????????????????? ??????????????????????. ???????? ?????????????????? `x`
 * ??????????????, ???????????????????? ?????????? ????????????????????????. ???????????? `hint` ?????????? ???????????????? ?? ??????????
 * ????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    ASSERT_HINT(2 + 2 == 4, "Math works");
 *    // ?????????? 2 ?????? ???????????????????????????? ?? bool ???????????? ?????????????????? true
 *    ASSERT_HINT(2, "Boolean cast works");
 *    // ?????????? ???????? ????????????????????
 *    ASSERT_HINT(false, "False is true");
 *    string user_name = "Harry Potter"s;
 *    // ???????? ?????????????????????????????????? ?????????????????? ????????????, ?????????????????? ???? ????????????????????????????,
 *    // ?????? ?????? string ???? ?????????? ???????? ???????????????????????? ?? ???????? bool.
 *    // ASSERT_HINT(user_name, "String to bool cast works");
 *  }
 *  ```
 */
#define ASSERT_HINT(x, hint)                                        \
  {                                                                 \
    std::ostringstream __assert_private_os;                         \
    __assert_private_os << #x << " is false, " << _FILE_NAME << ":" \
                        << __LINE__;                                \
    if (!std::string(hint).empty()) {                               \
      __assert_private_os << " (" << hint << ")";                   \
    }                                                               \
    Assert(static_cast<bool>(x), __assert_private_os.str());        \
  }

/**
 * ???????????? `ASSERT` ?????????????????? ???????????????????? ?????????????????? `x`. ?????????????????? `x` ????????????
 * ???????????????????????????????? ?? ???????? `bool`.
 * ???????? ?????????????????? `x` ??????????, ???????? ?????????????????? ??????????????????????. ???????? ?????????????????? `x`
 * ??????????????, ???????????????????? ?????????? ????????????????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    ASSERT(2 + 2 == 4);
 *    ASSERT(2); // ?????????? 2 ?????? ???????????????????????????? ?? bool ???????????? ?????????????????? true
 *    ASSERT(false); // ?????????? ???????? ????????????????????
 *    string user_name = "Harry Potter"s;
 *    // ???????? ?????????????????????????????????? ?????????????????? ????????????, ?????????????????? ???? ????????????????????????????,
 *    // ?????? ?????? string ???? ?????????? ???????? ???????????????????????? ?? ???????? bool.
 *    // ASSERT(user_name);
 *  }
 *  ```
 */
#define ASSERT(x)                                                   \
  {                                                                 \
    std::ostringstream __assert_private_os;                         \
    __assert_private_os << #x << " is false, " << _FILE_NAME << ":" \
                        << __LINE__;                                \
    Assert(static_cast<bool>(x), __assert_private_os.str());        \
  }

/**
 * ???????????? `RUN_TEST` ???????????? ?????? ???????????????? ?????????????? ????????-?????????????? `func`.
 * ???????????????? `tr` ???????????? ?????? ???????????????????? ???????? `TestRunner`.
 *
 * ????????????:
 *  ```
 *  void Test1() {
 *    // ???????????????????? ????????-?????????????? ...
 *  }
 *
 *  void Test2() {
 *    // ???????????????????? ????????-?????????????? ...
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
 * ???????????? `ASSERT_THROWS_HINT` ??????????????????, ?????? ?????? ???????????????????? ?????????????????? `expr`
 * ?????????? ?????????????????? ???????????????????? ???????? `expected_exception`. ???????? ????????????????????
 * ?????????????????? ???? ??????????, ???????? ???????????????????? ???????????????????? ?????????????? ????????, ???????? ??????????????????
 * ??????????????????????. ???????????? `hint` ?????????? ???????????????? ?? ?????????? ????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    using namespace std;
 *    ASSERT_THROWS_HINT(stoi("not-a-number"s), invalid_argument,
 *                       "stoi(string) throws");
 *  }
 *  ```
 */
#define ASSERT_THROWS_HINT(expr, expected_exception, hint)                \
  {                                                                       \
    bool __assert_private_flag = true;                                    \
    try {                                                                 \
      expr;                                                               \
      __assert_private_flag = false;                                      \
    } catch (expected_exception &) {                                      \
    } catch (...) {                                                       \
      std::ostringstream __assert_private_os;                             \
      __assert_private_os << "Expression " #expr                          \
                             " threw an unexpected exception"             \
                             " " _FILE_NAME ":"                           \
                          << __LINE__;                                    \
      if (!std::string(hint).empty()) {                                   \
        __assert_private_os << " (" << hint << ")";                       \
      }                                                                   \
      Assert(false, __assert_private_os.str());                           \
    }                                                                     \
    if (!__assert_private_flag) {                                         \
      std::ostringstream __assert_private_os;                             \
      __assert_private_os << "Expression " #expr                          \
                             " is expected to throw " #expected_exception \
                             " " _FILE_NAME ":"                           \
                          << __LINE__;                                    \
      if (!std::string(hint).empty()) {                                   \
        __assert_private_os << " (" << hint << ")";                       \
      }                                                                   \
      Assert(false, __assert_private_os.str());                           \
    }                                                                     \
  }

/**
 * ???????????? `ASSERT_THROWS` ??????????????????, ?????? ?????? ???????????????????? ?????????????????? `expr` ??????????
 * ?????????????????? ???????????????????? ???????? `expected_exception`.
 * ???????? ???????????????????? ?????????????????? ???? ??????????, ???????? ???????????????????? ???????????????????? ?????????????? ????????,
 * ???????? ?????????????????? ??????????????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    using namespace std;
 *    ASSERT_THROWS(stoi("not-a-number"s), invalid_argument);
 *  }
 *  ```
 */
#define ASSERT_THROWS(expr, expected_exception) \
  ASSERT_THROWS_HINT(expr, expected_exception, "")

/**
 * ???????????? `ASSERT_DOESNT_THROW_HINT` ??????????????????, ?????? ?????? ???????????????????? ??????????????????
 * `expr` ???? ?????????? ?????????????????? ?????????????? ????????????????????. ???????? ?????? ???????????????????? ??????????????????
 * `expr` ???????????????????? ????????????????????, ???????? ?????????? ????????????????. ???????????? `hint` ??????????
 * ???????????????? ?? ?????????? ????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    vector<int> v;
 *    v.push_back(1);
 *    ASSERT_DOESNT_THROW_HINT(v.at(0)), "At within bounds doesn't throw");
 *  }
 *  ```
 */
#define ASSERT_DOESNT_THROW_HINT(expr, hint)                           \
  try {                                                                \
    expr;                                                              \
  } catch (std::exception & e) {                                       \
    std::ostringstream __assert_private_os;                            \
    __assert_private_os << "Expression " #expr                         \
                           " threw an unexpected exception"            \
                           " "                                         \
                        << e.what() << " " _FILE_NAME ":" << __LINE__; \
    if (!std::string(hint).empty()) {                                  \
      __assert_private_os << " (" << hint << ")";                      \
    }                                                                  \
    Assert(false, __assert_private_os.str());                          \
  } catch (...) {                                                      \
    std::ostringstream __assert_private_os;                            \
    __assert_private_os << "Expression " #expr                         \
                           " threw an unexpected exception"            \
                           " " _FILE_NAME ":"                          \
                        << __LINE__;                                   \
    if (!std::string(hint).empty()) {                                  \
      __assert_private_os << " (" << hint << ")";                      \
    }                                                                  \
    Assert(false, __assert_private_os.str());                          \
  }

/**
 * ???????????? `ASSERT_DOESNT_THROW` ??????????????????, ?????? ?????? ???????????????????? ?????????????????? `expr`
 * ???? ?????????? ?????????????????? ?????????????? ????????????????????.
 * ???????? ?????? ???????????????????? ?????????????????? `expr` ???????????????????? ????????????????????, ???????? ??????????
 * ????????????????.
 *
 * ????????????:
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
 * ??????????????????, ?????? ???????????????? ???????????????????? `container` ?????????? ???????????? ??????????????????
 * ??????????????.
 */
template <typename Container>
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
      os << "Assertion failed: expected container items to be in strict "
            "descending order.";
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
 * ???????????? `ASSERT_ITEMS_DESC_HINT` ??????????????????, ?????? ???????????????? `container`
 * ?????????????????? ?? ???????????? ?????????????????? ??????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    // ????????????: ???????????????? ???? ??????????????
 *    ASSERT_ITEMS_DESC_HINT((std::list<int> {3, 3}), "Bad");
 *    // ????????????: ???????????????? ????????????????????
 *    ASSERT_ITEMS_DESC_HINT((std::list<int> {3, 4}), "Bad");
 *    // ???? ????????????
 *    ASSERT_ITEMS_DESC_HINT((std::list<int> {3, 2, 1}), "Good");
 *  }
 *  ```
 */
#define ASSERT_ITEMS_DESC_HINT(container, hint)                        \
  {                                                                    \
    std::ostringstream __assert_private_os;                            \
    __assert_private_os << "Container " #container ", " _FILE_NAME ":" \
                        << __LINE__;                                   \
    if (!std::string(hint).empty()) {                                  \
      __assert_private_os << " (" << hint << ")";                      \
    }                                                                  \
    AssertItemsDescImpl(container, __assert_private_os.str());         \
  }

/**
 * ??????????????????, ?????? ?????? ???????????????? ???????????????????? `container` ??????????
 */
template <typename Container>
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
 * ???????????? `ASSERT_ITEMS_EQUAL` ??????????????????, ?????? ?????? ???????????????? `container` ??????????
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    ASSERT_ITEMS_EQUAL((std::list<int> {3, 3}));
 *    // ????????????
 *    ASSERT_ITEMS_EQUAL((std::list<int> {3, 4}));
 *  }
 *  ```
 */
#define ASSERT_ITEMS_EQUAL(container)                                  \
  {                                                                    \
    std::ostringstream __assert_private_os;                            \
    __assert_private_os << "Container " #container ", " _FILE_NAME ":" \
                        << __LINE__;                                   \
    AssertItemsEqualImpl(container, __assert_private_os.str());        \
  }

/**
 * ???????????????????? ???????????????? t ?? u. ???????? ?????????????????????? ???????????? ?????? ???? 0.0001%, ????????
 * ??????????????????????????. ???????????? hint ???????????????? ??????????????????, ?????????????? ??????????????????, ???????? ????????
 * ????????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    AssertSoftEqual(1ull, 1.000001, "ok");
 *  }
 *  ```
 */
template <class T, class U>
void AssertSoftEqual(const T &t, const U &u, const std::string &hint = {}) {
  double diff = std::abs(static_cast<double>(t) - static_cast<double>(u)) /
                std::abs(static_cast<double>(t));
  const double cutoff = 1.001e-6;
  if (diff > cutoff) {
    std::ostringstream os;
    os << std::setprecision(7);
    os << "Assertion failed: " << t << " !??? " << u;
    os << ", difference is " << (diff * 100) << "%";
    if (!hint.empty()) {
      os << " hint: " << hint;
    }
    throw std::runtime_error(os.str());
  }
}

/**
 * ???????????? `ASSERT_SOFT_EQUAL_HINT` ?????????????????? ???????????????? ?????????????????? `x` ?? `y` ????
 * ???????????? ??????????????????. ???????? ???????????????? ?????????????????????? ???????????? ?????? ???? 0.000001%, ????????
 * ?????????????????? ??????????????????????. ???????????? `hint` ?????????? ???????????????? ?? ?????????? ????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    ASSERT_SOFT_EQUAL_HINT(1ull, 1.000001, "ok");
 *    // ?????? ???????????????? ???? ??????????????????, ?? ???????? ?????????? ????????????????.
 *    ASSERT_SOFT_EQUAL_HINT(1ull, 1.00001, "bad");
 *  }
 *  ```
 */
#define ASSERT_SOFT_EQUAL_HINT(x, y, hint)                                 \
  {                                                                        \
    std::ostringstream __assert_private_os;                                \
    __assert_private_os << #x << " !??? " << #y << ", " << _FILE_NAME << ":" \
                        << __LINE__;                                       \
    if (!std::string(hint).empty()) {                                      \
      __assert_private_os << " (" << hint << ")";                          \
    }                                                                      \
    AssertSoftEqual(x, y, __assert_private_os.str());                      \
  }

/**
 * ???????????? `ASSERT_SOFT_EQUAL` ?????????????????? ???????????????? ?????????????????? `x` ?? `y` ???? ????????????
 * ??????????????????. ???????? ???????????????? ?????????????????????? ???????????? ?????? ???? 0.000001%, ???????? ??????????????????
 * ??????????????????????. ???????????? `hint` ?????????? ???????????????? ?? ?????????? ????????????.
 *
 * ????????????:
 *  ```
 *  void Test() {
 *    ASSERT_SOFT_EQUAL(1ull, 1.000001);
 *    // ?????? ???????????????? ???? ??????????????????, ?? ???????? ?????????? ????????????????.
 *    ASSERT_SOFT_EQUAL(1ull, 1.00001);
 *  }
 *  ```
 */
#define ASSERT_SOFT_EQUAL(x, y) ASSERT_SOFT_EQUAL_HINT(x, y, "")
