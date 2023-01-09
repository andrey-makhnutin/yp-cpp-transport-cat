cmake_minimum_required(VERSION 3.25)

project(transport-cat CXX)
set(CMAKE_CXX_STANDARD 17)

set(CODE_FILES
    transport-catalogue/domain.h
    transport-catalogue/geo.h
    transport-catalogue/graph.h
    transport-catalogue/input_reader.h transport-catalogue/input_reader.cpp
    transport-catalogue/json_builder.h transport-catalogue/json_builder.cpp
    transport-catalogue/json_reader.h transport-catalogue/json_reader.cpp
    transport-catalogue/json.h transport-catalogue/json.cpp
    transport-catalogue/map_renderer.h transport-catalogue/map_renderer.cpp
    transport-catalogue/ranges.h
    transport-catalogue/request_handler.h transport-catalogue/request_handler.cpp
    transport-catalogue/router.h
    transport-catalogue/shapes.h transport-catalogue/shapes.cpp
    transport-catalogue/stat_reader.h transport-catalogue/stat_reader.cpp
    transport-catalogue/svg.h transport-catalogue/svg.cpp
    transport-catalogue/transport_catalogue.h transport-catalogue/transport_catalogue.cpp
    transport-catalogue/transport_router.h transport-catalogue/transport_router.cpp
)

add_executable(transport-cat ${CODE_FILES} transport-catalogue/main.cpp)

set(TEST_FILES
    tests/geo.h tests/geo.cpp
    tests/input_reader.h tests/input_reader.cpp
    tests/json_builder.h tests/json_builder.cpp
    tests/json_reader.h tests/json_reader.cpp
    tests/json.h tests/json.cpp
    tests/map_renderer.h tests/map_renderer.cpp
    tests/request_handler.h tests/request_handler.cpp
    tests/shapes.h tests/shapes.cpp
    tests/stat_reader.h tests/stat_reader.cpp
    tests/svg.h tests/svg.cpp
    tests/test_framework.h
    tests/transport_catalogue.h tests/transport_catalogue.cpp
)

add_executable(tests ${CODE_FILES} ${TEST_FILES} tests/main.cpp)
