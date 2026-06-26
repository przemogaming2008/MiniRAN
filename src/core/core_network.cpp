#include "miniran/core/core_network.h"

#include <utility>

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

    //Optionally reject malformed requests using AttachReject or Error.
    if (request.header.messageType != MessageType::AttachRequest) {
        return std::nullopt;
    }

    if (request.header.ueId == 0 ||
        request.header.sequenceNumber == 0 ||
        request.header.sessionId != 0) {
        countProtocolRejection(request.header.ueId);

        ProtocolMessage protocolMessage{};
        protocolMessage.header.timestampMs = nowMs;
        protocolMessage.header.ueId = request.header.ueId;
        protocolMessage.header.sequenceNumber = request.header.sequenceNumber;
        protocolMessage.header.sessionId = request.header.sessionId;
        protocolMessage.header.messageType = MessageType::Error;

        return protocolMessage;
    }

    ProtocolMessage protocolMessage = ProtocolMessage{};
    protocolMessage.header.timestampMs = nowMs;
    protocolMessage.header.ueId = request.header.ueId;
    protocolMessage.header.sequenceNumber = request.header.sequenceNumber;

    if (hasActiveSession(request.header.ueId)) {
        //REFRESH SESSION
        auto& session = sessions_[request.header.ueId];

        session.lastSeenMs = nowMs;

        protocolMessage.header.messageType = MessageType::AttachAccept;
        protocolMessage.header.sessionId = session.sessionId;

        return protocolMessage;
    }

    //Allocate a new session id for new sessions.
    const std::uint32_t newSessionId = nextSessionId_;

    //Create/update SessionRecord with state Attached.
    SessionRecord sessionRecord = SessionRecord{};
    sessionRecord.ueId = request.header.ueId;
    sessionRecord.state = SessionState::Attached;
    sessionRecord.sessionId = newSessionId;
    sessionRecord.attachedAtMs = nowMs;
    sessionRecord.lastSeenMs = nowMs;
    sessionRecord.endReason = CoreSessionEndReason::None;

    sessions_[request.header.ueId] = sessionRecord;

    //Return AttachAccept with the session id.
    protocolMessage.header.messageType = MessageType::AttachAccept;
    protocolMessage.header.sessionId = newSessionId;

    ++nextSessionId_;

    return protocolMessage;
}

std::optional<ProtocolMessage> CoreNetwork::handleDetachRequest(const ProtocolMessage& request, std::uint64_t nowMs) {
    if (request.header.messageType != MessageType::DetachRequest) {
        countProtocolRejection(request.header.ueId);

        return makeMessage(
            MessageType::Error,
            request.header.ueId,
            request.header.sessionId,
            request.header.sequenceNumber,
            nowMs
        );
    }

    const auto ueId = request.header.ueId;
    auto sessionIt = sessions_.find(ueId);

    if (sessionIt == sessions_.end()) {
        const auto historyIt = sessionHistory_.find(ueId);

        if (historyIt != sessionHistory_.end()) {
            for (auto recordIt = historyIt->second.rbegin();
                recordIt != historyIt->second.rend();
                ++recordIt) {

                if (recordIt->sessionId == request.header.sessionId &&
                    recordIt->endReason == CoreSessionEndReason::CleanDetach) {

                    return makeMessage(
                        MessageType::DetachAccept,
                        ueId,
                        request.header.sessionId,
                        request.header.sequenceNumber,
                        nowMs
                    );
                }
            }
        }

        countProtocolRejection(ueId);

        return makeMessage(
            MessageType::Error,
            ueId,
            request.header.sessionId,
            request.header.sequenceNumber,
            nowMs
        );
    }

    auto& session = sessionIt->second;

    if (request.header.sessionId == 0 ||
        request.header.sessionId != session.sessionId) {

        session.protocolErrors += 1;
        countProtocolRejection(ueId);

        return makeMessage(
            MessageType::Error,
            ueId,
            request.header.sessionId,
            request.header.sequenceNumber,
            nowMs
        );
    }

    storeFinishedSession(session, CoreSessionEndReason::CleanDetach, nowMs);
    sessions_.erase(sessionIt);

    return makeMessage(
        MessageType::DetachAccept,
        ueId,
        request.header.sessionId,
        request.header.sequenceNumber,
        nowMs
    );
}

