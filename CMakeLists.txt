# CMakeLists.txt for libedge-proto 2.0

cmake_minimum_required(VERSION 3.14)

# Attempt to disable any compiler caching (e.g., ccache)
set(CMAKE_C_COMPILER_LAUNCHER "" CACHE STRING "C compiler launcher" FORCE)
set(CMAKE_CXX_COMPILER_LAUNCHER "" CACHE STRING "CXX compiler launcher" FORCE)

project(libedge-proto C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(LIBEDGE_BUILD_TESTS "Build tests for the library" ON)

# Find all source files for the main library
file(GLOB_RECURSE LIB_SOURCES 
    "src/core/*.c"
    "src/common/*.c"
    "src/protocols/modbus/*.c"
    "src/protocols/dlms/*.c"
    "src/protocols/dlt645/*.c"
    "src/protocols/dlt698/*.c"
    "src/protocols/iec104/*.c"
)

add_library(edge_proto STATIC ${LIB_SOURCES})

target_include_directories(edge_proto PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_include_directories(edge_proto PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(LIBEDGE_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        cmocka
        GIT_REPOSITORY https://git.cryptomilk.org/projects/cmocka.git
        GIT_TAG        cmocka-1.1.5
    )
    FetchContent_MakeAvailable(cmocka)

    # Core tests
    add_executable(run_core_tests tests/test_core.c)
    target_link_libraries(run_core_tests PRIVATE edge_proto cmocka)
    add_test(NAME core_tests COMMAND run_core_tests)
    
    # Modbus tests
    add_executable(run_modbus_tests tests/test_modbus.c)
    target_link_libraries(run_modbus_tests PRIVATE edge_proto cmocka)
    add_test(NAME modbus_tests COMMAND run_modbus_tests)

endif()
