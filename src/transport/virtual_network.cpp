#include "miniran/transport/virtual_network.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace miniran {

VirtualNetwork::VirtualNetwork(LinkProfile profile, std::uint32_t seed)
    : profile_(profile),
      rng_(seed)
{
}

bool VirtualNetwork::submit(Datagram datagram, std::uint64_t nowMs)
{
    if (!profile_.isValid()) {
        return false;
    }

    metrics_.packetsSent += 1;
    metrics_.bytesSent += datagram.bytes.size();

    datagram.enqueueTimeMs = nowMs;
    datagram.serialNumber = serialCounter_++;

    if (profile_.mode == TransportMode::Udp &&
        probability_(rng_) < profile_.lossPercent)
    {
        metrics_.packetsDropped += 1;
        return true;
    }

    if (queue_.size() >= profile_.queueLimitPackets) {
        metrics_.packetsDropped += 1;
        return false;
    }

    const auto latencyMs = static_cast<std::int64_t>(profile_.latencyMs);
    const auto jitterRangeMs = static_cast<std::int64_t>(profile_.jitterMs);

    std::int64_t jitterValueMs = 0;
    if (jitterRangeMs > 0) {
        std::uniform_int_distribution<std::int64_t> jitterDistribution(
            -jitterRangeMs,
            jitterRangeMs
        );
        jitterValueMs = jitterDistribution(rng_);
    }

    const std::uint64_t serializationDelayMs =
        static_cast<std::uint64_t>(
            std::ceil(
                (static_cast<double>(datagram.bytes.size()) * 8.0) /
                static_cast<double>(profile_.bandwidthKbps)
            )
        );

    const std::uint64_t scheduledTxMs = std::max(nowMs, nextAvailableTxMs_);

    if (serializationDelayMs >
        std::numeric_limits<std::uint64_t>::max() - scheduledTxMs)
    {
        return false;
    }

    const std::uint64_t txDoneMs = scheduledTxMs + serializationDelayMs;
    nextAvailableTxMs_ = txDoneMs;

    const std::int64_t delayWithJitterMs =
        std::max<std::int64_t>(0, latencyMs + jitterValueMs);

    const std::uint64_t baseDelay =
        static_cast<std::uint64_t>(delayWithJitterMs);

    if (baseDelay > std::numeric_limits<std::uint64_t>::max() - txDoneMs) {
        return false;
    }

    datagram.deliverAtMs = txDoneMs + baseDelay;

    if (profile_.mode == TransportMode::Tcp) {
        if (hasLastTcpDeliverAtMs_ &&
            datagram.deliverAtMs < lastTcpDeliverAtMs_)
        {
            datagram.deliverAtMs = lastTcpDeliverAtMs_;
        }

        lastTcpDeliverAtMs_ = datagram.deliverAtMs;
        hasLastTcpDeliverAtMs_ = true;
    }

    if (profile_.mode == TransportMode::Udp &&
        probability_(rng_) < profile_.reorderPercent)
    {
        const std::uint64_t advanceMs = std::min<std::uint64_t>(
            profile_.latencyMs / 2U,
            datagram.deliverAtMs - nowMs
        );
        datagram.deliverAtMs -= advanceMs;
    }

    queue_.push_back(std::move(datagram));
    return true;
}

std::vector<Datagram> VirtualNetwork::pollReady(std::uint64_t nowMs)
{
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

    std::sort(
        ready.begin(),
        ready.end(),
        [](const Datagram& left, const Datagram& right) {
            if (left.deliverAtMs == right.deliverAtMs) {
                return left.serialNumber < right.serialNumber;
            }

            return left.deliverAtMs < right.deliverAtMs;
        }
    );

    for (const auto& datagram : ready) {
        metrics_.packetsDelivered += 1;
        metrics_.bytesDelivered += datagram.bytes.size();
    }

    return ready;
}

std::size_t VirtualNetwork::queuedPackets() const
{
    return queue_.size();
}

const FlowMetrics& VirtualNetwork::metrics() const
{
    return metrics_;
}

}  // namespace miniran