#include "miniran/core/core_network.h"

#include <cstdint>
#include <optional>

namespace
{

bool elapsedAtLeast(std::uint64_t nowMs,
                    std::uint64_t sinceMs,
                    std::uint64_t durationMs)
{
    if (nowMs < sinceMs) {
        return false;
    }

    return (nowMs - sinceMs) >= durationMs;
}

}  // namespace

namespace miniran {

CoreNetwork::CoreNetwork(SessionTimers timers)
    : timers_(timers)
{
}

bool CoreNetwork::hasActiveSession(std::uint32_t ueId) const
{
    const auto iterator = sessions_.find(ueId);

    return iterator != sessions_.end() &&
           iterator->second.state == SessionState::Attached;
}

std::size_t CoreNetwork::activeSessionCount() const
{
    std::size_t count = 0;

    for (const auto& entry : sessions_) {
        const auto& session = entry.second;

        if (session.state == SessionState::Attached) {
            ++count;
        }
    }

    return count;
}

std::optional<ProtocolMessage>
CoreNetwork::handleAttachRequest(const ProtocolMessage& request,
                                 std::uint64_t nowMs)
{
    if (request.header.messageType != MessageType::AttachRequest) {
        return std::nullopt;
    }

    if (request.header.ueId == 0 ||
        request.header.sequenceNumber == 0 ||
        request.header.sessionId != 0)
    {
        ProtocolMessage protocolMessage{};
        protocolMessage.header.timestampMs = nowMs;
        protocolMessage.header.ueId = request.header.ueId;
        protocolMessage.header.sequenceNumber =
            request.header.sequenceNumber;
        protocolMessage.header.sessionId = request.header.sessionId;
        protocolMessage.header.messageType = MessageType::Error;

        return protocolMessage;
    }

    ProtocolMessage protocolMessage{};
    protocolMessage.header.timestampMs = nowMs;
    protocolMessage.header.ueId = request.header.ueId;
    protocolMessage.header.sequenceNumber = request.header.sequenceNumber;

    if (hasActiveSession(request.header.ueId)) {
        auto& session = sessions_[request.header.ueId];

        session.lastSeenMs = nowMs;

        protocolMessage.header.messageType = MessageType::AttachAccept;
        protocolMessage.header.sessionId = session.sessionId;

        return protocolMessage;
    }

    const std::uint32_t newSessionId = nextSessionId_;

    SessionRecord sessionRecord{};
    sessionRecord.ueId = request.header.ueId;
    sessionRecord.state = SessionState::Attached;
    sessionRecord.sessionId = newSessionId;
    sessionRecord.attachedAtMs = nowMs;
    sessionRecord.lastSeenMs = nowMs;
    sessionRecord.lastAcceptedSequenceNumber =
        request.header.sequenceNumber;

    sessions_[request.header.ueId] = sessionRecord;

    protocolMessage.header.messageType = MessageType::AttachAccept;
    protocolMessage.header.sessionId = newSessionId;

    ++nextSessionId_;

    return protocolMessage;
}

std::optional<ProtocolMessage>
CoreNetwork::handleDetachRequest(const ProtocolMessage& request,
                                 std::uint64_t nowMs)
{
    if (request.header.messageType != MessageType::DetachRequest) {
        return std::nullopt;
    }

    ProtocolMessage protocolMessage{};
    protocolMessage.header.timestampMs = nowMs;
    protocolMessage.header.ueId = request.header.ueId;
    protocolMessage.header.sequenceNumber = request.header.sequenceNumber;
    protocolMessage.header.sessionId = request.header.sessionId;

    auto it = sessions_.find(request.header.ueId);

    if (it == sessions_.end()) {
        protocolMessage.header.messageType = MessageType::DetachAccept;
        return protocolMessage;
    }

    if (it->second.sessionId != request.header.sessionId) {
        protocolMessage.header.messageType = MessageType::Error;
        return protocolMessage;
    }

    if (request.header.sequenceNumber <=
        it->second.lastAcceptedSequenceNumber)
    {
        return std::nullopt;
    }

    it->second.lastAcceptedSequenceNumber =
        request.header.sequenceNumber;

    sessions_.erase(it);

    protocolMessage.header.messageType = MessageType::DetachAccept;

    return protocolMessage;
}

std::optional<ProtocolMessage>
CoreNetwork::handleHeartbeat(const ProtocolMessage& request,
                             std::uint64_t nowMs)
{
    if (request.header.messageType != MessageType::Heartbeat) {
        return std::nullopt;
    }

    auto it = sessions_.find(request.header.ueId);
    if (it == sessions_.end()) {
        return std::nullopt;
    }

    SessionRecord& session = it->second;

    if (session.state != SessionState::Attached) {
        return std::nullopt;
    }

    if (request.header.sessionId != session.sessionId) {
        return std::nullopt;
    }

    if (request.header.sequenceNumber <=
        session.lastAcceptedSequenceNumber)
    {
        return std::nullopt;
    }

    session.lastAcceptedSequenceNumber = request.header.sequenceNumber;
    session.lastSeenMs = nowMs;

    ProtocolMessage msg = makeMessage(
        MessageType::HeartbeatAck,
        request.header.ueId,
        session.sessionId,
        request.header.sequenceNumber,
        nowMs
    );

    return msg;
}

void CoreNetwork::handleData(const ProtocolMessage& request,
                             std::uint64_t nowMs)
{
    if (request.header.messageType != MessageType::Data) {
        return;
    }

    auto it = sessions_.find(request.header.ueId);
    if (it == sessions_.end()) {
        return;
    }

    SessionRecord& session = it->second;

    if (session.state != SessionState::Attached) {
        return;
    }

    if (request.header.sessionId != session.sessionId) {
        return;
    }

    if (request.payload.size() != request.header.payloadLength) {
        return;
    }

    if (request.payload.empty()) {
        return;
    }

    if (request.header.sequenceNumber <=
        session.lastAcceptedSequenceNumber)
    {
        return;
    }

    session.lastAcceptedSequenceNumber = request.header.sequenceNumber;

    session.deliveredBytes += request.payload.size();
    session.deliveredPackets += 1;

    deliveredBytes_ += request.payload.size();
    deliveredPackets_ += 1;

    session.lastSeenMs = nowMs;
}

void CoreNetwork::expireInactiveSessions(std::uint64_t nowMs)
{
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        const SessionRecord& session = it->second;

        if (elapsedAtLeast(nowMs,
                           session.lastSeenMs,
                           timers_.inactivityTimeoutMs))
        {
            ++expiredSessions_;
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

std::size_t CoreNetwork::deliveredBytes() const
{
    return deliveredBytes_;
}

std::size_t CoreNetwork::deliveredPackets() const
{
    return deliveredPackets_;
}

const std::unordered_map<std::uint32_t, SessionRecord>&
CoreNetwork::sessions() const
{
    return sessions_;
}

}  // namespace miniran