cmake_minimum_required(VERSION 3.20)

project(ant_tests LANGUAGES CXX)

find_package(GTest REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../src)

add_executable(test_connection test_connection.cpp)
target_link_libraries(test_connection GTest::gtest GTest::gtest_main)

add_executable(test_signal test_signal.cpp)
target_link_libraries(test_signal GTest::gtest GTest::gtest_main)

add_executable(test_auto_disconnect test_auto_disconnect.cpp)
target_link_libraries(test_auto_disconnect GTest::gtest GTest::gtest_main)

add_executable(test_functional test_functional.cpp)
target_link_libraries(test_functional GTest::gtest GTest::gtest_main)

add_executable(run_all_tests 
    test_connection.cpp 
    test_signal.cpp 
    test_auto_disconnect.cpp 
    test_functional.cpp
)
target_link_libraries(run_all_tests GTest::gtest GTest::gtest_main)

enable_testing()
add_test(NAME ConnectionTests COMMAND test_connection)
add_test(NAME SignalTests COMMAND test_signal)
add_test(NAME AutoDisconnectTests COMMAND test_auto_disconnect)
add_test(NAME FunctionalTests COMMAND test_functional)
add_test(NAME AllTests COMMAND run_all_tests)