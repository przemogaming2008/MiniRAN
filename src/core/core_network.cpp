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
    // (void)request;
    //(void)nowMs;
    // TODO(student):
    // 4. Optionally reject malformed requests using AttachReject or Error.
    if(request.header.messageType != MessageType::AttachRequest){
        return std::nullopt;
    }
    // 1. Allocate a new session id (or reuse the existing one if policy allows).
    std::uint32_t new_session_id = nextSessionId_;

    ProtocolMessage protocolMessage = ProtocolMessage{};
    protocolMessage.header.timestampMs = nowMs;
    protocolMessage.header.ueId = request.header.ueId;
    protocolMessage.header.sequenceNumber = request.header.sequenceNumber;

    if(hasActiveSession(request.header.ueId)){
        //REFRESH SESSION
        auto& session = sessions_[request.header.ueId];

        session.lastSeenMs = nowMs;

        protocolMessage.header.messageType = MessageType::AttachAccept;
        protocolMessage.header.sessionId = session.sessionId;
        return protocolMessage;
    }
    // 2. Create/update SessionRecord with state Attached.
    SessionRecord sessionRecord = SessionRecord{};
    sessionRecord.ueId = request.header.ueId;
    sessionRecord.state = SessionState::Attached;
    sessionRecord.sessionId = new_session_id;
    sessionRecord.attachedAtMs = nowMs;
    sessionRecord.lastSeenMs = nowMs;

    sessions_[request.header.ueId] = sessionRecord;
    // 3. Return AttachAccept with the session id.
    protocolMessage.header.messageType = MessageType::AttachAccept;
    protocolMessage.header.sessionId = new_session_id;
    
    ++nextSessionId_; 
    return protocolMessage;

}

std::optional<ProtocolMessage> CoreNetwork::handleDetachRequest(const ProtocolMessage& request, std::uint64_t nowMs) {
    // (void)request;
    // (void)nowMs;
    // TODO(student):
    if(request.header.messageType != MessageType::DetachRequest){
        return std::nullopt;
    }
    // 1. Find the UE session.
    std::uint32_t ueId = request.header.ueId;

    ProtocolMessage protocolMessage = ProtocolMessage{};
    protocolMessage.header.timestampMs = nowMs;
    protocolMessage.header.ueId = request.header.ueId;
    protocolMessage.header.sequenceNumber = request.header.sequenceNumber;
    
    protocolMessage.header.sessionId = request.header.sessionId;
    
    auto it = sessions_.find(ueId);
    if (it != sessions_.end()) {
        // 2. Mark it Released or remove it.
        sessions_.erase(ueId);

         // 3. Return DetachAccept when successful.
        protocolMessage.header.messageType = MessageType::DetachAccept;
        return protocolMessage;
    } else {
        protocolMessage.header.messageType = MessageType::Error;
        return protocolMessage;
    }

}

std::optional<ProtocolMessage> CoreNetwork::handleHeartbeat(const ProtocolMessage& request, std::uint64_t nowMs) {
    //(void)request;
    //(void)nowMs;
    // TODO(student):
    // Refresh lastSeenMs and reply with HeartbeatAck.
    if (request.header.messageType != MessageType::Heartbeat) {
        return std::nullopt;
    }

    auto it = sessions_.find(request.header.ueId);
    if (it == sessions_.end()) {
        //session not found, ignore
        return std::nullopt;
    }

    SessionRecord& session = it->second;

    if (session.state != SessionState::Attached) {
        //not attached, ignore
        return std::nullopt;
    }

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

void CoreNetwork::handleData(const ProtocolMessage& request, std::uint64_t nowMs) {
    //(void)request;
    //(void)nowMs;
    // TODO(student):
    if (request.header.messageType != MessageType::Data) {
        return;
    }
    auto it = sessions_.find(request.header.ueId);
    if (it == sessions_.end()) {
        return;
    }
    SessionRecord& session = it->second;
    // 1. Accept data only for active sessions.
    if (session.state != SessionState::Attached) {
        //not attached, ignore
        return;
    }
    // 2. Count delivered bytes and packets.
    session.deliveredBytes += request.header.payloadLength;
    session.deliveredPackets += 1;
    deliveredBytes_ += request.header.payloadLength;
    deliveredPackets_ += 1;
    // 3. Refresh lastSeenMs for the session.
    session.lastSeenMs = nowMs;
}

void CoreNetwork::expireInactiveSessions(std::uint64_t nowMs) {
    //(void)nowMs;
    // TODO(student):
    // Remove or close sessions that exceeded inactivity timeout.
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if( (nowMs-(it->second).lastSeenMs) >= timers_.inactivityTimeoutMs ) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }

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
