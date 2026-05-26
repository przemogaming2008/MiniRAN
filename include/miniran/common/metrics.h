#pragma once

#include <cstddef>
#include <cstdint>

namespace miniran {

struct FlowMetrics {
    std::size_t bytesSent = 0;
    std::size_t bytesDelivered = 0;
    std::size_t packetsSent = 0;
    std::size_t packetsDelivered = 0;
    std::size_t packetsDropped = 0;

    std::size_t packetsDroppedByLoss = 0;
    std::size_t packetsDroppedByQueue = 0;

    double throughputMbps(std::size_t durationMs) const;
    double deliveryRatio() const;
};

struct UeProtocolMetrics {
    std::uint32_t attachRetries = 0;
    std::uint32_t detachRetries = 0;

    std::size_t heartbeatsSent = 0;
    std::size_t heartbeatAcksReceived = 0;

    std::size_t protocolErrorsReceived = 0;
    std::size_t invalidMessagesDropped = 0;
};

}  // namespace miniran
