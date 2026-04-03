#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_attach_then_detach_over_tcp) {
    ScenarioConfig config;
    config.scenarioName = "component_attach_detach_tcp";
    config.transportMode = TransportMode::Tcp;
    config.linkProfile.mode = TransportMode::Tcp;
    config.linkProfile.lossPercent = 0.0;
    config.trafficProfile.pattern = TrafficPattern::ConstantBitrate;
    config.trafficProfile.durationMs = 600;
    config.trafficProfile.packetSizeBytes = 128;
    config.trafficProfile.packetsPerSecond = 5;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(result.attachSucceeded);
    ASSERT_TRUE(result.detachSucceeded);
    ASSERT_EQ(result.activeSessionsAtEnd, 0U);
}
