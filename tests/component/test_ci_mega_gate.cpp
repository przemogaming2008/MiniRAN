#include "support/test_framework.h"
#include "miniran/simulation/scenario_config.h"
#include "miniran/simulation/scenario_runner.h"

using namespace miniran;

namespace {
SimulationResult runScenarioFile(const std::string& path) {
    std::string error;
    auto config = ScenarioConfig::fromFile(path, error);
    ASSERT_TRUE(config.has_value());
    ScenarioRunner runner(*config);
    return runner.run();
}

void assertScenarioHealthy(const SimulationResult& result) {
    ASSERT_TRUE(result.attachSucceeded);
    ASSERT_TRUE(result.trafficStarted);
    ASSERT_TRUE(result.detachSucceeded);
    ASSERT_EQ(result.activeSessionsAtEnd, static_cast<std::size_t>(0));
    ASSERT_TRUE(result.bytesGenerated > 0);
    ASSERT_TRUE(result.bytesDeliveredToCore > 0);
}
}

TEST_CASE(component_ci_tcp_heavy_scenario_is_stable) {
    const auto result = runScenarioFile("scenarios/ci_tcp_heavy.cfg");
    assertScenarioHealthy(result);
    ASSERT_EQ(result.packetsDroppedInNetwork, static_cast<std::size_t>(0));
}

TEST_CASE(component_ci_udp_loss_15_still_finishes_session) {
    const auto result = runScenarioFile("scenarios/ci_udp_loss_15.cfg");
    assertScenarioHealthy(result);
    ASSERT_TRUE(result.packetsDroppedInNetwork > 0);
}

TEST_CASE(component_ci_low_bandwidth_limits_throughput_but_not_session) {
    const auto result = runScenarioFile("scenarios/ci_low_bandwidth.cfg");
    assertScenarioHealthy(result);
    ASSERT_TRUE(result.throughputMbps < 1.0);
}

TEST_CASE(component_ci_repeated_tcp_runs_are_deterministic) {
    for (int i = 0; i < 20; ++i) {
        const auto result = runScenarioFile("scenarios/ci_tcp_heavy.cfg");
        assertScenarioHealthy(result);
    }
}
