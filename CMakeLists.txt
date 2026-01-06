# CMakeLists.txt for libedge-proto 6.5 (Industrial Grade)

cmake_minimum_required(VERSION 3.14)
project(libedge-proto C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(LIBEDGE_BUILD_TESTS "Build tests" ON)

set(LIB_SOURCES
    src/core/edge_vector.c
    src/core/edge_cursor.c
    src/common/crc.c
    src/protocols/modbus/mb_pdu.c
    src/protocols/modbus/mb_slave.c
    src/protocols/dlms/dlms_axdr.c
    src/protocols/dlms/dlms_encoder.c
    src/protocols/dlms/dlms_hdlc.c
    src/protocols/dlms/dlms_cosem.c
    src/protocols/dlms/dlms_security.c
    src/protocols/dlms/dlms_block.c
    src/protocols/dlms/dlms_server.c
    src/protocols/dlms/dlms_ic.c
    src/protocols/dlt645/dlt645_codec.c
    src/protocols/dlt698/dlt698_codec.c
    src/protocols/dlt698/dlt698_apdu.c
    src/protocols/iec104/iec104_apci.c
    src/protocols/iec104/iec104_asdu.c
    src/protocols/iec104/iec104_session.c
    src/protocols/dnp3/dnp3_link.c
    src/protocols/dnp3/dnp3_link_rx.c
    src/protocols/dnp3/dnp3_transport.c
    src/protocols/dnp3/dnp3_app.c
)

add_library(edge_proto STATIC ${LIB_SOURCES})
target_include_directories(edge_proto PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(edge_proto PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(LIBEDGE_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(cmocka GIT_REPOSITORY https://git.cryptomilk.org/projects/cmocka.git GIT_TAG cmocka-1.1.5)
    FetchContent_MakeAvailable(cmocka)

    macro(add_proto_test NAME SRC)
        add_executable(${NAME} ${SRC})
        target_link_libraries(${NAME} PRIVATE edge_proto cmocka m)
        add_test(NAME ${NAME} COMMAND ${NAME})
    endmacro()

    add_proto_test(test_core tests/test_core.c)
    add_proto_test(test_dlms tests/test_dlms_expert.c)
    add_proto_test(test_dnp3 tests/test_dnp3_expert.c)
    add_proto_test(test_iec104 tests/test_iec104_expert.c)
endif()