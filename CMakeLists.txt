cmake_minimum_required(VERSION 3.16)
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
target_compile_options(miniran_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(miniran_cli src/main.cpp)
target_link_libraries(miniran_cli PRIVATE miniran_lib)

if(MINIRAN_BUILD_TESTS)
    enable_testing()
    add_executable(miniran_tests
        tests/test_main.cpp
        tests/unit/test_frame_codec.cpp
        tests/unit/test_traffic_generator.cpp
        tests/unit/test_link_profile.cpp
        tests/unit/test_virtual_network.cpp
        tests/unit/test_session_manager.cpp
        tests/unit/test_duplicate_attach_request.cpp
        tests/unit/test_data_after_detach.cpp
        tests/unit/test_detach_after_session_expiration.cpp
        tests/unit/test_reattach_after_released.cpp
        
        tests/component/test_attach_detach.cpp
        tests/component/test_tcp_traffic.cpp
        tests/component/test_udp_stability.cpp
    )
    target_link_libraries(miniran_tests PRIVATE miniran_lib)
    target_include_directories(miniran_tests PRIVATE tests)
    add_test(NAME miniran_tests COMMAND miniran_tests)
endif()
