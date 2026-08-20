#include "miniran/nodes/ue.h"

#include <optional>
#include <string>
#include <vector>

#include "miniran/protocol/frame_codec.h"

namespace miniran {

Ue::Ue(std::uint32_t nodeId,
       std::uint32_t accessNodeId,
       SessionTimers timers)
    : nodeId_(nodeId),
      accessNodeId_(accessNodeId),
      sessionManager_(nodeId, timers)
{
}

std::uint32_t Ue::nodeId() const
{
    return nodeId_;
}

SessionState Ue::state() const
{
    return sessionManager_.state();
}

bool Ue::isAttached() const
{
    return sessionManager_.isAttached();
}

const FlowMetrics& Ue::metrics() const
{
    return metrics_;
}

void Ue::startAttach(std::uint64_t nowMs)
{
    if (!sessionManager_.beginAttach(nowMs)) {
        return;
    }

    ProtocolMessage msg = makeMessage(
        MessageType::AttachRequest,
        sessionManager_.ueId(),
        sessionManager_.sessionId(),
        sessionManager_.nextSequenceNumber(),
        nowMs
    );

    Datagram datagram{};
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = true;
    datagram.bytes = FrameCodec::encode(msg);

    metrics_.packetsSent += 1;
    metrics_.bytesSent += datagram.bytes.size();

    outgoing_.push_back(datagram);
}

void Ue::startDetach(std::uint64_t nowMs)
{
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

    Datagram datagram{};
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = true;
    datagram.bytes = FrameCodec::encode(msg);

    metrics_.packetsSent += 1;
    metrics_.bytesSent += datagram.bytes.size();

    outgoing_.push_back(datagram);
}

void Ue::sendTraffic(const std::vector<std::uint8_t>& payload,
                     std::uint64_t nowMs)
{
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

    Datagram datagram{};
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = false;
    datagram.bytes = FrameCodec::encode(msg);

    metrics_.packetsSent += 1;
    metrics_.bytesSent += datagram.bytes.size();

    outgoing_.push_back(datagram);
}

void Ue::tick(std::uint64_t nowMs)
{
    RetryDecision decision = sessionManager_.onTick(nowMs);

    if (!decision.shouldRetransmit) {
        return;
    }

    ProtocolMessage msg{};

    if (decision.messageType == MessageType::AttachRequest) {
        msg = makeMessage(
            MessageType::AttachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );
    } else if (decision.messageType == MessageType::DetachRequest) {
        msg = makeMessage(
            MessageType::DetachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );
    } else if (decision.messageType == MessageType::Heartbeat) {
        msg = makeMessage(
            MessageType::Heartbeat,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );
    } else {
        return;
    }

    Datagram datagram{};
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = true;
    datagram.bytes = FrameCodec::encode(msg);

    metrics_.packetsSent += 1;
    metrics_.bytesSent += datagram.bytes.size();

    outgoing_.push_back(datagram);
}

void Ue::onDatagram(const Datagram& datagram, std::uint64_t nowMs)
{
    if (datagram.toNodeId != nodeId_) {
        return;
    }

    if (datagram.fromNodeId != accessNodeId_) {
        return;
    }

    std::string error;
    std::optional<ProtocolMessage> protocolMessageOpt =
        FrameCodec::decode(datagram.bytes, error);

    if (!protocolMessageOpt) {
        return;
    }

    ProtocolMessage protocolMessage = *protocolMessageOpt;

    if (protocolMessage.header.ueId != nodeId_) {
        return;
    }

    if (protocolMessage.header.messageType == MessageType::AttachAccept) {
        if (protocolMessage.header.sessionId == 0) {
            return;
        }

        sessionManager_.onAttachAccepted(
            protocolMessage.header.sessionId,
            nowMs
        );
        return;
    }

    if (protocolMessage.header.messageType == MessageType::DetachAccept) {
        if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
            return;
        }

        sessionManager_.onDetachAccepted(nowMs);
        return;
    }

    if (protocolMessage.header.messageType == MessageType::HeartbeatAck) {
        if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
            return;
        }

        sessionManager_.onHeartbeatResponse(nowMs);
        return;
    }

    if (protocolMessage.header.messageType == MessageType::AttachReject) {
        sessionManager_.onAttachRejected(nowMs);
        return;
    }

    if (protocolMessage.header.messageType == MessageType::Error) {
        if (sessionManager_.state() == SessionState::Attaching) {
            sessionManager_.reset();
        }

        return;
    }
}

std::vector<Datagram> Ue::flushOutgoing()
{
    std::vector<Datagram> datagrams;
    datagrams.reserve(outgoing_.size());

    while (!outgoing_.empty()) {
        datagrams.push_back(std::move(outgoing_.front()));
        outgoing_.pop_front();
    }

    return datagrams;
}

}  // namespace miniran