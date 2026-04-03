#include "miniran/traffic/traffic_generator.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(cbr_generator_emits_expected_packet_count) {
    TrafficProfile profile;
    profile.pattern = TrafficPattern::ConstantBitrate;
    profile.durationMs = 1000;
    profile.packetSizeBytes = 128;
    profile.packetsPerSecond = 10;

    TrafficGenerator generator(profile, 7);
    const auto events = generator.generate();

    ASSERT_EQ(events.size(), 10U);
    ASSERT_EQ(events.front().timestampMs, 0U);
    ASSERT_EQ(events.back().payload.size(), 128U);
}

TEST_CASE(bursty_generator_creates_multiple_packets_per_burst) {
    TrafficProfile profile;
    profile.pattern = TrafficPattern::Bursty;
    profile.durationMs = 450;
    profile.packetSizeBytes = 64;
    profile.packetsPerSecond = 5;
    profile.burstPackets = 3;
    profile.burstIntervalMs = 200;

    TrafficGenerator generator(profile, 7);
    const auto events = generator.generate();

    ASSERT_EQ(events.size(), 9U);
    ASSERT_EQ(events[0].timestampMs, 0U);
    ASSERT_EQ(events[1].timestampMs, 1U);
    ASSERT_EQ(events[3].timestampMs, 200U);
}
