#include "input_reader.h"

#include <stddef.h>
#include <stdlib.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>

using namespace std;

namespace trans_cat::input_reader {

namespace from_char_stream {

namespace detail {
/**
 * Делит строку на части разделённые строкой `by`.
 * Отрезаются все пробельные символы вокруг выделенных частей текста.
 * Если текст начинается с разделителя, первым элементом будет пустая строка.
 * Если текст кончается разделителем, последним элементом также будет пустая строка.
 *
 * `string_view` в результирующем векторе ссылаются на ту же строку, на какую ссылается
 * параметр `string_view line`.
 */
vector<string_view> SplitNoWS(string_view line, const string_view by) {
  vector<string_view> parts;
  do {
    size_t begin = 0;
    // убираем пробелы в начале строки
    for (; begin < line.size() && isspace(line[begin]); ++begin)
      ;

    size_t delim_i = min(line.find(by, begin), line.size());
    // end - "символ" после последнего символа выделяемой строки, т.е. ему можно быть равным line.size()
    size_t end = delim_i;
    // убираем пробелы между окончанием строки и разделителем
    // здесь могли бы быть проблемы, если end == 0 или end == begin.
    // begin не может быть меньше 0 и больше line.size(),
    // поэтому при end > begin всегда имеем end > 0
    for (; end > begin && isspace(line[end - 1]); --end)
      ;
    // здесь возможна ситуация, когда end == begin, и это ок

    // если разделитель - пробельный символ, и мы добрались до конца строки,
    // не добавляем пустую строку в конец списка
    if (by.size() == 1 && isspace(by[0]) && delim_i == line.size()
        && end == begin) {
    } else {
      parts.push_back(line.substr(begin, end - begin));
    }
    // если строка кончается разделителем, добавляем пустую строку перед выходом
    if (delim_i == line.size() - by.size()) {
      parts.push_back(line.substr(line.size(), 0));
    }
    line.remove_prefix(min(delim_i + by.size(), line.size()));
  } while (line.size() > 0);
  return parts;
}

/**
 * Делит строку на части разделённые строкой `by`.
 * Отрезаются все пробельные символы вокруг выделенных частей текста.
 * Если текст начинается с разделителя, первым элементом будет пустая строка.
 * Если текст кончается разделителем, последним элементом также будет пустая строка.
 *
 * `string_view` в результирующем векторе ссылаются на ту же строку, на какую ссылается
 * параметр `string_view line`.
 */
vector<string_view> SplitNoWS(string_view line, char by) {
  return SplitNoWS(line, string_view { &by, 1 });
}

/**
 * Парсит команду на добавление остановки.
 *
 * `string_view` в результирующей структуре ссылаются на ту же строку, на какую ссылается
 * параметр `string_view line`.
 */
AddStopCmd ParseAddStopCmd(string_view line) {
  auto parts1 = SplitNoWS(line, ':');
  assert(parts1.size() == 2);
  auto parts2 = SplitNoWS(parts1[1], ',');
  assert(parts2.size() >= 2);
  char *endp = nullptr;
  double lat = strtod(parts2[0].data(), &endp);
  double lng = strtod(parts2[1].data(), &endp);
  vector<AddStopCmd::Distance> distances;
  distances.reserve(parts2.size() - 2);
  for (size_t i = 2; i < parts2.size(); ++i) {
    auto dis_parts = SplitNoWS(parts2[i], " to "sv);
    assert(dis_parts.size() == 2);
    auto len_part = dis_parts[0];
    assert(len_part.size() > 0 && len_part[len_part.size() - 1] == 'm');
    unsigned int dis = 0;
    from_chars(len_part.data(), len_part.data() + len_part.size() - 1, dis);
    distances.emplace_back(dis_parts[1], dis);
  }
  return {parts1[0], {lat, lng}, move(distances)};
}

/**
 * Парсит команду на добавление маршрута.
 *
 * `string_view` в результирующей структуре ссылаются на ту же строку, на какую ссылается
 * параметр `string_view line`.
 */
AddBusCmd ParseAddBusCmd(string_view line) {
  auto parts1 = SplitNoWS(line, ':');
  assert(parts1.size() == 2);
  string_view name = parts1[0];
  string_view stops = parts1[1];
  size_t marker_pos = stops.find_first_of(">-"sv);
  assert(marker_pos != stops.npos);
  char marker = stops[marker_pos];
  RouteType route_type =
      marker == '-' ? RouteType::LINEAR : RouteType::CIRCULAR;

  return {
    name,
    route_type,
    SplitNoWS(stops, marker),
  };
}

}  // namespace trans_cat::input_reader::from_char_stream::detail

/**
 * Парсит команды из символьного потока `sin_` и складывает результат
 * в векторы `add_stop_cmds_` и `add_bus_cmds_`.
 */
void DbReader::Parse() {
  using detail::ParseAddStopCmd;
  using detail::ParseAddBusCmd;

  int cmd_count;
  sin_ >> cmd_count;
  while (cmd_count--) {
    string cmd;
    sin_ >> cmd;
    if (cmd == "Stop"s) {
      string line;
      getline(sin_ >> ws, line);
      const string &line_internal = raw_strings_.emplace_back(move(line));
      add_stop_cmds_.push_back(ParseAddStopCmd(string_view { line_internal }));
    } else if (cmd == "Bus"s) {
      string line;
      getline(sin_ >> ws, line);
      const string &line_internal = raw_strings_.emplace_back(move(line));
      add_bus_cmds_.push_back(ParseAddBusCmd(string_view { line_internal }));
    }
  }
}

}  // namespace trans_cat::input_reader::from_char_stream

}  // namespace trans_cat::input_reader
