cmake_minimum_required(VERSION 3.20)

project(ant VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ant INTERFACE)

target_include_directories(ant INTERFACE src)

target_compile_features(ant INTERFACE cxx_std_23)