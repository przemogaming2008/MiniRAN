#include "miniran/simulation/scenario_runner.h"
#include "support/test_framework.h"

using namespace miniran;

namespace {

double offeredThroughputMbps(const SimulationResult& result, std::uint64_t durationMs) {
    return (durationMs == 0)
               ? 0.0
               : (static_cast<double>(result.bytesGenerated) * 8.0) /
                     (static_cast<double>(durationMs) / 1000.0) /
                     1'000'000.0;
}

}  // namespace

TEST_CASE(component_low_bandwidth_limits_throughput) {
    ScenarioConfig config;
    config.scenarioName = "low_bandwidth_limit";
    config.transportMode = TransportMode::Udp;
    config.stepMs = 10;
    config.attachPhaseBudgetMs = 1600;
    config.detachPhaseBudgetMs = 5000;

    config.linkProfile.mode = TransportMode::Udp;
    config.linkProfile.latencyMs = 10;
    config.linkProfile.jitterMs = 2;
    config.linkProfile.lossPercent = 0.0;
    config.linkProfile.reorderPercent = 0.0;
    config.linkProfile.bandwidthKbps = 400;
    config.linkProfile.queueLimitPackets = 32;

    config.timers.attachTimeoutMs = 150;
    config.timers.detachTimeoutMs = 180;
    config.timers.heartbeatIntervalMs = 200;
    config.timers.inactivityTimeoutMs = 6000;
    config.timers.maxAttachRetries = 4;
    config.timers.maxDetachRetries = 30;

    config.trafficProfile.pattern = TrafficPattern::ConstantBitrate;
    config.trafficProfile.durationMs = 1800;
    config.trafficProfile.packetSizeBytes = 800;
    config.trafficProfile.packetsPerSecond = 100;

    ScenarioRunner runner(config);
    const auto result = runner.run();

    ASSERT_TRUE(result.attachSucceeded);
    ASSERT_TRUE(result.trafficStarted);
    ASSERT_TRUE(result.detachSucceeded);

    ASSERT_TRUE(result.bytesGenerated > 0);
    ASSERT_TRUE(result.bytesDeliveredToCore > 0);

    const double offeredMbps = offeredThroughputMbps(result, config.trafficProfile.durationMs);
    const double linkLimitMbps = static_cast<double>(config.linkProfile.bandwidthKbps) / 1000.0;

    ASSERT_TRUE(offeredMbps > linkLimitMbps);
    ASSERT_TRUE(result.packetsDroppedInNetwork > 0);
    ASSERT_TRUE(result.throughputMbps < offeredMbps);
}