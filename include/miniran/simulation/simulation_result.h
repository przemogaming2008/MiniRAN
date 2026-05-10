#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "miniran/protocol/protocol_message.h"

namespace miniran {

struct SimulationResult {
    std::string scenarioName;
    std::size_t totalDurationMs = 0;

    std::size_t packetsDroppedInNetwork = 0;
    std::size_t packetsDeliveredByNetwork = 0;
    std::size_t activeSessionsAtEnd = 0;
    std::uint64_t expiredSessions = 0;

    std::vector<UeSimulationResult> ueResults;
    std::vector<std::string> notes;

    std::string summary() const;
};

struct UeSimulationResult {
    std::uint32_t ueId = 0;

    bool attachSucceeded = false;
    bool trafficStarted = false;
    bool detachSucceeded = false;
    SessionState finalUeState = SessionState::Idle;

    std::size_t packetsGenerated = 0;
    std::size_t bytesGenerated = 0;
    std::size_t packetsDeliveredToCore = 0;
    std::size_t bytesDeliveredToCore = 0;

    double throughputMbps = 0.0;
    std::vector<std::string> notes;
};

}  // namespace miniran
