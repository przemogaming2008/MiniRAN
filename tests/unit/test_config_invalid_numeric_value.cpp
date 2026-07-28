#include <cstdio>
#include <fstream>
#include <string>

#include "miniran/simulation/scenario_config.h"
#include "support/test_framework.h"

using namespace miniran;

namespace {

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    file << content;
}

void assertConfigFails(const std::string& path, const std::string& content, const std::string& expectedErrorPart) {
    writeFile(path, content);

    std::string error;
    const auto config = ScenarioConfig::fromFile(path, error);

    std::remove(path.c_str());

    ASSERT_TRUE(!config.has_value());
    ASSERT_TRUE(error.find(expectedErrorPart) != std::string::npos);
}

}  // namespace

TEST_CASE(config_invalid_text_numeric_value_returns_error) {
    assertConfigFails(
        "tmp_invalid_numeric_text.cfg",
        "step_ms = abc\n",
        "step_ms"
    );
}

TEST_CASE(config_numeric_value_with_trailing_text_returns_error) {
    assertConfigFails(
        "tmp_invalid_numeric_trailing.cfg",
        "step_ms = 10junk\n",
        "step_ms"
    );
}

TEST_CASE(config_negative_unsigned_value_returns_error) {
    assertConfigFails(
        "tmp_invalid_numeric_negative.cfg",
        "step_ms = -1\n",
        "step_ms"
    );
}

TEST_CASE(config_uint32_overflow_returns_error) {
    assertConfigFails(
        "tmp_invalid_uint32_overflow.cfg",
        "ue_id = 4294967297\n",
        "ue_id"
    );
}

TEST_CASE(config_zero_step_ms_returns_error) {
    assertConfigFails(
        "tmp_invalid_zero_step.cfg",
        "step_ms = 0\n",
        "step_ms"
    );
}

TEST_CASE(config_unknown_key_returns_error) {
    assertConfigFails(
        "tmp_invalid_unknown_key.cfg",
        "typo_step_ms = 10\n",
        "Unknown config key"
    );
}

TEST_CASE(config_duplicate_key_returns_error) {
    assertConfigFails(
        "tmp_invalid_duplicate_key.cfg",
        "step_ms = 10\nstep_ms = 20\n",
        "Duplicate config key"
    );
}

TEST_CASE(config_invalid_transport_mode_returns_error) {
    assertConfigFails(
        "tmp_invalid_transport_mode.cfg",
        "transport_mode = spaceship\n",
        "transport_mode"
    );
}

TEST_CASE(config_invalid_traffic_pattern_returns_error) {
    assertConfigFails(
        "tmp_invalid_traffic_pattern.cfg",
        "traffic_pattern = randomstorm\n",
        "traffic_pattern"
    );
}