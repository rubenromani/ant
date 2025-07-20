cmake_minimum_required(VERSION 3.20)

project(ant_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../src)

add_executable(01_basic_usage 01_basic_usage.cpp)
add_executable(02_observer_pattern 02_observer_pattern.cpp)
add_executable(03_event_system 03_event_system.cpp)

target_compile_features(01_basic_usage PRIVATE cxx_std_23)
target_compile_features(02_observer_pattern PRIVATE cxx_std_23)
target_compile_features(03_event_system PRIVATE cxx_std_23)