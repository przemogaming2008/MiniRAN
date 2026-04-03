#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace miniran {

enum class TrafficPattern {
    ConstantBitrate,
    Bursty,
    Ramp,
};

inline std::string toString(TrafficPattern pattern) {
    switch (pattern) {
        case TrafficPattern::ConstantBitrate:
            return "cbr";
        case TrafficPattern::Bursty:
            return "bursty";
        case TrafficPattern::Ramp:
            return "ramp";
    }
    return "unknown";
}

inline std::optional<TrafficPattern> parseTrafficPattern(std::string_view value) {
    if (value == "cbr") {
        return TrafficPattern::ConstantBitrate;
    }
    if (value == "bursty") {
        return TrafficPattern::Bursty;
    }
    if (value == "ramp") {
        return TrafficPattern::Ramp;
    }
    return std::nullopt;
}

struct TrafficProfile {
    TrafficPattern pattern = TrafficPattern::ConstantBitrate;
    std::uint64_t durationMs = 1000;
    std::size_t packetSizeBytes = 128;
    std::uint32_t packetsPerSecond = 10;
    std::uint32_t burstPackets = 4;
    std::uint32_t burstIntervalMs = 200;
    std::uint32_t rampStartPps = 5;
    std::uint32_t rampEndPps = 20;

    bool isValid() const {
        return durationMs > 0 && packetSizeBytes > 0 && burstIntervalMs > 0 && packetsPerSecond > 0 &&
               rampStartPps > 0 && rampEndPps > 0;
    }
};

struct TrafficEvent {
    std::uint64_t timestampMs = 0;
    std::vector<std::uint8_t> payload;
};

}  // namespace miniran
