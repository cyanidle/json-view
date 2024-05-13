cmake_minimum_required(VERSION 3.16)
project(test_json_view)

option(JSON_VIEW_TEST "build tests" OFF)

add_library(json_view INTERFACE)
target_compile_features(json_view INTERFACE cxx_std_20)
target_include_directories(json_view INTERFACE .)

if (JSON_VIEW_TEST)
    add_executable(json_view_test test.cpp)
    target_link_libraries(json_view_test PRIVATE json_view)
endif()
