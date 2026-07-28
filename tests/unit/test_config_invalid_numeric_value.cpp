#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

#include "miniran/simulation/scenario_config.h"
#include "support/test_framework.h"

using namespace miniran;

namespace
{

std::filesystem::path makeTemporaryConfigPath(const std::string& testName)
{
    const auto now =
        std::chrono::steady_clock::now().time_since_epoch().count();

    const auto dir = std::filesystem::temp_directory_path() / "miniran-tests";
    std::filesystem::create_directories(dir);

    return dir / (testName + "-" + std::to_string(now) + ".cfg");
}

class TemporaryConfigFile
{
public:
    TemporaryConfigFile(const std::string& testName,
                        const std::string& content)
        : path_(makeTemporaryConfigPath(testName))
    {
        std::ofstream file(path_);
        if (!file) {
            throw std::runtime_error("Cannot create temporary config file");
        }

        file << content;
    }

    ~TemporaryConfigFile()
    {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    const std::filesystem::path& path() const
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void assertConfigFails(const std::string& testName,
                       const std::string& content,
                       const std::string& expectedErrorPart)
{
    TemporaryConfigFile configFile(testName, content);

    std::string error;
    const auto config =
        ScenarioConfig::fromFile(configFile.path().string(), error);

    ASSERT_TRUE(!config.has_value());
    ASSERT_TRUE(error.find(expectedErrorPart) != std::string::npos);
}

}  // namespace

TEST_CASE(config_invalid_text_numeric_value_returns_error)
{
    assertConfigFails(
        "invalid_numeric_text",
        "step_ms = abc\n",
        "step_ms"
    );
}

TEST_CASE(config_numeric_value_with_trailing_text_returns_error)
{
    assertConfigFails(
        "invalid_numeric_trailing",
        "step_ms = 10junk\n",
        "step_ms"
    );
}

TEST_CASE(config_negative_unsigned_value_returns_error)
{
    assertConfigFails(
        "invalid_numeric_negative",
        "step_ms = -1\n",
        "step_ms"
    );
}

TEST_CASE(config_uint32_overflow_returns_error)
{
    assertConfigFails(
        "invalid_uint32_overflow",
        "ue_id = 4294967297\n",
        "ue_id"
    );
}

TEST_CASE(config_zero_step_ms_returns_error)
{
    assertConfigFails(
        "invalid_zero_step",
        "step_ms = 0\n",
        "step_ms"
    );
}

TEST_CASE(config_unknown_key_returns_error)
{
    assertConfigFails(
        "invalid_unknown_key",
        "typo_step_ms = 10\n",
        "Unknown config key"
    );
}

TEST_CASE(config_duplicate_key_returns_error)
{
    assertConfigFails(
        "invalid_duplicate_key",
        "step_ms = 10\nstep_ms = 20\n",
        "Duplicate config key"
    );
}

TEST_CASE(config_invalid_transport_mode_returns_error)
{
    assertConfigFails(
        "invalid_transport_mode",
        "transport_mode = spaceship\n",
        "transport_mode"
    );
}

TEST_CASE(config_invalid_traffic_pattern_returns_error)
{
    assertConfigFails(
        "invalid_traffic_pattern",
        "traffic_pattern = randomstorm\n",
        "traffic_pattern"
    );
}