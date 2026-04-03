#include "miniran/traffic/traffic_generator.h"

#include <algorithm>

namespace miniran {

TrafficGenerator::TrafficGenerator(TrafficProfile profile, std::uint32_t seed) : profile_(profile), rng_(seed) {}

std::vector<std::uint8_t> TrafficGenerator::makePayload(std::size_t size, std::uint8_t seed) {
    std::vector<std::uint8_t> payload(size);
    for (std::size_t index = 0; index < size; ++index) {
        payload[index] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(index % 31U));
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
            const double intervalMs = 1000.0 / static_cast<double>(profile_.packetsPerSecond);
            double currentMs = 0.0;
            while (currentMs < static_cast<double>(profile_.durationMs)) {
                TrafficEvent event;
                event.timestampMs = static_cast<std::uint64_t>(currentMs);
                event.payload = makePayload(profile_.packetSizeBytes, static_cast<std::uint8_t>(events.size() & 0xFFU));
                events.push_back(std::move(event));
                currentMs += intervalMs;
            }
            break;
        }
        case TrafficPattern::Bursty: {
            std::uint64_t burstStartMs = 0;
            while (burstStartMs < profile_.durationMs) {
                for (std::uint32_t packetIndex = 0; packetIndex < profile_.burstPackets; ++packetIndex) {
                    TrafficEvent event;
                    event.timestampMs = std::min<std::uint64_t>(profile_.durationMs - 1, burstStartMs + packetIndex);
                    event.payload = makePayload(profile_.packetSizeBytes, static_cast<std::uint8_t>((events.size() * 3U) & 0xFFU));
                    events.push_back(std::move(event));
                }
                burstStartMs += profile_.burstIntervalMs;
            }
            break;
        }
        case TrafficPattern::Ramp: {
            double currentMs = 0.0;
            while (currentMs < static_cast<double>(profile_.durationMs)) {
                const double progress = currentMs / static_cast<double>(profile_.durationMs);
                const double currentPps = static_cast<double>(profile_.rampStartPps) +
                                          (static_cast<double>(profile_.rampEndPps - profile_.rampStartPps) * progress);
                const double intervalMs = 1000.0 / std::max(1.0, currentPps);
                TrafficEvent event;
                event.timestampMs = static_cast<std::uint64_t>(currentMs);
                event.payload = makePayload(profile_.packetSizeBytes, static_cast<std::uint8_t>((events.size() * 5U) & 0xFFU));
                events.push_back(std::move(event));
                currentMs += intervalMs;
            }
            break;
        }
    }

    return events;
}

}  // namespace miniran
