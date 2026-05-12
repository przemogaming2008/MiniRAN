#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "miniran/protocol/protocol_message.h"

namespace miniran {

enum class SessionEndReason {
    None,
    CleanDetach,
    DetachNotConfirmed,
    AttachFailed,
    InactivityTimeout,
    StillActiveAtEnd,
    Error
};

inline std::string toString(SessionEndReason reason) {
    switch (reason) {
        case SessionEndReason::None:
            return "none";
        case SessionEndReason::CleanDetach:
            return "clean_detach";
        case SessionEndReason::DetachNotConfirmed:
            return "detach_not_confirmed";
        case SessionEndReason::AttachFailed:
            return "attach_failed";
        case SessionEndReason::InactivityTimeout:
            return "inactivity_timeout";
        case SessionEndReason::StillActiveAtEnd:
            return "still_active_at_end";
        case SessionEndReason::Error:
            return "error";
    }
    return "unknown";
}

struct UeSimulationResult {
    std::uint32_t nodeId = 0;
    std::uint32_t ueId = 0;
    std::uint32_t sessionId = 0;

    bool attachSucceeded = false;
    bool trafficStarted = false;
    bool detachSucceeded = false;
    bool activeAtEnd = false;

    SessionState finalUeState = SessionState::Idle;
    SessionEndReason endReason = SessionEndReason::None;

    std::uint32_t attachRetries = 0;
    std::uint32_t detachRetries = 0;

    std::size_t heartbeatsSent = 0;
    std::size_t heartbeatAcksReceived = 0;

    // Uplink: UE -> CoreNetwork.
    std::size_t uplinkPacketsGenerated = 0;
    std::size_t uplinkBytesGenerated = 0;

    std::size_t uplinkPacketsSent = 0;
    std::size_t uplinkBytesSent = 0;

    std::size_t uplinkPacketsDeliveredByNetwork = 0;
    std::size_t uplinkBytesDeliveredByNetwork = 0;

    std::size_t uplinkPacketsAcceptedByCore = 0;
    std::size_t uplinkBytesAcceptedByCore = 0;

    std::size_t downlinkPacketsGenerated = 0;
    std::size_t downlinkBytesGenerated = 0;

    std::size_t downlinkPacketsSent = 0;
    std::size_t downlinkBytesSent = 0;

    std::size_t downlinkPacketsReceivedByUe = 0;
    std::size_t downlinkBytesReceivedByUe = 0;

    double uplinkThroughputMbps = 0.0;
    double downlinkThroughputMbps = 0.0;

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

    std::size_t uplinkPacketsAcceptedByCore = 0;
    std::size_t uplinkBytesAcceptedByCore = 0;

    std::size_t downlinkPacketsDeliveredToUe = 0;
    std::size_t downlinkBytesDeliveredToUe = 0;

    double totalUplinkThroughputMbps = 0.0;
    double totalDownlinkThroughputMbps = 0.0;

    std::vector<UeSimulationResult> ueResults;
    std::vector<std::string> notes;

    std::string summary() const;
};



}  // namespace miniran
