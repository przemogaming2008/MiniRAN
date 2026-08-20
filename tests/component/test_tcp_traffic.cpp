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
TEST_CASE(component_tcp_irregular_step_delivers_all_generated_packets) {
    ScenarioConfig config;
    config.scenarioName = "component_tcp_irregular_step_delivers_all";
    config.transportMode = TransportMode::Tcp;
    config.stepMs = 37;
    config.attachPhaseBudgetMs = 2500;
    config.detachPhaseBudgetMs = 2500;

    config.linkProfile.mode = TransportMode::Tcp;
    config.linkProfile.latencyMs = 5;
    config.linkProfile.jitterMs = 0;
    config.linkProfile.lossPercent = 0.0;
    config.linkProfile.reorderPercent = 0.0;
    config.linkProfile.bandwidthKbps = 100'000;
    config.linkProfile.queueLimitPackets = 512;

    config.timers.attachTimeoutMs = 120;
    config.timers.detachTimeoutMs = 120;
    config.timers.heartbeatIntervalMs = 200;
    config.timers.inactivityTimeoutMs = 6000;
    config.timers.maxAttachRetries = 10;
    config.timers.maxDetachRetries = 10;

    config.trafficProfile.pattern = TrafficPattern::ConstantBitrate;
    config.trafficProfile.durationMs = 1000;
    config.trafficProfile.packetSizeBytes = 128;
    config.trafficProfile.packetsPerSecond = 21;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(result.attachSucceeded);
    ASSERT_TRUE(result.trafficStarted);
    ASSERT_TRUE(result.detachSucceeded);
    ASSERT_TRUE(result.isHealthy());

    ASSERT_TRUE(result.packetsGenerated > 0);
    ASSERT_EQ(result.packetsDeliveredToCore, result.packetsGenerated);
    ASSERT_EQ(result.bytesDeliveredToCore, result.bytesGenerated);
    ASSERT_EQ(result.packetsDroppedInNetwork, static_cast<std::size_t>(0));
    ASSERT_EQ(result.rejectedNetworkSubmissions, static_cast<std::size_t>(0));
}