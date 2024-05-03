cmake_minimum_required(VERSION 3.16)
project(test_json_view)

add_executable(json_view_test test.cpp)
target_compile_features(json_view_test PRIVATE cxx_std_20)
