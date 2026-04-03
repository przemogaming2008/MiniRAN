#include "miniran/core/core_network.h"

namespace miniran {

CoreNetwork::CoreNetwork(SessionTimers timers) : timers_(timers) {}

bool CoreNetwork::hasActiveSession(std::uint32_t ueId) const {
    const auto iterator = sessions_.find(ueId);
    return iterator != sessions_.end() && iterator->second.state == SessionState::Attached;
}

std::size_t CoreNetwork::activeSessionCount() const {
    std::size_t count = 0;
    for (const auto& [ueId, session] : sessions_) {
        (void)ueId;
        if (session.state == SessionState::Attached) {
            ++count;
        }
    }
    return count;
}

std::optional<ProtocolMessage> CoreNetwork::handleAttachRequest(const ProtocolMessage& request, std::uint64_t nowMs) {
    (void)request;
    (void)nowMs;
    // TODO(student):
    // 1. Allocate a new session id (or reuse the existing one if policy allows).
    // 2. Create/update SessionRecord with state Attached.
    // 3. Return AttachAccept with the session id.
    // 4. Optionally reject malformed requests using AttachReject or Error.
    return std::nullopt;
}

std::optional<ProtocolMessage> CoreNetwork::handleDetachRequest(const ProtocolMessage& request, std::uint64_t nowMs) {
    (void)request;
    (void)nowMs;
    // TODO(student):
    // 1. Find the UE session.
    // 2. Mark it Released or remove it.
    // 3. Return DetachAccept when successful.
    return std::nullopt;
}

std::optional<ProtocolMessage> CoreNetwork::handleHeartbeat(const ProtocolMessage& request, std::uint64_t nowMs) {
    (void)request;
    (void)nowMs;
    // TODO(student):
    // Refresh lastSeenMs and reply with HeartbeatAck.
    return std::nullopt;
}

void CoreNetwork::handleData(const ProtocolMessage& request, std::uint64_t nowMs) {
    (void)request;
    (void)nowMs;
    // TODO(student):
    // 1. Accept data only for active sessions.
    // 2. Count delivered bytes and packets.
    // 3. Refresh lastSeenMs for the session.
}

void CoreNetwork::expireInactiveSessions(std::uint64_t nowMs) {
    (void)nowMs;
    // TODO(student):
    // Remove or close sessions that exceeded inactivity timeout.
}

std::size_t CoreNetwork::deliveredBytes() const {
    return deliveredBytes_;
}

std::size_t CoreNetwork::deliveredPackets() const {
    return deliveredPackets_;
}

const std::unordered_map<std::uint32_t, SessionRecord>& CoreNetwork::sessions() const {
    return sessions_;
}

}  // namespace miniran
