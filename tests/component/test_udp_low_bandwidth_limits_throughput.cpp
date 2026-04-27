#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_low_bandwidth_limits_throughput){
    ScenarioConfig config;
    config.scenarioName = "low_bandwidth_limit";
    config.transportMode = TransportMode::Udp;

    config.linkProfile.mode = TransportMode::Udp;
    config.linkProfile.latencyMs = 10;
    config.linkProfile.jitterMs = 2;
    config.linkProfile.lossPercent = 0.0;
    config.linkProfile.reorderPercent = 0.0;
    config.linkProfile.bandwidthKbps = 400;

    config.timers.attachTimeoutMs = 150;
    config.timers.detachTimeoutMs = 250;
    config.timers.maxAttachRetries = 4;
    config.timers.maxDetachRetries = 5;

    config.trafficProfile.pattern = TrafficPattern::Bursty;
    config.trafficProfile.durationMs = 1000;
    config.trafficProfile.packetSizeBytes = 400;
    config.trafficProfile.burstPackets = 6;
    config.trafficProfile.burstIntervalMs = 80;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(result.attachSucceeded);
    ASSERT_TRUE(result.detachSucceeded);

    ASSERT_TRUE(result.bytesDeliveredToCore > 0);
}