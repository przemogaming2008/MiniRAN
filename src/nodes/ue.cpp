#include "miniran/nodes/ue.h"

#include <string>
#include <utility>

#include "miniran/protocol/frame_codec.h"

namespace miniran {

Ue::Ue(
    std::uint32_t nodeId,
    std::uint32_t ueId,
    std::uint32_t accessNodeId,
    TransportMode transportMode,
    SessionTimers timers
)
    : nodeId_(nodeId),
      ueId_(ueId),
      accessNodeId_(accessNodeId),
      sessionManager_(ueId, timers) {
    (void)transportMode;
}

std::uint32_t Ue::nodeId() const {
    return nodeId_;
}

std::uint32_t Ue::ueId() const {
    return ueId_;
}

std::uint32_t Ue::sessionId() const {
    return sessionManager_.sessionId();
}

std::uint32_t Ue::lastSessionId() const {
    return sessionManager_.lastSessionId();
}

SessionState Ue::state() const {
    return sessionManager_.state();
}

bool Ue::isAttached() const {
    return sessionManager_.isAttached();
}

bool Ue::detachConfirmed() const {
    return sessionManager_.detachConfirmed();
}

const FlowMetrics& Ue::uplinkMetrics() const {
    return uplinkMetrics_;
}

const FlowMetrics& Ue::downlinkMetrics() const {
    return downlinkMetrics_;
}

const UeProtocolMetrics& Ue::protocolMetrics() const {
    return protocolMetrics_;
}

UeSessionEndReason Ue::endReason() const {
    return sessionManager_.endReason();
}

void Ue::startAttach(std::uint64_t nowMs) {
    if (sessionManager_.beginAttach(nowMs)) {
        ProtocolMessage msg = makeMessage(
            MessageType::AttachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );

        Datagram datagram = Datagram{};
        datagram.fromNodeId = nodeId_;
        datagram.toNodeId = accessNodeId_;
        datagram.enqueueTimeMs = nowMs;
        datagram.controlPlane = true;
        datagram.bytes = FrameCodec::encode(msg);

        outgoing_.push_back(datagram);
    }
}

void Ue::startDetach(std::uint64_t nowMs) {
    if (!sessionManager_.beginDetach(nowMs)) {
        return;
    }

    ProtocolMessage msg = makeMessage(
        MessageType::DetachRequest,
        sessionManager_.ueId(),
        sessionManager_.sessionId(),
        sessionManager_.nextSequenceNumber(),
        nowMs
    );

    Datagram datagram = Datagram{};
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = true;
    datagram.bytes = FrameCodec::encode(msg);

    outgoing_.push_back(datagram);
}

void Ue::sendTraffic(const std::vector<std::uint8_t>& payload, std::uint64_t nowMs) {
    if (!sessionManager_.canSendData()) {
        return;
    }

    ProtocolMessage msg = makeMessage(
        MessageType::Data,
        sessionManager_.ueId(),
        sessionManager_.sessionId(),
        sessionManager_.nextSequenceNumber(),
        nowMs,
        payload
    );

    uplinkMetrics_.packetsSent += 1;
    uplinkMetrics_.bytesSent += payload.size();

    Datagram datagram = Datagram{};
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = false;
    datagram.bytes = FrameCodec::encode(msg);

    outgoing_.push_back(datagram);
}

void Ue::tick(std::uint64_t nowMs) {
    RetryDecision decision = sessionManager_.onTick(nowMs);

    if (!decision.shouldRetransmit) {
        return;
    }

    std::vector<std::uint8_t> encodedMsg;

    if (decision.messageType == MessageType::AttachRequest) {
        protocolMetrics_.attachRetries += 1;

        ProtocolMessage msg = makeMessage(
            MessageType::AttachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );

        encodedMsg = FrameCodec::encode(msg);
    } else if (decision.messageType == MessageType::DetachRequest) {
        protocolMetrics_.detachRetries += 1;

        ProtocolMessage msg = makeMessage(
            MessageType::DetachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );

        encodedMsg = FrameCodec::encode(msg);
    } else if (decision.messageType == MessageType::Heartbeat) {
        protocolMetrics_.heartbeatsSent += 1;

        ProtocolMessage msg = makeMessage(
            MessageType::Heartbeat,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );

        encodedMsg = FrameCodec::encode(msg);
    } else {
        return;
    }

    Datagram datagram = Datagram{};
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = true;
    datagram.bytes = std::move(encodedMsg);

    outgoing_.push_back(datagram);
}

void Ue::onDatagram(const Datagram& datagram, std::uint64_t nowMs) {
    std::string error;
    std::optional<ProtocolMessage> protocolMessageOpt = FrameCodec::decode(datagram.bytes, error);

    if (!protocolMessageOpt) {
        protocolMetrics_.invalidMessagesDropped += 1;
        return;
    }

    ProtocolMessage protocolMessage = *protocolMessageOpt;

    if (protocolMessage.header.ueId != ueId_) {
        protocolMetrics_.invalidMessagesDropped += 1;
        return;
    }

    if (protocolMessage.header.messageType == MessageType::AttachAccept) {
        if (sessionManager_.state() != SessionState::Attaching) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        if (protocolMessage.header.sessionId == 0) {
            protocolMetrics_.protocolErrorsReceived += 1;
            return;
        }

        if (protocolMessage.header.payloadLength != 0 || !protocolMessage.payload.empty()) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        if (!sessionManager_.onAttachAccepted(protocolMessage.header.sessionId, nowMs)) {
            protocolMetrics_.protocolErrorsReceived += 1;
            return;
        }

        return;
    }

    if (protocolMessage.header.messageType == MessageType::DetachAccept) {
        if (sessionManager_.state() != SessionState::Detaching) {
            protocolMetrics_.protocolErrorsReceived += 1;
            return;
        }

        if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        if (!sessionManager_.onDetachAccepted(nowMs)) {
            protocolMetrics_.protocolErrorsReceived += 1;
        }

        return;
    }

    if (protocolMessage.header.messageType == MessageType::HeartbeatAck) {
        if (!sessionManager_.isAttached()) {
            return;
        }

        if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        protocolMetrics_.heartbeatAcksReceived += 1;
        sessionManager_.onHeartbeatResponse(nowMs);
        return;
    }

    if (protocolMessage.header.messageType == MessageType::DataAck) {
        if (!sessionManager_.isAttached()) {
            return;
        }

        if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        return;
    }

    if (protocolMessage.header.messageType == MessageType::DownlinkData) {
        if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        if (!sessionManager_.isAttached()) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        if (protocolMessage.payload.size() != protocolMessage.header.payloadLength) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        if (protocolMessage.payload.empty()) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        downlinkMetrics_.packetsDelivered += 1;
        downlinkMetrics_.bytesDelivered += protocolMessage.payload.size();
        return;
    }

    if (protocolMessage.header.messageType == MessageType::Error) {
        protocolMetrics_.protocolErrorsReceived += 1;
        return;
    }

    protocolMetrics_.invalidMessagesDropped += 1;
}

std::vector<Datagram> Ue::flushOutgoing() {
    std::vector<Datagram> datagrams;
    datagrams.reserve(outgoing_.size());

    while (!outgoing_.empty()) {
        datagrams.push_back(std::move(outgoing_.front()));
        outgoing_.pop_front();
    }

    return datagrams;
}

}  // namespace miniran