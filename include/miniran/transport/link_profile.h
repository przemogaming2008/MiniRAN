#pragma once

#include <cstddef>
#include <cstdint>

#include "miniran/transport/transport_mode.h"

namespace miniran {

struct LinkProfile {
    TransportMode mode = TransportMode::Tcp;
    std::uint32_t latencyMs = 20;
    std::uint32_t jitterMs = 0;
    double lossPercent = 0.0;
    double reorderPercent = 0.0;
    std::uint64_t bandwidthKbps = 10'000;
    std::size_t queueLimitPackets = 256;

    bool isValid() const {
        constexpr std::uint32_t maxReasonableLatencyMs = 60'000;
        constexpr std::uint32_t maxReasonableJitterMs = 60'000;
        constexpr std::uint64_t maxReasonableBandwidthKbps = 100'000'000;
        constexpr std::size_t maxReasonableQueueLimitPackets = 1'000'000;

        return latencyMs <= maxReasonableLatencyMs &&
            jitterMs <= maxReasonableJitterMs &&
            lossPercent >= 0.0 &&
            lossPercent <= 100.0 &&
            reorderPercent >= 0.0 &&
            reorderPercent <= 100.0 &&
            bandwidthKbps > 0 &&
            bandwidthKbps <= maxReasonableBandwidthKbps &&
            queueLimitPackets > 0 &&
            queueLimitPackets <= maxReasonableQueueLimitPackets;
    }
};

}  // namespace miniran
