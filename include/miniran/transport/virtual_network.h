#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "miniran/common/metrics.h"
#include "miniran/transport/datagram.h"
#include "miniran/transport/link_profile.h"

namespace miniran {

class VirtualNetwork {
public:
    explicit VirtualNetwork(LinkProfile profile, std::uint32_t seed = 1337);

    bool submit(Datagram datagram, std::uint64_t nowMs);
    std::vector<Datagram> pollReady(std::uint64_t nowMs);

    std::size_t queuedPackets() const;
    const FlowMetrics& metrics() const;

private:
    LinkProfile profile_{};
    FlowMetrics metrics_{};
    std::vector<Datagram> queue_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> probability_{0.0, 100.0};
    std::uint64_t nextAvailableTxMs_ = 0;
    std::uint64_t serialCounter_ = 1;
};

}  // namespace miniran
