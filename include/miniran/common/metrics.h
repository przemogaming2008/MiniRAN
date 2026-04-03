#pragma once

#include <cstddef>

namespace miniran {

struct FlowMetrics {
    std::size_t bytesSent = 0;
    std::size_t bytesDelivered = 0;
    std::size_t packetsSent = 0;
    std::size_t packetsDelivered = 0;
    std::size_t packetsDropped = 0;

    double throughputMbps(std::size_t durationMs) const;
    double deliveryRatio() const;
};

}  // namespace miniran
