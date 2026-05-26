#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "miniran/protocol/protocol_message.h"
#include "miniran/protocol/session_manager.h"

namespace miniran {

enum class CoreSessionEndReason {
    None,
    CleanDetach,
    InactivityTimeout,
    ProtocolError,
    RetryLimitExceeded
};

struct SessionRecord {
    std::uint32_t ueId = 0;
    std::uint32_t sessionId = 0;
    SessionState state = SessionState::Idle;

    std::uint64_t attachedAtMs = 0;
    std::uint64_t lastSeenMs = 0;
    std::uint64_t endedAtMs = 0;

    std::size_t deliveredBytes = 0;
    std::size_t deliveredPackets = 0;

    std::size_t heartbeatsReceived = 0;
    std::size_t heartbeatAcksSent = 0;
    std::size_t protocolErrors = 0;

    CoreSessionEndReason endReason = CoreSessionEndReason::None;
};

class CoreNetwork {
public:
    explicit CoreNetwork(SessionTimers timers = {});

    bool hasActiveSession(std::uint32_t ueId) const;
    std::size_t activeSessionCount() const;

    std::optional<ProtocolMessage> handleAttachRequest(const ProtocolMessage& request, std::uint64_t nowMs);
    std::optional<ProtocolMessage> handleDetachRequest(const ProtocolMessage& request, std::uint64_t nowMs);
    std::optional<ProtocolMessage> handleHeartbeat(const ProtocolMessage& request, std::uint64_t nowMs);
    void handleData(const ProtocolMessage& request, std::uint64_t nowMs);
    void expireInactiveSessions(std::uint64_t nowMs);

    std::size_t deliveredBytes() const;
    std::size_t deliveredPackets() const;

    std::size_t deliveredBytesForUe(std::uint32_t ueId) const;
    std::size_t deliveredPacketsForUe(std::uint32_t ueId) const;

    std::uint64_t expiredSessions() const;
    std::uint64_t protocolRejectedPackets() const;

    const std::unordered_map<std::uint32_t, SessionRecord>& sessions() const;
    const std::unordered_map<std::uint32_t, std::vector<SessionRecord>>& sessionHistory() const;

private:
    void storeFinishedSession(SessionRecord record, CoreSessionEndReason reason, std::uint64_t nowMs);
    void countProtocolRejection(std::uint32_t ueId);

    SessionTimers timers_{};

    std::unordered_map<std::uint32_t, SessionRecord> sessions_;
    std::unordered_map<std::uint32_t, std::vector<SessionRecord>> sessionHistory_;

    std::uint32_t nextSessionId_ = 1000;

    std::size_t deliveredBytes_ = 0;
    std::size_t deliveredPackets_ = 0;

    std::uint64_t expiredSessions_ = 0;
    std::uint64_t protocolRejectedPackets_ = 0;
};

}  // namespace miniran
