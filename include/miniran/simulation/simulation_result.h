#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "miniran/protocol/protocol_message.h"

namespace miniran {

struct UeSimulationResult {
    std::uint32_t ueId = 0;
    std::uint32_t sessionId = 0;

    bool attachSucceeded = false;
    bool trafficStarted = false;
    bool detachSucceeded = false;
    bool activeAtEnd = false;

    SessionState finalUeState = SessionState::Idle;

    std::uint32_t attachRetries = 0;
    std::uint32_t detachRetries = 0;

    std::size_t heartbeatsSent = 0;
    std::size_t heartbeatAcksReceived = 0;

    std::size_t packetsGenerated = 0;
    std::size_t bytesGenerated = 0;

    std::size_t packetsSent = 0;
    std::size_t bytesSent = 0;

    std::size_t packetsDeliveredByNetwork = 0;
    std::size_t bytesDeliveredByNetwork = 0;

    std::size_t packetsAcceptedByCore = 0;
    std::size_t bytesAcceptedByCore = 0;

    double throughputMbps = 0.0;

    std::vector<std::string> notes;
};

struct SimulationResult {
    std::string scenarioName;
    std::size_t totalDurationMs = 0;

    std::size_t ueCount = 0;
    std::size_t cleanlyDetachedSessions = 0;

    std::size_t activeSessionsAtEnd = 0;
    std::uint64_t expiredSessions = 0;

    std::size_t packetsDroppedInNetwork = 0;
    std::size_t packetsDroppedByLoss = 0;
    std::size_t packetsDroppedByQueue = 0;
    std::size_t packetsDeliveredByNetwork = 0;

    std::size_t packetsAcceptedByCore = 0;
    std::size_t bytesAcceptedByCore = 0;

    double totalThroughputMbps = 0.0;

    std::vector<UeSimulationResult> ueResults;
    std::vector<std::string> notes;

    std::string summary() const;
};



}  // namespace miniran
