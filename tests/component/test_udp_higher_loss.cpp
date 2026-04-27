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
    config.timers.attachTimeoutMs = 120;
    config.timers.detachTimeoutMs = 120;
    config.timers.maxAttachRetries = 4;
    config.timers.maxDetachRetries = 3;
    config.trafficProfile.pattern = TrafficPattern::Bursty;
    config.trafficProfile.durationMs = 1000;
    config.trafficProfile.packetSizeBytes = 180;
    config.trafficProfile.burstPackets = 4;
    config.trafficProfile.burstIntervalMs = 200;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(result.attachSucceeded);
    ASSERT_TRUE(result.bytesDeliveredToCore > 0);
    ASSERT_TRUE(result.detachSucceeded);
}