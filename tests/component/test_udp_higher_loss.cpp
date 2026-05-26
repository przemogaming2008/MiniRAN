#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_udp_higher_loss) {
    ScenarioConfig config;
    config.scenarioName = "component_udp_higher_loss";

    config.transportMode = TransportMode::Udp;

    config.linkProfile.mode = TransportMode::Udp;
    config.linkProfile.latencyMs = 20;
    config.linkProfile.jitterMs = 5;
    config.linkProfile.lossPercent = 20.0;
    config.linkProfile.reorderPercent = 10.0;

    config.scenarioDurationMs = 4500;
    config.stepMs = 10;

    config.timers.attachTimeoutMs = 180;
    config.timers.detachTimeoutMs = 180;
    config.timers.maxAttachRetries = 6;
    config.timers.maxDetachRetries = 5;

    config.ueConfigs.resize(1);

    auto& ue = config.ueConfigs[0];
    ue.nodeId = 7;
    ue.ueId = 7;
    ue.attachStartMs = 0;
    ue.trafficStartMs = 1200;
    ue.trafficEndMs = 3000;

    ue.uplinkTrafficProfile.pattern = TrafficPattern::Bursty;
    ue.uplinkTrafficProfile.durationMs = 1000;
    ue.uplinkTrafficProfile.packetSizeBytes = 180;
    ue.uplinkTrafficProfile.burstPackets = 4;
    ue.uplinkTrafficProfile.burstIntervalMs = 200;

    ue.downlinkEnabled = false;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_EQ(result.ueResults.size(), 1U);
    ASSERT_TRUE(result.ueResults[0].attachSucceeded);
    ASSERT_TRUE(result.uplinkBytesAcceptedByCore > 0);
    ASSERT_TRUE(result.ueResults[0].detachSucceeded);
}