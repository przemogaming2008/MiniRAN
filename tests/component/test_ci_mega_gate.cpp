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

void assertSameScenarioResult(const SimulationResult& expected, const SimulationResult& actual) {
    ASSERT_EQ(actual.attachSucceeded, expected.attachSucceeded);
    ASSERT_EQ(actual.trafficStarted, expected.trafficStarted);
    ASSERT_EQ(actual.detachSucceeded, expected.detachSucceeded);

    ASSERT_TRUE(actual.finalUeState == expected.finalUeState);

    ASSERT_EQ(actual.packetsGenerated, expected.packetsGenerated);
    ASSERT_EQ(actual.bytesGenerated, expected.bytesGenerated);

    ASSERT_EQ(actual.packetsDeliveredToCore, expected.packetsDeliveredToCore);
    ASSERT_EQ(actual.bytesDeliveredToCore, expected.bytesDeliveredToCore);

    ASSERT_EQ(actual.packetsDroppedInNetwork, expected.packetsDroppedInNetwork);
    ASSERT_EQ(actual.packetsDeliveredByNetwork, expected.packetsDeliveredByNetwork);

    ASSERT_EQ(actual.activeSessionsAtEnd, expected.activeSessionsAtEnd);
    ASSERT_EQ(actual.expiredSessions, expected.expiredSessions);

    ASSERT_EQ(actual.totalDurationMs, expected.totalDurationMs);
}

double offeredThroughputMbps(const SimulationResult& result, std::uint64_t durationMs) {
    return (durationMs == 0)
               ? 0.0
               : (static_cast<double>(result.bytesGenerated) * 8.0) /
                     (static_cast<double>(durationMs) / 1000.0) /
                     1'000'000.0;
}

}  // namespace

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

    const double offeredMbps = offeredThroughputMbps(result, 1800);
    const double linkLimitMbps = 0.256;

    ASSERT_TRUE(offeredMbps > linkLimitMbps);
    ASSERT_TRUE(result.packetsDroppedInNetwork > 0);
    ASSERT_TRUE(result.throughputMbps < offeredMbps);
}

TEST_CASE(component_ci_repeated_tcp_runs_are_deterministic) {
    const auto reference = runScenarioFile("scenarios/ci_tcp_heavy.cfg");
    assertScenarioHealthy(reference);

    for (int i = 0; i < 20; ++i) {
        const auto result = runScenarioFile("scenarios/ci_tcp_heavy.cfg");
        assertScenarioHealthy(result);
        assertSameScenarioResult(reference, result);
    }
}