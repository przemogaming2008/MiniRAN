#include "miniran/transport/virtual_network.h"

#include <algorithm>
#include <cmath>

namespace miniran {

VirtualNetwork::VirtualNetwork(LinkProfile profile, std::uint32_t seed) : profile_(profile), rng_(seed) {}

bool VirtualNetwork::submit(Datagram datagram, std::uint64_t nowMs) {
    if (!profile_.isValid()) {
        return false;
    }

    metrics_.packetsSent += 1;
    metrics_.bytesSent += datagram.bytes.size();

    if (queue_.size() >= profile_.queueLimitPackets) {
        metrics_.packetsDropped += 1;
        return false;
    }

    datagram.enqueueTimeMs = nowMs;
    datagram.serialNumber = serialCounter_++;

    if (profile_.mode == TransportMode::Udp && probability_(rng_) < profile_.lossPercent) {
        metrics_.packetsDropped += 1;
        return true;
    }

    const auto jitterRange = static_cast<int>(profile_.jitterMs);
    std::uniform_int_distribution<int> jitterDistribution(-jitterRange, jitterRange);
    const int jitterValue = (profile_.jitterMs == 0) ? 0 : jitterDistribution(rng_);

    const std::uint64_t serializationDelayMs = static_cast<std::uint64_t>(
        std::ceil((static_cast<double>(datagram.bytes.size()) * 8.0) / static_cast<double>(profile_.bandwidthKbps)));

    const std::uint64_t scheduledTxMs = std::max(nowMs, nextAvailableTxMs_);
    nextAvailableTxMs_ = scheduledTxMs + serializationDelayMs;

    std::uint64_t baseDelay = profile_.latencyMs;
    if (jitterValue < 0 && static_cast<std::uint32_t>(-jitterValue) > baseDelay) {
        baseDelay = 0;
    } else {
        baseDelay = static_cast<std::uint64_t>(static_cast<int>(baseDelay) + jitterValue);
    }

    datagram.deliverAtMs = scheduledTxMs + serializationDelayMs + baseDelay;

    if (profile_.mode == TransportMode::Udp && probability_(rng_) < profile_.reorderPercent) {
        const std::uint64_t advanceMs = std::min<std::uint64_t>(profile_.latencyMs / 2U, datagram.deliverAtMs - nowMs);
        datagram.deliverAtMs -= advanceMs;
    }

    queue_.push_back(std::move(datagram));
    return true;
}

std::vector<Datagram> VirtualNetwork::pollReady(std::uint64_t nowMs) {
    std::vector<Datagram> ready;
    std::vector<Datagram> remaining;
    ready.reserve(queue_.size());
    remaining.reserve(queue_.size());

    for (auto& datagram : queue_) {
        if (datagram.deliverAtMs <= nowMs) {
            ready.push_back(std::move(datagram));
        } else {
            remaining.push_back(std::move(datagram));
        }
    }
    queue_ = std::move(remaining);

    std::sort(ready.begin(), ready.end(), [](const Datagram& left, const Datagram& right) {
        if (left.deliverAtMs == right.deliverAtMs) {
            return left.serialNumber < right.serialNumber;
        }
        return left.deliverAtMs < right.deliverAtMs;
    });

    for (const auto& datagram : ready) {
        metrics_.packetsDelivered += 1;
        metrics_.bytesDelivered += datagram.bytes.size();
    }

    return ready;
}

std::size_t VirtualNetwork::queuedPackets() const {
    return queue_.size();
}

const FlowMetrics& VirtualNetwork::metrics() const {
    return metrics_;
}

}  // namespace miniran
