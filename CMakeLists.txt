# CMakeLists.txt for libedge-proto

cmake_minimum_required(VERSION 3.14)

# Attempt to disable any compiler caching (e.g., ccache)
set(CMAKE_C_COMPILER_LAUNCHER "" CACHE STRING "C compiler launcher" FORCE)
set(CMAKE_CXX_COMPILER_LAUNCHER "" CACHE STRING "CXX compiler launcher" FORCE)

project(libedge-proto C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ##############################################################################
# # Project Options
# ##############################################################################
option(LIBEDGE_BUILD_TESTS "Build tests for the library" ON)

# ##############################################################################
# # Library Definition
# ##############################################################################

add_library(edge_proto STATIC
    src/common/crc.c
    src/modbus/modbus_codec.c
    src/dlt645/dlt645_2007.c
    src/dlt698/dlt698_codec.c
    src/iec104/iec104_codec.c
    src/dlms/dlms_codec.c
)

target_include_directories(edge_proto PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_include_directories(edge_proto PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# ##############################################################################
# # Testing Configuration (cmocka)
# ##############################################################################
if(LIBEDGE_BUILD_TESTS)
    enable_testing()

    # Fetch cmocka using FetchContent
    include(FetchContent)
    FetchContent_Declare(
        cmocka
        GIT_REPOSITORY https://git.cryptomilk.org/projects/cmocka.git
        GIT_TAG        cmocka-1.1.5
    )
    FetchContent_MakeAvailable(cmocka)

    # === CRC Tests ===
    add_executable(run_crc_tests
        tests/test_main.c
    )
    target_include_directories(run_crc_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(run_crc_tests PRIVATE edge_proto cmocka)
    add_test(NAME crc_tests COMMAND run_crc_tests)

    # === Modbus Tests ===
    add_executable(run_modbus_tests
        tests/test_modbus.c
    )
    target_link_libraries(run_modbus_tests PRIVATE edge_proto cmocka)
    add_test(NAME modbus_tests COMMAND run_modbus_tests)

    # === DLT645 Tests ===
    add_executable(run_dlt645_tests
        tests/test_dlt645.c
    )
    target_link_libraries(run_dlt645_tests PRIVATE edge_proto cmocka)
    add_test(NAME dlt645_tests COMMAND run_dlt645_tests)

    # === DLT698 Tests ===
    add_executable(run_dlt698_tests
        tests/test_dlt698.c
    )
    target_link_libraries(run_dlt698_tests PRIVATE edge_proto cmocka)
    add_test(NAME dlt698_tests COMMAND run_dlt698_tests)

    # === IEC104 Tests ===
    add_executable(run_iec104_tests
        tests/test_iec104.c
    )
    target_link_libraries(run_iec104_tests PRIVATE edge_proto cmocka)
    add_test(NAME iec104_tests COMMAND run_iec104_tests)

    # === DLMS Tests ===
    add_executable(run_dlms_tests
        tests/test_dlms.c
    )
    target_link_libraries(run_dlms_tests PRIVATE edge_proto cmocka)
    add_test(NAME dlms_tests COMMAND run_dlms_tests)

endif()
