#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_multi_ue_tcp_uplink_downlink) {
    ScenarioConfig config;
    config.scenarioName = "component_multi_ue_tcp_uplink_downlink";

    config.transportMode = TransportMode::Tcp;
    config.linkProfile.mode = TransportMode::Tcp;
    config.linkProfile.latencyMs = 15;
    config.linkProfile.jitterMs = 1;
    config.linkProfile.lossPercent = 0.0;
    config.linkProfile.reorderPercent = 0.0;
    config.linkProfile.bandwidthKbps = 12000;
    config.linkProfile.queueLimitPackets = 256;

    config.scenarioDurationMs = 3600;
    config.stepMs = 10;

    config.ueConfigs.resize(3);

    auto& ue0 = config.ueConfigs[0];
    ue0.nodeId = 7;
    ue0.ueId = 7;
    ue0.attachStartMs = 0;
    ue0.trafficStartMs = 700;
    ue0.trafficEndMs = 2300;
    ue0.uplinkTrafficProfile.pattern = TrafficPattern::ConstantBitrate;
    ue0.uplinkTrafficProfile.durationMs = 1000;
    ue0.uplinkTrafficProfile.packetSizeBytes = 256;
    ue0.uplinkTrafficProfile.packetsPerSecond = 20;
    ue0.downlinkEnabled = true;
    ue0.downlinkTrafficProfile.pattern = TrafficPattern::ConstantBitrate;
    ue0.downlinkTrafficProfile.durationMs = 1000;
    ue0.downlinkTrafficProfile.packetSizeBytes = 128;
    ue0.downlinkTrafficProfile.packetsPerSecond = 10;

    auto& ue1 = config.ueConfigs[1];
    ue1.nodeId = 8;
    ue1.ueId = 8;
    ue1.attachStartMs = 200;
    ue1.trafficStartMs = 900;
    ue1.trafficEndMs = 2500;
    ue1.uplinkTrafficProfile.pattern = TrafficPattern::Bursty;
    ue1.uplinkTrafficProfile.durationMs = 1000;
    ue1.uplinkTrafficProfile.packetSizeBytes = 192;
    ue1.uplinkTrafficProfile.burstPackets = 4;
    ue1.uplinkTrafficProfile.burstIntervalMs = 250;
    ue1.downlinkEnabled = true;
    ue1.downlinkTrafficProfile.pattern = TrafficPattern::Bursty;
    ue1.downlinkTrafficProfile.durationMs = 1000;
    ue1.downlinkTrafficProfile.packetSizeBytes = 96;
    ue1.downlinkTrafficProfile.burstPackets = 4;
    ue1.downlinkTrafficProfile.burstIntervalMs = 300;

    auto& ue2 = config.ueConfigs[2];
    ue2.nodeId = 9;
    ue2.ueId = 9;
    ue2.attachStartMs = 400;
    ue2.trafficStartMs = 1100;
    ue2.trafficEndMs = 2900;
    ue2.uplinkTrafficProfile.pattern = TrafficPattern::Ramp;
    ue2.uplinkTrafficProfile.durationMs = 1000;
    ue2.uplinkTrafficProfile.packetSizeBytes = 256;
    ue2.uplinkTrafficProfile.rampStartPps = 4;
    ue2.uplinkTrafficProfile.rampEndPps = 20;
    ue2.downlinkEnabled = true;
    ue2.downlinkTrafficProfile.pattern = TrafficPattern::Ramp;
    ue2.downlinkTrafficProfile.durationMs = 1000;
    ue2.downlinkTrafficProfile.packetSizeBytes = 128;
    ue2.downlinkTrafficProfile.rampStartPps = 3;
    ue2.downlinkTrafficProfile.rampEndPps = 18;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_EQ(result.ueResults.size(), 3U);
    ASSERT_TRUE(result.succeeded());

    ASSERT_TRUE(result.ueResults[0].attachSucceeded);
    ASSERT_TRUE(result.ueResults[1].attachSucceeded);
    ASSERT_TRUE(result.ueResults[2].attachSucceeded);

    ASSERT_TRUE(result.ueResults[0].detachSucceeded);
    ASSERT_TRUE(result.ueResults[1].detachSucceeded);
    ASSERT_TRUE(result.ueResults[2].detachSucceeded);

    ASSERT_TRUE(result.ueResults[0].sessionId != 0U);
    ASSERT_TRUE(result.ueResults[1].sessionId != 0U);
    ASSERT_TRUE(result.ueResults[2].sessionId != 0U);

    ASSERT_TRUE(result.ueResults[0].sessionId != result.ueResults[1].sessionId);
    ASSERT_TRUE(result.ueResults[1].sessionId != result.ueResults[2].sessionId);
    ASSERT_TRUE(result.ueResults[0].sessionId != result.ueResults[2].sessionId);

    ASSERT_TRUE(result.ueResults[0].uplinkBytesAcceptedByCore > 0);
    ASSERT_TRUE(result.ueResults[1].uplinkBytesAcceptedByCore > 0);
    ASSERT_TRUE(result.ueResults[2].uplinkBytesAcceptedByCore > 0);

    ASSERT_TRUE(result.ueResults[0].downlinkBytesReceivedByUe > 0);
    ASSERT_TRUE(result.ueResults[1].downlinkBytesReceivedByUe > 0);
    ASSERT_TRUE(result.ueResults[2].downlinkBytesReceivedByUe > 0);

    ASSERT_EQ(result.activeSessionsAtEnd, 0U);
    ASSERT_EQ(result.expiredSessions, 0U);
    ASSERT_EQ(result.protocolRejectedPackets, 0U);
}