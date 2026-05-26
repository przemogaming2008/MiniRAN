#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_low_bandwidth_limits_throughput) {
    ScenarioConfig config;
    config.scenarioName = "low_bandwidth_limit";

    config.transportMode = TransportMode::Udp;

    config.linkProfile.mode = TransportMode::Udp;
    config.linkProfile.latencyMs = 10;
    config.linkProfile.jitterMs = 2;
    config.linkProfile.lossPercent = 0.0;
    config.linkProfile.reorderPercent = 0.0;
    config.linkProfile.bandwidthKbps = 400;

    config.scenarioDurationMs = 4500;
    config.stepMs = 10;

    config.timers.attachTimeoutMs = 150;
    config.timers.detachTimeoutMs = 250;
    config.timers.maxAttachRetries = 4;
    config.timers.maxDetachRetries = 5;

    config.ueConfigs.resize(1);

    auto& ue = config.ueConfigs[0];
    ue.nodeId = 7;
    ue.ueId = 7;
    ue.attachStartMs = 0;
    ue.trafficStartMs = 800;
    ue.trafficEndMs = 3000;

    ue.uplinkTrafficProfile.pattern = TrafficPattern::Bursty;
    ue.uplinkTrafficProfile.durationMs = 1000;
    ue.uplinkTrafficProfile.packetSizeBytes = 400;
    ue.uplinkTrafficProfile.burstPackets = 6;
    ue.uplinkTrafficProfile.burstIntervalMs = 80;

    ue.downlinkEnabled = false;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_EQ(result.ueResults.size(), 1U);
    ASSERT_TRUE(result.ueResults[0].attachSucceeded);
    ASSERT_TRUE(result.ueResults[0].detachSucceeded);
    ASSERT_TRUE(result.uplinkBytesAcceptedByCore > 0);
}