#include <iostream>

#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"

using namespace std;
using namespace transport_catalogue;

int main() {
  TransportCatalogue transport_catalogue;
  json_reader::BufferingRequestReader request_reader { cin };
  json_reader::ResponsePrinter response_printer { cout };

  request_handler::ProcessRequests(transport_catalogue, request_reader,
                                   response_printer);
}
