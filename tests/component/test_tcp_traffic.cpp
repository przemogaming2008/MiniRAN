#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_tcp_run_delivers_payload_to_core) {
    ScenarioConfig config;
    config.scenarioName = "component_tcp_traffic";

    config.transportMode = TransportMode::Tcp;
    config.linkProfile.mode = TransportMode::Tcp;
    config.linkProfile.latencyMs = 15;
    config.linkProfile.lossPercent = 0.0;

    config.scenarioDurationMs = 2600;
    config.stepMs = 10;

    config.ueConfigs.resize(1);

    auto& ue = config.ueConfigs[0];
    ue.nodeId = 7;
    ue.ueId = 7;
    ue.attachStartMs = 0;
    ue.trafficStartMs = 600;
    ue.trafficEndMs = 1700;

    ue.uplinkTrafficProfile.pattern = TrafficPattern::ConstantBitrate;
    ue.uplinkTrafficProfile.durationMs = 1000;
    ue.uplinkTrafficProfile.packetSizeBytes = 256;
    ue.uplinkTrafficProfile.packetsPerSecond = 20;

    ue.downlinkEnabled = false;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_EQ(result.ueResults.size(), 1U);
    ASSERT_TRUE(result.ueResults[0].attachSucceeded);
    ASSERT_TRUE(result.ueResults[0].detachSucceeded);
    ASSERT_TRUE(result.uplinkBytesAcceptedByCore > 0);
    ASSERT_EQ(result.packetsDroppedInNetwork, 0U);
    ASSERT_TRUE(result.totalUplinkThroughputMbps > 0.0);
}