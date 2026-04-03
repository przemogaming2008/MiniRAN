#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "miniran/traffic/traffic_profile.h"

namespace miniran {

class TrafficGenerator {
public:
    explicit TrafficGenerator(TrafficProfile profile, std::uint32_t seed = 7);

    std::vector<TrafficEvent> generate();
    static std::vector<std::uint8_t> makePayload(std::size_t size, std::uint8_t seed);

private:
    TrafficProfile profile_{};
    std::mt19937 rng_;
};

}  // namespace miniran
