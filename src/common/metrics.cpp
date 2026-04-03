#include "miniran/common/metrics.h"

namespace miniran {

double FlowMetrics::throughputMbps(std::size_t durationMs) const {
    if (durationMs == 0) {
        return 0.0;
    }
    const double bitsDelivered = static_cast<double>(bytesDelivered) * 8.0;
    const double seconds = static_cast<double>(durationMs) / 1000.0;
    return (bitsDelivered / seconds) / 1'000'000.0;
}

double FlowMetrics::deliveryRatio() const {
    if (packetsSent == 0) {
        return 0.0;
    }
    return static_cast<double>(packetsDelivered) / static_cast<double>(packetsSent);
}

}  // namespace miniran
