#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <unordered_set>

#include "miniran/protocol/session_manager.h"
#include "miniran/traffic/traffic_profile.h"
#include "miniran/transport/link_profile.h"

namespace miniran {

struct UeConfig {
    std::uint32_t nodeId = 7;
    std::uint32_t ueId = 7;

    std::uint64_t attachStartMs = 0;
    std::uint64_t trafficStartMs = 0;
    std::uint64_t trafficEndMs = 1000;

    TrafficProfile uplinkTrafficProfile{};

    TrafficProfile downlinkTrafficProfile{};

    bool downlinkEnabled = false;
};

struct ScenarioConfig {
    std::string scenarioName = "default";

    TransportMode transportMode = TransportMode::Tcp;
    std::uint32_t accessNodeId = 1000;

    std::uint64_t stepMs = 10;
    std::uint64_t scenarioDurationMs = 2000;

    SessionTimers timers{};
    LinkProfile linkProfile{};

    std::uint32_t networkSeed = 1337;
    std::uint32_t trafficSeed = 7;

    std::vector<UeConfig> ueConfigs{{}};

    static std::optional<ScenarioConfig> fromFile(const std::string& path, std::string& error);
    bool validate(std::string& error) const;
};

}  // namespace miniran