std::optional<ProtocolMessage> CoreNetwork::handleHeartbeat(const ProtocolMessage& request, std::uint64_t nowMs) {

    //Refresh lastSeenMs and reply with HeartbeatAck.
    if (request.header.messageType != MessageType::Heartbeat) {
        return std::nullopt;
    }

    auto it = sessions_.find(request.header.ueId);
    if (it == sessions_.end()) {
        countProtocolRejection(request.header.ueId);
        return std::nullopt;
    }

    SessionRecord& session = it->second;

    if (session.state != SessionState::Attached) {
        ++session.protocolErrors;
        countProtocolRejection(request.header.ueId);
        return std::nullopt;
    }

    if (request.header.sessionId != session.sessionId) {
        ++session.protocolErrors;
        countProtocolRejection(request.header.ueId);

        ProtocolMessage msg = makeMessage(
            MessageType::Error,
            request.header.ueId,
            request.header.sessionId,
            request.header.sequenceNumber,
            nowMs
        );

        return msg;
    }

    session.lastSeenMs = nowMs;
    session.heartbeatsReceived += 1;
    session.heartbeatAcksSent += 1;

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

    if (request.header.messageType != MessageType::Data) {
        return;
    }

    auto it = sessions_.find(request.header.ueId);
    if (it == sessions_.end()) {
        countProtocolRejection(request.header.ueId);
        return;
    }

    SessionRecord& session = it->second;

    //Accept data only for active sessions.
    if (session.state != SessionState::Attached) {
        ++session.protocolErrors;
        countProtocolRejection(request.header.ueId);
        return;
    }

    if (request.header.sessionId != session.sessionId) {
        ++session.protocolErrors;
        countProtocolRejection(request.header.ueId);
        return;
    }

    if (request.payload.size() != request.header.payloadLength) {
        ++session.protocolErrors;
        countProtocolRejection(request.header.ueId);
        return;
    }

    if (request.payload.empty()) {
        ++session.protocolErrors;
        countProtocolRejection(request.header.ueId);
        return;
    }

    //Count delivered bytes and packets.
    session.deliveredBytes += request.payload.size();
    session.deliveredPackets += 1;

    deliveredBytes_ += request.payload.size();
    deliveredPackets_ += 1;

    //Refresh lastSeenMs for the session.
    session.lastSeenMs = nowMs;
}

std::optional<ProtocolMessage> CoreNetwork::makeDownlinkData(
    std::uint32_t ueId,
    std::uint32_t sequenceNumber,
    std::uint64_t nowMs,
    const std::vector<std::uint8_t>& payload
) {
    if (ueId == 0) {
        countProtocolRejection(ueId);
        return std::nullopt;
    }

    if (payload.empty()) {
        countProtocolRejection(ueId);
        return std::nullopt;
    }

    auto sessionIt = sessions_.find(ueId);

    if (sessionIt == sessions_.end()) {
        countProtocolRejection(ueId);
        return std::nullopt;
    }

    auto& session = sessionIt->second;

    if (session.state != SessionState::Attached) {
        session.protocolErrors += 1;
        countProtocolRejection(ueId);
        return std::nullopt;
    }

    session.lastSeenMs = nowMs;

    return makeMessage(
        MessageType::DownlinkData,
        ueId,
        session.sessionId,
        sequenceNumber,
        nowMs,
        payload
    );
}

void CoreNetwork::expireInactiveSessions(std::uint64_t nowMs) {

    //Remove sessions that exceeded inactivity timeout.
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if ((nowMs - it->second.lastSeenMs) >= timers_.inactivityTimeoutMs) {
            storeFinishedSession(it->second, CoreSessionEndReason::InactivityTimeout, nowMs);

            ++expiredSessions_;
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

std::size_t CoreNetwork::deliveredBytesForUe(std::uint32_t ueId) const {
    std::size_t bytes = 0;

    if (const auto active = sessions_.find(ueId); active != sessions_.end()) {
        bytes += active->second.deliveredBytes;
    }

    if (const auto history = sessionHistory_.find(ueId); history != sessionHistory_.end()) {
        for (const auto& session : history->second) {
            bytes += session.deliveredBytes;
        }
    }

    return bytes;
}

std::size_t CoreNetwork::deliveredPacketsForUe(std::uint32_t ueId) const {
    std::size_t packets = 0;

    if (const auto active = sessions_.find(ueId); active != sessions_.end()) {
        packets += active->second.deliveredPackets;
    }

    if (const auto history = sessionHistory_.find(ueId); history != sessionHistory_.end()) {
        for (const auto& session : history->second) {
            packets += session.deliveredPackets;
        }
    }

    return packets;
}

std::uint64_t CoreNetwork::expiredSessions() const {
    return expiredSessions_;
}

std::uint64_t CoreNetwork::protocolRejectedPackets() const {
    return protocolRejectedPackets_;
}

const std::unordered_map<std::uint32_t, SessionRecord>& CoreNetwork::sessions() const {
    return sessions_;
}

const std::unordered_map<std::uint32_t, std::vector<SessionRecord>>& CoreNetwork::sessionHistory() const {
    return sessionHistory_;
}

void CoreNetwork::storeFinishedSession(SessionRecord record, CoreSessionEndReason reason, std::uint64_t nowMs) {
    record.state = SessionState::Released;
    record.endReason = reason;
    record.endedAtMs = nowMs;

    sessionHistory_[record.ueId].push_back(std::move(record));
}

void CoreNetwork::countProtocolRejection(std::uint32_t ueId) {
    (void)ueId;
    ++protocolRejectedPackets_;
}

}  // namespace miniran
