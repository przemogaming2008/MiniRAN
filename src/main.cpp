#include <iostream>
#include <string>

#include "miniran/simulation/scenario_config.h"
#include "miniran/simulation/scenario_runner.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: miniran_cli <scenario-config>\n";
        return 1;
    }

    std::string error;
    const auto config = miniran::ScenarioConfig::fromFile(argv[1], error);
    if (!config) {
        std::cerr << "Config error: " << error << '\n';
        return 2;
    }

    miniran::ScenarioRunner runner(*config);
    const auto result = runner.run();
    std::cout << result.summary();

    return (result.attachSucceeded && result.detachSucceeded) ? 0 : 3;
}
