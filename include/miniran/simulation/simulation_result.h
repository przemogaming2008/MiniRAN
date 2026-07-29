#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "miniran/protocol/protocol_message.h"
#include "miniran/transport/transport_mode.h"

namespace miniran {

struct SimulationResult {
    bool isHealthy() const;

    std::string scenarioName;
    TransportMode transportMode = TransportMode::Tcp;

    std::size_t totalDurationMs = 0;
    bool attachSucceeded = false;
    bool trafficStarted = false;
    bool detachSucceeded = false;
    SessionState finalUeState = SessionState::Idle;

    std::size_t packetsGenerated = 0;
    std::size_t bytesGenerated = 0;
    std::size_t packetsDeliveredToCore = 0;
    std::size_t bytesDeliveredToCore = 0;
    std::size_t packetsDroppedInNetwork = 0;
    std::size_t packetsDeliveredByNetwork = 0;
    std::size_t rejectedNetworkSubmissions = 0;
    std::size_t activeSessionsAtEnd = 0;

    double throughputMbps = 0.0;
    std::vector<std::string> notes;

    std::uint64_t expiredSessions = 0;

    std::string summary() const;
};

}  // namespace miniran