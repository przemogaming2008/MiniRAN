#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_multi_ue_independent_detach) {
    ScenarioConfig config;
    config.scenarioName = "component_multi_ue_independent_detach";

    config.transportMode = TransportMode::Tcp;
    config.linkProfile.mode = TransportMode::Tcp;
    config.linkProfile.latencyMs = 10;
    config.linkProfile.lossPercent = 0.0;
    config.linkProfile.reorderPercent = 0.0;

    config.scenarioDurationMs = 4200;
    config.stepMs = 10;

    config.ueConfigs.resize(2);

    auto& early = config.ueConfigs[0];
    early.nodeId = 7;
    early.ueId = 7;
    early.attachStartMs = 0;
    early.trafficStartMs = 600;
    early.trafficEndMs = 1400;
    early.uplinkTrafficProfile.pattern = TrafficPattern::ConstantBitrate;
    early.uplinkTrafficProfile.durationMs = 800;
    early.uplinkTrafficProfile.packetSizeBytes = 128;
    early.uplinkTrafficProfile.packetsPerSecond = 10;
    early.downlinkEnabled = false;

    auto& late = config.ueConfigs[1];
    late.nodeId = 8;
    late.ueId = 8;
    late.attachStartMs = 200;
    late.trafficStartMs = 900;
    late.trafficEndMs = 3000;
    late.uplinkTrafficProfile.pattern = TrafficPattern::ConstantBitrate;
    late.uplinkTrafficProfile.durationMs = 1800;
    late.uplinkTrafficProfile.packetSizeBytes = 256;
    late.uplinkTrafficProfile.packetsPerSecond = 15;
    late.downlinkEnabled = false;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_EQ(result.ueResults.size(), 2U);

    ASSERT_TRUE(result.ueResults[0].attachSucceeded);
    ASSERT_TRUE(result.ueResults[1].attachSucceeded);

    ASSERT_TRUE(result.ueResults[0].detachSucceeded);
    ASSERT_TRUE(result.ueResults[1].detachSucceeded);

    ASSERT_EQ(result.cleanlyDetachedSessions, 2U);
    ASSERT_EQ(result.activeSessionsAtEnd, 0U);
    ASSERT_EQ(result.expiredSessions, 0U);
    ASSERT_EQ(result.protocolRejectedPackets, 0U);

    ASSERT_TRUE(result.ueResults[0].uplinkBytesAcceptedByCore > 0);
    ASSERT_TRUE(result.ueResults[1].uplinkBytesAcceptedByCore > 0);

    ASSERT_TRUE(result.ueResults[0].sessionId != result.ueResults[1].sessionId);
}