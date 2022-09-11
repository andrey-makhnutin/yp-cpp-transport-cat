#include "../transport-catalogue/transport_catalogue.h"
#include "transport_catalogue.h"

#include <stdexcept>

#include "test_framework.h"

using namespace std;

namespace transport_catalogue {

void TestAddStop() {
  TransportCatalogue tc;

  tc.AddStop("Rasskazovka"sv, { 55.632761, 37.333324 });
  // AddBus должна упасть, т.к. остановки Marushkino ещё нет
  ASSERT_THROWS(tc.AddBus("750"sv, RouteType::LINEAR, { "Rasskazovka"sv,
                              "Marushkino"sv }),
                invalid_argument);
  tc.AddStop("Marushkino"sv, { 55.595884, 37.209755 });
  // теперь AddBus должна сработать, т.к. остановка Marushkino была добавлена
  ASSERT_DOESNT_THROW(tc.AddBus("750"sv, RouteType::LINEAR, { "Rasskazovka"sv,
                                    "Marushkino"sv }));

  {  // проверяем, что можно передавать `string_view` смотрящую на временную строку
    string temp_str { "Tolstopaltsevo" };
    tc.AddStop(string_view { temp_str }, { 55.611087, 37.208290 });
    temp_str[0] = 'S';
    ASSERT_DOESNT_THROW(tc.AddBus("751"sv, RouteType::LINEAR, { "Rasskazovka"sv,
                                      "Marushkino"sv, "Tolstopaltsevo"sv }));
  }

  // запрещаем дважды добавлять одну и ту же остановку (пока не требуется уметь обновлять данные)
  ASSERT_THROWS(tc.AddStop("Marushkino"sv, { 55.595884, 37.209755 }),
                invalid_argument);
}

void TestAddBus() {
  TransportCatalogue tc;

  {
    // GetBusStats долен вернуть пустоту, т.к. такого маршрута пока нет
    auto bi = tc.GetBusStats("750"sv);
    ASSERT(!bi.has_value());
  }

  tc.AddStop("Rasskazovka"sv, { 55.632761, 37.333324 });
  tc.AddStop("Marushkino"sv, { 55.595884, 37.209755 });
  tc.AddBus("750"sv, RouteType::LINEAR, { "Rasskazovka"sv, "Marushkino"sv });
  {
    // GetBusStats должен вернуть непустой объект, т.к. такой маршрут появился
    auto bi = tc.GetBusStats("750"sv);
    ASSERT(bi.has_value());
  }

  // допускаем случай, когда остановка идёт несколько раз подряд
  ASSERT_DOESNT_THROW(tc.AddBus("751"sv, RouteType::LINEAR, { "Rasskazovka"sv,
                                    "Marushkino"sv, "Marushkino"sv }));

  // в кольцевых маршрутах первая и последняя остановки должны совпадать
  ASSERT_THROWS(tc.AddBus("752"sv, RouteType::CIRCULAR, { "Rasskazovka"sv,
                              "Marushkino"sv }),
                invalid_argument);
  ASSERT_DOESNT_THROW(tc.AddBus("752"sv, RouteType::CIRCULAR, { "Rasskazovka"sv,
                                    "Marushkino"sv, "Rasskazovka"sv }));

  {  // проверяем, что можно передавать `string_view` смотрящую на временную строку
    string temp_str { "753" };
    tc.AddBus(string_view { temp_str }, RouteType::LINEAR, { "Rasskazovka"sv,
                  "Marushkino"sv });
    temp_str[0] = '8';
    auto bi = tc.GetBusStats("753"sv);
    ASSERT(bi.has_value());
  }

  // запрещаем дважды добавлять один и тот же маршрут (пока не требуется уметь обновлять данные)
  ASSERT_THROWS(tc.AddBus("750"sv, RouteType::LINEAR, { "Rasskazovka"sv,
                              "Marushkino"sv }),
                invalid_argument);
}

void TestSetDistance() {
  TransportCatalogue tc;

  // ругаемся, если остановку ещё не добавили в справочник
  ASSERT_THROWS(tc.SetDistance("A"sv, "B"sv, 1123), invalid_argument);
  tc.AddStop("A"sv, { 55.632761, 37.333324 });
  ASSERT_THROWS(tc.SetDistance("A"sv, "B"sv, 1123), invalid_argument);
  tc.AddStop("B"sv, { 55.632761, 37.3492554327 });
  tc.AddBus("1"sv, RouteType::LINEAR, { "A"sv, "B"sv });
  auto bi = tc.GetBusStats("1"sv);
  // до указания реальной дистанции длина равна длине маршрута по прямой
  ASSERT_SOFT_EQUAL(bi->route_length - bi->crow_route_length, 0);
  ASSERT_SOFT_EQUAL(bi->route_length, 2000.0);
  ASSERT_DOESNT_THROW(tc.SetDistance("A"sv, "B"sv, 1123));
  bi = tc.GetBusStats("1"sv);
  // проверяем, что после указания реальной дистанции длина маршрута поменялась
  ASSERT_SOFT_EQUAL(bi->route_length, 1123 * 2);

  // нельзя указывать одну и ту же дистанцию дважды
  ASSERT_THROWS(tc.SetDistance("A"sv, "B"sv, 1123), invalid_argument);
}

void TestGetBusStats() {
  {
    TransportCatalogue tc;

    // 4 остановки образуют "квадрат" со стороной в ~1000м
    tc.AddStop("Rasskazovka"sv, { 55.632761, 37.333324 });
    tc.AddStop("Marushkino"sv, { 55.632761, 37.3492554327 });  // километр на восток
    tc.AddStop("Tolstopaltsevo"sv, { 55.6417542160555, 37.3492554327 });  // километр на юг
    tc.AddStop("Biryulyovo Zapadnoye"sv, { 55.632761, 37.3492554327 });  // километр на запад
    tc.SetDistance("Marushkino"sv, "Tolstopaltsevo"sv, 1001);  // + 1 (есть расстояние в ту сторону)
    tc.SetDistance("Tolstopaltsevo"sv, "Marushkino"sv, 1004);  // + 4 (есть расстояние в ту сторону)
    tc.SetDistance("Tolstopaltsevo"sv, "Biryulyovo Zapadnoye"sv, 1016);  // + 16 + 16 (есть расстояние в ту или обратную сторону)
    tc.AddBus("750"sv, RouteType::LINEAR, { "Rasskazovka"sv, "Marushkino"sv,
                  "Tolstopaltsevo"sv, "Biryulyovo Zapadnoye"sv });
    auto bi = tc.GetBusStats("750"sv);
    ASSERT(bi.has_value());
    // линейный маршрут A -> B -> C -> D -> C -> B -> A: 7 остановок, 4 уникальные, 6 км
    ASSERT_EQUAL(bi->stops_count, 7u);
    ASSERT_EQUAL(bi->unique_stops_count, 4u);
    ASSERT_SOFT_EQUAL(bi->route_length, 6000.0 + 37);
    ASSERT_SOFT_EQUAL(bi->crow_route_length, 6000.0);
  }

  {
    TransportCatalogue tc;

    // 4 остановки образуют "квадрат" со стороной в ~1000м
    tc.AddStop("Rasskazovka"sv, { 55.632761, 37.333324 });
    tc.AddStop("Marushkino"sv, { 55.632761, 37.3492554327 });  // километр на восток
    tc.AddStop("Tolstopaltsevo"sv, { 55.6417542160555, 37.3492554327 });  // километр на юг
    tc.AddStop("Biryulyovo Zapadnoye"sv, { 55.632761, 37.3492554327 });  // километр на запад
    tc.SetDistance("Marushkino"sv, "Tolstopaltsevo"sv, 1001);  // + 1 (есть расстояние в ту сторону)
    tc.SetDistance("Tolstopaltsevo"sv, "Marushkino"sv, 1004);  // + 0 (не должно быть учтено)
    tc.SetDistance("Biryulyovo Zapadnoye"sv, "Tolstopaltsevo"sv, 1016);  // + 16 (есть расстояние в обратную сторону)
    tc.AddBus("750"sv, RouteType::CIRCULAR,
              { "Rasskazovka"sv, "Marushkino"sv, "Tolstopaltsevo"sv,
                  "Biryulyovo Zapadnoye"sv, "Rasskazovka"sv });
    auto bi = tc.GetBusStats("750"sv);
    ASSERT(bi.has_value());
    // кольцевой маршрут A -> B -> C -> D -> A: 5 остановок, 4 уникальные, 4 км
    ASSERT_EQUAL(bi->stops_count, 5u);
    ASSERT_EQUAL(bi->unique_stops_count, 4u);
    ASSERT_SOFT_EQUAL(bi->route_length, 4000.0 + 17);
    ASSERT_SOFT_EQUAL(bi->crow_route_length, 4000.0);
  }

  {
    TransportCatalogue tc;

    auto bi = tc.GetBusStats("750"sv);
    ASSERT(!bi.has_value());
  }
}

void TestGetStopInfo() {
  {
    TransportCatalogue tc;

    tc.AddStop("A"sv, { 1, 1 });
    tc.AddStop("B"sv, { 1, 1 });
    tc.AddStop("C"sv, { 1, 1 });
    tc.AddStop("D"sv, { 1, 1 });
    tc.AddStop("E"sv, { 1, 1 });
    tc.AddBus("Bus3", RouteType::LINEAR, { "C"sv, "D"sv });
    tc.AddBus("Bus1", RouteType::LINEAR, { "A"sv, "B"sv, "A"sv });
    tc.AddBus("Bus2", RouteType::LINEAR, { "D"sv, "C"sv, "B"sv, "B"sv });

    {
      auto si = tc.GetStopInfo("A"sv);
      ASSERT(si.has_value());
      ASSERT_EQUAL(*si, (BusesForStop { "Bus1"sv }));
    }
    {
      auto si = tc.GetStopInfo("B"sv);
      ASSERT(si.has_value());
      ASSERT_EQUAL(*si, (BusesForStop { "Bus1"sv, "Bus2"sv }));
    }
    {
      auto si = tc.GetStopInfo("C"sv);
      ASSERT(si.has_value());
      ASSERT_EQUAL(*si, (BusesForStop { "Bus2"sv, "Bus3"sv }));
    }
    {
      auto si = tc.GetStopInfo("D"sv);
      ASSERT(si.has_value());
      ASSERT_EQUAL(*si, (BusesForStop { "Bus2"sv, "Bus3"sv }));
    }
    {
      auto si = tc.GetStopInfo("E"sv);
      ASSERT(si.has_value());
      ASSERT_EQUAL(*si, BusesForStop { });
    }
    {
      auto si = tc.GetStopInfo("F"sv);
      ASSERT(!si.has_value());
    }
  }
}

}

void TestTransportCatalogue(TestRunner &tr) {
  using namespace transport_catalogue;

  RUN_TEST(tr, TestAddStop);
  RUN_TEST(tr, TestAddBus);
  RUN_TEST(tr, TestSetDistance);
  RUN_TEST(tr, TestGetBusStats);
  RUN_TEST(tr, TestGetStopInfo);
}
