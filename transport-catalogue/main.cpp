#include <iostream>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;
using namespace transport_catalogue;

int main() {
  using input_reader::from_char_stream::ReadDB;
  using stat_reader::from_char_stream::StatsRequestProcessor;
  using stat_reader::to_char_stream::StatsPrinter;

  TransportCatalogue transport_catalogue;

  ReadDB(transport_catalogue, cin);

  StatsRequestProcessor request_processor { cin };
  StatsPrinter stats_printer { cout };
  request_processor.ProcessRequests(transport_catalogue, stats_printer);
}
