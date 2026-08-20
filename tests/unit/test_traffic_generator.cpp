#include "miniran/traffic/traffic_generator.h"
#include "support/test_framework.h"

using namespace miniran;

namespace {

TrafficProfile makeCbrProfile(std::uint32_t packetsPerSecond, std::uint64_t durationMs = 1000) {
    TrafficProfile profile;
    profile.pattern = TrafficPattern::ConstantBitrate;
    profile.durationMs = durationMs;
    profile.packetSizeBytes = 128;
    profile.packetsPerSecond = packetsPerSecond;
    return profile;
}

}  // namespace

TEST_CASE(cbr_generator_emits_expected_packet_count)
{
    TrafficProfile profile = makeCbrProfile(10);

    TrafficGenerator generator(profile, 7);
    const auto events = generator.generate();

    ASSERT_EQ(events.size(), 10U);
    ASSERT_EQ(events.front().timestampMs, 0U);
    ASSERT_EQ(events.back().timestampMs, 900U);
    ASSERT_EQ(events.back().payload.size(), 128U);
}

TEST_CASE(cbr_generator_does_not_emit_extra_packet_for_fractional_intervals)
{
    {
        TrafficGenerator generator(makeCbrProfile(11), 7);
        const auto events = generator.generate();

        ASSERT_EQ(events.size(), 11U);
        ASSERT_EQ(events.front().timestampMs, 0U);
        ASSERT_TRUE(events.back().timestampMs < 1000U);
    }

    {
        TrafficGenerator generator(makeCbrProfile(21), 7);
        const auto events = generator.generate();

        ASSERT_EQ(events.size(), 21U);
        ASSERT_EQ(events.front().timestampMs, 0U);
        ASSERT_TRUE(events.back().timestampMs < 1000U);
    }

    {
        TrafficGenerator generator(makeCbrProfile(60), 7);
        const auto events = generator.generate();

        ASSERT_EQ(events.size(), 60U);
        ASSERT_EQ(events.front().timestampMs, 0U);
        ASSERT_TRUE(events.back().timestampMs < 1000U);
    }
}

TEST_CASE(cbr_generator_handles_partial_second_without_drift)
{
    TrafficProfile profile = makeCbrProfile(11, 1500);

    TrafficGenerator generator(profile, 7);
    const auto events = generator.generate();

    ASSERT_EQ(events.size(), 17U);
    ASSERT_EQ(events.front().timestampMs, 0U);
    ASSERT_TRUE(events.back().timestampMs < 1500U);
}

TEST_CASE(bursty_generator_creates_multiple_packets_per_burst)
{
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

TEST_CASE(ramp_generator_rejects_descending_profile)
{
    TrafficProfile profile;
    profile.pattern = TrafficPattern::Ramp;
    profile.durationMs = 1000;
    profile.packetSizeBytes = 128;
    profile.rampStartPps = 100;
    profile.rampEndPps = 10;

    ASSERT_TRUE(!profile.isValid());

    TrafficGenerator generator(profile, 7);
    const auto events = generator.generate();

    ASSERT_TRUE(events.empty());
}

TEST_CASE(ramp_generator_accepts_ascending_profile)
{
    TrafficProfile profile;
    profile.pattern = TrafficPattern::Ramp;
    profile.durationMs = 1000;
    profile.packetSizeBytes = 128;
    profile.rampStartPps = 5;
    profile.rampEndPps = 20;

    ASSERT_TRUE(profile.isValid());

    TrafficGenerator generator(profile, 7);
    const auto events = generator.generate();

    ASSERT_TRUE(!events.empty());
    ASSERT_EQ(events.front().timestampMs, 0U);
    ASSERT_EQ(events.front().payload.size(), 128U);
}