#pragma once

#include "miniran/simulation/scenario_config.h"
#include "miniran/simulation/simulation_result.h"

namespace miniran {

class ScenarioRunner {
public:
    explicit ScenarioRunner(ScenarioConfig config);

    SimulationResult run();

private:
    ScenarioConfig config_{};
};

}  // namespace miniran
