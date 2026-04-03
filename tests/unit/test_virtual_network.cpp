#include <vector>

#include "miniran/transport/virtual_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(tcp_virtual_network_delivers_all_packets_even_if_loss_is_configured) {
    LinkProfile profile;
    profile.mode = TransportMode::Tcp;
    profile.lossPercent = 80.0;
    profile.latencyMs = 10;
    profile.bandwidthKbps = 1000;

    VirtualNetwork network(profile, 123);

    for (std::uint32_t index = 0; index < 3; ++index) {
        Datagram datagram;
        datagram.fromNodeId = 1;
        datagram.toNodeId = 100;
        datagram.bytes = std::vector<std::uint8_t>(20, static_cast<std::uint8_t>(index));
        ASSERT_TRUE(network.submit(datagram, 0));
    }

    const auto ready = network.pollReady(100);
    ASSERT_EQ(ready.size(), 3U);
    ASSERT_EQ(network.metrics().packetsDropped, 0U);
}

TEST_CASE(queue_limit_causes_drop_when_full) {
    LinkProfile profile;
    profile.mode = TransportMode::Udp;
    profile.queueLimitPackets = 1;
    profile.lossPercent = 0.0;
    profile.bandwidthKbps = 1000;
    profile.latencyMs = 1000;

    VirtualNetwork network(profile, 123);

    Datagram first;
    first.fromNodeId = 1;
    first.toNodeId = 100;
    first.bytes = std::vector<std::uint8_t>(10, 1);

    Datagram second = first;
    second.bytes = std::vector<std::uint8_t>(10, 2);

    ASSERT_TRUE(network.submit(first, 0));
    ASSERT_TRUE(!network.submit(second, 0));
    ASSERT_EQ(network.metrics().packetsDropped, 1U);
}
