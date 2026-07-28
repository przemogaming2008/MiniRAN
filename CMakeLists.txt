cmake_minimum_required(VERSION 3.21)
project(MiniRANProtoLab VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MINIRAN_BUILD_TESTS "Build tests" ON)

add_library(miniran_lib
    src/common/byte_buffer.cpp
    src/common/metrics.cpp
    src/protocol/frame_codec.cpp
    src/protocol/session_manager.cpp
    src/transport/virtual_network.cpp
    src/traffic/traffic_generator.cpp
    src/core/core_network.cpp
    src/nodes/ue.cpp
    src/nodes/access_node.cpp
    src/simulation/scenario_config.cpp
    src/simulation/simulation_result.cpp
    src/simulation/scenario_runner.cpp
)

target_include_directories(miniran_lib PUBLIC include)
if(MSVC)
    target_compile_options(miniran_lib PRIVATE /W4)
else()
    target_compile_options(miniran_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(miniran_cli src/main.cpp)
target_link_libraries(miniran_cli PRIVATE miniran_lib)

if(MINIRAN_BUILD_TESTS)
    enable_testing()
    set(MINIRAN_UNIT_TEST_SOURCES
    tests/unit/test_frame_codec.cpp
    tests/unit/test_traffic_generator.cpp
    tests/unit/test_link_profile.cpp
    tests/unit/test_virtual_network.cpp
    tests/unit/test_session_manager.cpp
    tests/unit/test_duplicate_attach_request.cpp
    tests/unit/test_data_after_detach.cpp
    tests/unit/test_detach_after_session_expiration.cpp
    tests/unit/test_reattach_after_released.cpp
    tests/unit/test_detach_in_invalid_states.cpp
    tests/unit/test_data_after_session_expiration.cpp
    tests/unit/test_data_with_wrong_session_id.cpp
    tests/unit/test_detach_with_wrong_session_id.cpp
    tests/unit/test_attach_validate.cpp
    tests/unit/test_config_invalid_numeric_value.cpp
    tests/unit/test_data_with_zero_length_payload.cpp
    )

    set(MINIRAN_COMPONENT_TEST_SOURCES
        tests/component/test_attach_detach.cpp
        tests/component/test_tcp_traffic.cpp
        tests/component/test_udp_stability.cpp
        tests/component/test_udp_higher_loss.cpp
        tests/component/test_udp_low_bandwidth_limits_throughput.cpp
    )

    set(MINIRAN_MEGA_TEST_SOURCES
        tests/component/test_ci_mega_gate.cpp
    )

    function(add_miniran_test_binary target_name)
        add_executable(${target_name} tests/test_main.cpp ${ARGN})
        target_link_libraries(${target_name} PRIVATE miniran_lib)
        target_include_directories(${target_name} PRIVATE tests)
    endfunction()

    add_miniran_test_binary(miniran_unit_tests ${MINIRAN_UNIT_TEST_SOURCES})
    add_miniran_test_binary(miniran_component_tests ${MINIRAN_COMPONENT_TEST_SOURCES})
    add_miniran_test_binary(miniran_mega_tests ${MINIRAN_MEGA_TEST_SOURCES})
    add_miniran_test_binary(miniran_tests
        ${MINIRAN_UNIT_TEST_SOURCES}
        ${MINIRAN_COMPONENT_TEST_SOURCES}
        ${MINIRAN_MEGA_TEST_SOURCES}
    )

    add_test(NAME unit_tests COMMAND miniran_unit_tests)
    set_tests_properties(unit_tests PROPERTIES
        LABELS "unit"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_test(NAME component_tests COMMAND miniran_component_tests)
    set_tests_properties(component_tests PROPERTIES
        LABELS "component"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_test(NAME mega_gate_tests COMMAND miniran_mega_tests)
    set_tests_properties(mega_gate_tests PROPERTIES
        LABELS "mega"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_test(NAME all_tests COMMAND miniran_tests)
    set_tests_properties(all_tests PROPERTIES
        LABELS "all"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

endif()
