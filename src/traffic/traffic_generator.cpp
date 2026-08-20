#include "miniran/traffic/traffic_generator.h"

#include <algorithm>
#include <cstdint>

namespace miniran {

namespace {

constexpr std::size_t kMaxGeneratedEvents = 1'000'000;

bool reachedEventLimit(const std::vector<TrafficEvent>& events) {
    return events.size() >= kMaxGeneratedEvents;
}

std::uint64_t cbrPacketCount(std::uint64_t durationMs, std::uint32_t packetsPerSecond) {
    if (durationMs == 0 || packetsPerSecond == 0) {
        return 0;
    }

    const std::uint64_t fullSeconds = durationMs / 1000U;
    const std::uint64_t remainingMs = durationMs % 1000U;

    const std::uint64_t packetsInFullSeconds =
        fullSeconds * static_cast<std::uint64_t>(packetsPerSecond);

    const std::uint64_t packetsInRemainder =
        (remainingMs * static_cast<std::uint64_t>(packetsPerSecond) + 999U) / 1000U;

    return packetsInFullSeconds + packetsInRemainder;
}

std::uint64_t cbrTimestampMs(std::uint64_t packetIndex, std::uint32_t packetsPerSecond) {
    return (packetIndex * 1000U) / static_cast<std::uint64_t>(packetsPerSecond);
}

}  // namespace

TrafficGenerator::TrafficGenerator(TrafficProfile profile, std::uint32_t seed)
    : profile_(profile), rng_(seed) {}

std::vector<std::uint8_t> TrafficGenerator::makePayload(std::size_t size, std::uint8_t seed) {
    std::vector<std::uint8_t> payload(size);
    for (std::size_t index = 0; index < size; ++index) {
        payload[index] = static_cast<std::uint8_t>(
            seed + static_cast<std::uint8_t>(index % 31U)
        );
    }
    return payload;
}

std::vector<TrafficEvent> TrafficGenerator::generate() {
    std::vector<TrafficEvent> events;
    if (!profile_.isValid()) {
        return events;
    }

    switch (profile_.pattern) {
        case TrafficPattern::ConstantBitrate: {
            const std::uint64_t packetCount =
                std::min<std::uint64_t>(
                    cbrPacketCount(profile_.durationMs, profile_.packetsPerSecond),
                    static_cast<std::uint64_t>(kMaxGeneratedEvents)
                );

            for (std::uint64_t packetIndex = 0; packetIndex < packetCount; ++packetIndex) {
                TrafficEvent event;
                event.timestampMs = cbrTimestampMs(packetIndex, profile_.packetsPerSecond);
                event.payload = makePayload(
                    profile_.packetSizeBytes,
                    static_cast<std::uint8_t>(events.size() & 0xFFU)
                );
                events.push_back(std::move(event));
            }

            break;
        }

        case TrafficPattern::Bursty: {
            std::uint64_t burstStartMs = 0;
            while (burstStartMs < profile_.durationMs && !reachedEventLimit(events)) {
                for (std::uint32_t packetIndex = 0;
                     packetIndex < profile_.burstPackets && !reachedEventLimit(events);
                     ++packetIndex) {
                    TrafficEvent event;
                    event.timestampMs = std::min<std::uint64_t>(
                        profile_.durationMs - 1,
                        burstStartMs + packetIndex
                    );
                    event.payload = makePayload(
                        profile_.packetSizeBytes,
                        static_cast<std::uint8_t>((events.size() * 3U) & 0xFFU)
                    );
                    events.push_back(std::move(event));
                }
                burstStartMs += profile_.burstIntervalMs;
            }
            break;
        }

        case TrafficPattern::Ramp: {
            const double startPps = static_cast<double>(profile_.rampStartPps);
            const double endPps = static_cast<double>(profile_.rampEndPps);

            double currentMs = 0.0;
            while (currentMs < static_cast<double>(profile_.durationMs) &&
                   !reachedEventLimit(events)) {
                const double progress =
                    currentMs / static_cast<double>(profile_.durationMs);
                const double currentPps = startPps + ((endPps - startPps) * progress);
                const double intervalMs = 1000.0 / std::max(1.0, currentPps);

                TrafficEvent event;
                event.timestampMs = static_cast<std::uint64_t>(currentMs);
                event.payload = makePayload(
                    profile_.packetSizeBytes,
                    static_cast<std::uint8_t>((events.size() * 5U) & 0xFFU)
                );
                events.push_back(std::move(event));
                currentMs += intervalMs;
            }
            break;
        }
    }

    return events;
}

}  // namespace miniran