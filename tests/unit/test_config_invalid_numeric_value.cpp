#include <fstream>
#include <string>
#include "miniran/simulation/scenario_config.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(config_invalid_numeric_value_returns_error){
    const std::string path = "invalid_numeric.cfg";

    {
        std::ofstream file(path);
        file << "step_ms = abc\n";
    }

    std::string error;
    auto config = ScenarioConfig::fromFile(path, error);

    ASSERT_TRUE(!config.has_value());
    ASSERT_TRUE(error.find("step_ms") != std::string::npos);
}