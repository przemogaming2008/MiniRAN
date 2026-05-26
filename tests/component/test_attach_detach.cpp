#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_attach_then_detach_over_tcp) {
    ScenarioConfig config;
    config.scenarioName = "component_attach_detach_tcp";

    config.transportMode = TransportMode::Tcp;
    config.linkProfile.mode = TransportMode::Tcp;
    config.linkProfile.lossPercent = 0.0;

    config.scenarioDurationMs = 2200;
    config.stepMs = 10;

    config.ueConfigs.resize(1);

    auto& ue = config.ueConfigs[0];
    ue.nodeId = 7;
    ue.ueId = 7;
    ue.attachStartMs = 0;
    ue.trafficStartMs = 500;
    ue.trafficEndMs = 1200;

    ue.uplinkTrafficProfile.pattern = TrafficPattern::ConstantBitrate;
    ue.uplinkTrafficProfile.durationMs = 600;
    ue.uplinkTrafficProfile.packetSizeBytes = 128;
    ue.uplinkTrafficProfile.packetsPerSecond = 5;

    ue.downlinkEnabled = false;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_EQ(result.ueResults.size(), 1U);
    ASSERT_TRUE(result.ueResults[0].attachSucceeded);
    ASSERT_TRUE(result.ueResults[0].detachSucceeded);
    ASSERT_EQ(result.activeSessionsAtEnd, 0U);
}
