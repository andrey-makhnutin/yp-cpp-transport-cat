#include <iostream>

#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"

using namespace std;
using namespace transport_catalogue;

int main() {
  using transport_catalogue::request_handler::BufferingRequestHandler;
  TransportCatalogue transport_catalogue;
  json_reader::BufferingRequestReader request_reader{cin};
  BufferingRequestHandler request_handler{transport_catalogue, request_reader};
  json_reader::ResponsePrinter response_printer{cout};

  request_handler.ProcessRequests(response_printer);
}
