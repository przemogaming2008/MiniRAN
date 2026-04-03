#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

#include "miniran/protocol/protocol_message.h"
#include "miniran/protocol/session_manager.h"

namespace miniran {

struct SessionRecord {
    std::uint32_t ueId = 0;
    std::uint32_t sessionId = 0;
    SessionState state = SessionState::Idle;
    std::uint64_t attachedAtMs = 0;
    std::uint64_t lastSeenMs = 0;
    std::size_t deliveredBytes = 0;
    std::size_t deliveredPackets = 0;
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
    const std::unordered_map<std::uint32_t, SessionRecord>& sessions() const;

private:
    SessionTimers timers_{};
    std::unordered_map<std::uint32_t, SessionRecord> sessions_;
    std::uint32_t nextSessionId_ = 1000;
    std::size_t deliveredBytes_ = 0;
    std::size_t deliveredPackets_ = 0;
};

}  // namespace miniran
