#include "../transport-catalogue/transport_catalogue.h"
#include "transport_catalogue.h"

#include <stdexcept>

#include "test_framework.h"

using namespace std;

namespace transport_catalogue {

void TestAddStop() {
  TransportCatalogue tc;

  tc.AddStop("Rasskazovka"s, { 55.632761, 37.333324 });

  // AddBus должна упасть, т.к. остановки Marushkino ещё нет
  ASSERT_THROWS(tc.AddBus("750"s, RouteType::LINEAR, { "Rasskazovka"s,
                              "Marushkino"s }),
                invalid_argument);
  tc.AddStop("Marushkino"s, { 55.595884, 37.209755 });

  // теперь AddBus должна сработать, т.к. остановка Marushkino была добавлена
  ASSERT_DOESNT_THROW(tc.AddBus("750"s, RouteType::LINEAR, { "Rasskazovka"s,
                                    "Marushkino"s }));

  // запрещаем дважды добавлять одну и ту же остановку (пока не требуется уметь обновлять данные)
  ASSERT_THROWS(tc.AddStop("Marushkino"s, { 55.595884, 37.209755 }),
                invalid_argument);
}

void TestAddBus() {
  TransportCatalogue tc;

  {
    // GetBusStats долен вернуть пустоту, т.к. такого маршрута пока нет
    auto bi = tc.GetBusStats("750"sv);
    ASSERT(!bi.has_value());
  }

  tc.AddStop("Rasskazovka"s, { 55.632761, 37.333324 });
  tc.AddStop("Marushkino"s, { 55.595884, 37.209755 });
  tc.AddBus("750"s, RouteType::LINEAR, { "Rasskazovka"s, "Marushkino"s });

  {
    // GetBusStats должен вернуть непустой объект, т.к. такой маршрут появился
    auto bi = tc.GetBusStats("750"sv);
    ASSERT(bi.has_value());
  }

  // допускаем случай, когда остановка идёт несколько раз подряд
  ASSERT_DOESNT_THROW(tc.AddBus("751"s, RouteType::LINEAR, { "Rasskazovka"s,
                                    "Marushkino"s, "Marushkino"s }));

  // в кольцевых маршрутах первая и последняя остановки должны совпадать
  ASSERT_THROWS(tc.AddBus("752"s, RouteType::CIRCULAR, { "Rasskazovka"s,
                              "Marushkino"s }),
                invalid_argument);
  ASSERT_DOESNT_THROW(tc.AddBus("752"s, RouteType::CIRCULAR, { "Rasskazovka"s,
                                    "Marushkino"s, "Rasskazovka"s }));

  // запрещаем дважды добавлять один и тот же маршрут (пока не требуется уметь обновлять данные)
  ASSERT_THROWS(tc.AddBus("750"s, RouteType::LINEAR, { "Rasskazovka"s,
                              "Marushkino"s }),
                invalid_argument);
}

void TestSetDistance() {
  TransportCatalogue tc;

  // ругаемся, если остановку ещё не добавили в справочник
  ASSERT_THROWS(tc.SetDistance("A"s, "B"s, 1123), invalid_argument);
  tc.AddStop("A"s, { 55.632761, 37.333324 });
  ASSERT_THROWS(tc.SetDistance("A"s, "B"s, 1123), invalid_argument);
  tc.AddStop("B"s, { 55.632761, 37.3492554327 });
  tc.AddBus("1"s, RouteType::LINEAR, { "A"s, "B"s });
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
    tc.AddStop("Rasskazovka"s, { 55.632761, 37.333324 });
    tc.AddStop("Marushkino"s, { 55.632761, 37.3492554327 });  // километр на восток
    tc.AddStop("Tolstopaltsevo"s, { 55.6417542160555, 37.3492554327 });  // километр на юг
    tc.AddStop("Biryulyovo Zapadnoye"s, { 55.632761, 37.3492554327 });  // километр на запад
    tc.SetDistance("Marushkino"sv, "Tolstopaltsevo"sv, 1001);  // + 1 (есть расстояние в ту сторону)
    tc.SetDistance("Tolstopaltsevo"sv, "Marushkino"sv, 1004);  // + 4 (есть расстояние в ту сторону)
    tc.SetDistance("Tolstopaltsevo"sv, "Biryulyovo Zapadnoye"sv, 1016);  // + 16 + 16 (есть расстояние в ту или обратную сторону)
    tc.AddBus("750"s, RouteType::LINEAR, { "Rasskazovka"s, "Marushkino"s,
                  "Tolstopaltsevo"s, "Biryulyovo Zapadnoye"s });
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
    tc.AddStop("Rasskazovka"s, { 55.632761, 37.333324 });
    tc.AddStop("Marushkino"s, { 55.632761, 37.3492554327 });  // километр на восток
    tc.AddStop("Tolstopaltsevo"s, { 55.6417542160555, 37.3492554327 });  // километр на юг
    tc.AddStop("Biryulyovo Zapadnoye"s, { 55.632761, 37.3492554327 });  // километр на запад
    tc.SetDistance("Marushkino"sv, "Tolstopaltsevo"sv, 1001);  // + 1 (есть расстояние в ту сторону)
    tc.SetDistance("Tolstopaltsevo"sv, "Marushkino"sv, 1004);  // + 0 (не должно быть учтено)
    tc.SetDistance("Biryulyovo Zapadnoye"sv, "Tolstopaltsevo"sv, 1016);  // + 16 (есть расстояние в обратную сторону)
    tc.AddBus("750"s, RouteType::CIRCULAR,
              { "Rasskazovka"s, "Marushkino"s, "Tolstopaltsevo"s,
                  "Biryulyovo Zapadnoye"s, "Rasskazovka"s });
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

    tc.AddStop("A"s, { 1, 1 });
    tc.AddStop("B"s, { 1, 1 });
    tc.AddStop("C"s, { 1, 1 });
    tc.AddStop("D"s, { 1, 1 });
    tc.AddStop("E"s, { 1, 1 });
    tc.AddBus("Bus3"s, RouteType::LINEAR, { "C"s, "D"s });
    tc.AddBus("Bus1"s, RouteType::LINEAR, { "A"s, "B"s, "A"s });
    tc.AddBus("Bus2"s, RouteType::LINEAR, { "D"s, "C"s, "B"s, "B"s });

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
