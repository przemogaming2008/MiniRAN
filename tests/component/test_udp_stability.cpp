#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_udp_run_survives_moderate_loss) {
    ScenarioConfig config;
    config.scenarioName = "component_udp_stability";

    config.transportMode = TransportMode::Udp;

    config.linkProfile.mode = TransportMode::Udp;
    config.linkProfile.latencyMs = 20;
    config.linkProfile.jitterMs = 5;
    config.linkProfile.lossPercent = 5.0;
    config.linkProfile.reorderPercent = 10.0;

    config.scenarioDurationMs = 4000;
    config.stepMs = 10;

    config.timers.attachTimeoutMs = 160;
    config.timers.detachTimeoutMs = 160;
    config.timers.maxAttachRetries = 5;
    config.timers.maxDetachRetries = 4;

    config.ueConfigs.resize(1);

    auto& ue = config.ueConfigs[0];
    ue.nodeId = 7;
    ue.ueId = 7;
    ue.attachStartMs = 0;
    ue.trafficStartMs = 1000;
    ue.trafficEndMs = 2600;

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
