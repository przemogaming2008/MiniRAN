#pragma once

#include <optional>
#include <string>

#include "miniran/protocol/session_manager.h"
#include "miniran/traffic/traffic_profile.h"
#include "miniran/transport/link_profile.h"

namespace miniran {

struct ScenarioConfig {
    std::string scenarioName = "default";
    TransportMode transportMode = TransportMode::Tcp;
    std::uint32_t ueId = 7;
    std::uint32_t accessNodeId = 1000;
    std::uint64_t stepMs = 10;
    std::uint64_t attachPhaseBudgetMs = 600;
    std::uint64_t detachPhaseBudgetMs = 600;
    SessionTimers timers{};
    LinkProfile linkProfile{};
    TrafficProfile trafficProfile{};

    static std::optional<ScenarioConfig> fromFile(const std::string& path, std::string& error);
};

}  // namespace miniran
