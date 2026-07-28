#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_udp_run_survives_moderate_loss) {
    ScenarioConfig config;
    config.scenarioName = "component_udp_stability";
    config.transportMode = TransportMode::Udp;
    config.stepMs = 10;
    config.attachPhaseBudgetMs = 2500;
    config.detachPhaseBudgetMs = 5000;

    config.linkProfile.mode = TransportMode::Udp;
    config.linkProfile.latencyMs = 20;
    config.linkProfile.jitterMs = 5;
    config.linkProfile.lossPercent = 5.0;
    config.linkProfile.reorderPercent = 10.0;
    config.linkProfile.bandwidthKbps = 10'000;
    config.linkProfile.queueLimitPackets = 512;

    config.timers.attachTimeoutMs = 120;
    config.timers.detachTimeoutMs = 180;
    config.timers.heartbeatIntervalMs = 200;
    config.timers.inactivityTimeoutMs = 6000;
    config.timers.maxAttachRetries = 10;
    config.timers.maxDetachRetries = 30;

    config.trafficProfile.pattern = TrafficPattern::Bursty;
    config.trafficProfile.durationMs = 2000;
    config.trafficProfile.packetSizeBytes = 180;
    config.trafficProfile.burstPackets = 8;
    config.trafficProfile.burstIntervalMs = 100;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(result.attachSucceeded);
    ASSERT_TRUE(result.trafficStarted);
    ASSERT_TRUE(result.detachSucceeded);
    ASSERT_TRUE(result.isHealthy());

    ASSERT_EQ(result.finalUeState, SessionState::Released);
    ASSERT_EQ(result.activeSessionsAtEnd, static_cast<std::size_t>(0));
    ASSERT_EQ(result.expiredSessions, static_cast<std::size_t>(0));

    ASSERT_TRUE(result.bytesGenerated > 0);
    ASSERT_TRUE(result.bytesDeliveredToCore > 0);

    ASSERT_TRUE(result.packetsGenerated > 0);
    ASSERT_TRUE(result.packetsDeliveredToCore > 0);

    ASSERT_TRUE(result.throughputMbps > 0.0);
}