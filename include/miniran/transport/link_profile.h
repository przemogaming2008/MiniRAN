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
        return bandwidthKbps > 0 && lossPercent >= 0.0 && lossPercent <= 100.0 && reorderPercent >= 0.0 &&
               reorderPercent <= 100.0 && queueLimitPackets > 0;
    }
};

}  // namespace miniran
