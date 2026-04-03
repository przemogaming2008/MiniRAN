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
    config.trafficProfile.pattern = TrafficPattern::ConstantBitrate;
    config.trafficProfile.durationMs = 1000;
    config.trafficProfile.packetSizeBytes = 256;
    config.trafficProfile.packetsPerSecond = 20;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(result.attachSucceeded);
    ASSERT_TRUE(result.bytesDeliveredToCore > 0);
    ASSERT_EQ(result.packetsDroppedInNetwork, 0U);
    ASSERT_TRUE(result.throughputMbps > 0.0);
}
