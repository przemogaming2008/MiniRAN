#pragma once

#include <cstdint>

#include "miniran/protocol/protocol_message.h"

namespace miniran {

struct SessionTimers {
    std::uint32_t attachTimeoutMs = 150;
    std::uint32_t detachTimeoutMs = 150;
    std::uint32_t heartbeatIntervalMs = 200;
    std::uint32_t inactivityTimeoutMs = 700;
    std::uint32_t maxAttachRetries = 3;
    std::uint32_t maxDetachRetries = 2;
};

struct RetryDecision {
    bool shouldRetransmit = false;
    MessageType messageType = MessageType::Error;
};

class SessionManager {
public:
    explicit SessionManager(std::uint32_t ueId, SessionTimers timers = {});

    std::uint32_t ueId() const;
    std::uint32_t sessionId() const;
    SessionState state() const;
    bool isAttached() const;
    bool canSendData() const;

    std::uint32_t nextSequenceNumber();

    bool beginAttach(std::uint64_t nowMs);
    bool onAttachAccepted(std::uint32_t sessionId, std::uint64_t nowMs);
    bool beginDetach(std::uint64_t nowMs);
    bool onDetachAccepted(std::uint64_t nowMs);
    void onHeartbeatResponse(std::uint64_t nowMs);
    RetryDecision onTick(std::uint64_t nowMs);
    void reset();

private:
    std::uint32_t ueId_ = 0;
    SessionTimers timers_{};
    SessionState state_ = SessionState::Idle;
    std::uint32_t sessionId_ = 0;
    std::uint32_t nextSequenceNumber_ = 1;
    std::uint64_t lastControlTxMs_ = 0;
    std::uint64_t lastHeartbeatAckMs_ = 0;
    std::uint32_t attachRetryCount_ = 0;
    std::uint32_t detachRetryCount_ = 0;
};

}  // namespace miniran
