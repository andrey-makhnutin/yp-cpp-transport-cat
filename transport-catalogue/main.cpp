#include <iostream>
#include <string_view>
#include <vector>

#include "geo.h"
#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;
using namespace trans_cat;

int main() {
  TransportCatalogue cat;

  {
    using namespace input_reader;
    using from_char_stream::DbReader;

    DbReader input { cin };
    for (const AddStopCmd &add_stop : input.GetAddStopCmds()) {
      cat.AddStop(add_stop.name, add_stop.coordinates.lat,
                  add_stop.coordinates.lng);
    }
    for (const AddStopCmd &add_stop : input.GetAddStopCmds()) {
      for (auto dis : add_stop.distances) {
        cat.SetDistance(add_stop.name, dis.first, dis.second);
      }
    }
    for (const AddBusCmd &bus : input.GetAddBusCmds()) {
      cat.AddBus(bus.name, bus.route_type, bus.stop_names);
    }
  }

  {
    using stat_reader::from_char_stream::StatsRequestProcessor;
    using stat_reader::to_char_stream::StatsPrinter;

    StatsRequestProcessor request_processor { cin };
    StatsPrinter stats_printer { cout };
    request_processor.ProcessRequests(cat, stats_printer);
  }
}
