#include <cstdint>
#include <limits>

#include "miniran/transport/datagram.h"
#include "miniran/transport/link_profile.h"
#include "miniran/transport/virtual_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(link_profile_validation_accepts_reasonable_values) {
    LinkProfile profile;
    profile.bandwidthKbps = 1000;
    profile.lossPercent = 2.5;
    profile.reorderPercent = 10.0;
    profile.queueLimitPackets = 64;

    ASSERT_TRUE(profile.isValid());
}

TEST_CASE(link_profile_validation_rejects_invalid_values) {
    LinkProfile profile;
    profile.bandwidthKbps = 0;
    profile.lossPercent = 101.0;
    ASSERT_TRUE(!profile.isValid());
}

TEST_CASE(link_profile_rejects_extreme_latency_and_jitter)
{
    LinkProfile profile{};

    profile.latencyMs = std::numeric_limits<std::uint32_t>::max();
    ASSERT_TRUE(!profile.isValid());

    profile.latencyMs = 20;
    profile.jitterMs = std::numeric_limits<std::uint32_t>::max();
    ASSERT_TRUE(!profile.isValid());
}

TEST_CASE(virtual_network_rejects_invalid_extreme_delay_profile)
{
    LinkProfile profile{};
    profile.latencyMs = std::numeric_limits<std::uint32_t>::max();
    profile.jitterMs = std::numeric_limits<std::uint32_t>::max();

    VirtualNetwork network(profile, 123);

    Datagram datagram{};
    datagram.fromNodeId = 1;
    datagram.toNodeId = 2;
    datagram.bytes = {1, 2, 3, 4};

    ASSERT_TRUE(!network.submit(datagram, 0));
}
