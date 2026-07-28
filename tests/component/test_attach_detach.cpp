#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_attach_then_detach_over_tcp) {
    ScenarioConfig config;
    config.scenarioName = "component_attach_detach_tcp";
    config.transportMode = TransportMode::Tcp;
    config.stepMs = 10;
    config.attachPhaseBudgetMs = 1600;
    config.detachPhaseBudgetMs = 1600;

    config.linkProfile.mode = TransportMode::Tcp;
    config.linkProfile.lossPercent = 0.0;
    config.linkProfile.reorderPercent = 0.0;
    config.linkProfile.latencyMs = 10;
    config.linkProfile.jitterMs = 0;

    config.timers.attachTimeoutMs = 150;
    config.timers.detachTimeoutMs = 150;
    config.timers.heartbeatIntervalMs = 200;
    config.timers.inactivityTimeoutMs = 2000;
    config.timers.maxAttachRetries = 4;
    config.timers.maxDetachRetries = 4;

    config.trafficProfile.pattern = TrafficPattern::ConstantBitrate;
    config.trafficProfile.durationMs = 600;
    config.trafficProfile.packetSizeBytes = 128;
    config.trafficProfile.packetsPerSecond = 5;

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
    ASSERT_EQ(result.bytesDeliveredToCore, result.bytesGenerated);

    ASSERT_EQ(result.packetsDroppedInNetwork, static_cast<std::size_t>(0));
}