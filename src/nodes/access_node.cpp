#include "miniran/nodes/access_node.h"

#include "miniran/protocol/frame_codec.h"

namespace miniran {

AccessNode::AccessNode(std::uint32_t nodeId, std::uint32_t ueNodeId, CoreNetwork coreNetwork)
    : nodeId_(nodeId), ueNodeId_(ueNodeId), coreNetwork_(std::move(coreNetwork)) {}

std::uint32_t AccessNode::nodeId() const {
    return nodeId_;
}

const FlowMetrics& AccessNode::metrics() const {
    return metrics_;
}

const CoreNetwork& AccessNode::coreNetwork() const {
    return coreNetwork_;
}

CoreNetwork& AccessNode::coreNetwork() {
    return coreNetwork_;
}

void AccessNode::tick(std::uint64_t nowMs) {
    coreNetwork_.expireInactiveSessions(nowMs);
}

void AccessNode::onDatagram(const Datagram& datagram, std::uint64_t nowMs) {

    if (datagram.toNodeId != nodeId_) {
        metrics_.packetsDropped += 1;
        return;
    }

    if (datagram.fromNodeId != ueNodeId_) {
        metrics_.packetsDropped += 1;
        return;
    }

    std::string error;
    std::optional<ProtocolMessage> decode_opt = FrameCodec::decode(datagram.bytes, error);

    if (!decode_opt) {
        metrics_.packetsDropped += 1;

        return;
    }

    ProtocolMessage protocolMessage = *decode_opt;

    if (protocolMessage.header.ueId != datagram.fromNodeId) {
        metrics_.packetsDropped += 1;
        return;
    }

    if (protocolMessage.header.ueId != ueNodeId_) {
        metrics_.packetsDropped += 1;
        return;
    }

    if (protocolMessage.header.messageType == MessageType::AttachRequest) {
        metrics_.packetsDelivered += 1;
        metrics_.bytesDelivered += datagram.bytes.size();

        std::optional<ProtocolMessage> msg_opt = coreNetwork_.handleAttachRequest(protocolMessage, nowMs);
        if (msg_opt) {
            queueResponseToUe(*msg_opt, nowMs);
        }
        return;
    }

    if (protocolMessage.header.messageType == MessageType::Heartbeat) {
        metrics_.packetsDelivered += 1;
        metrics_.bytesDelivered += datagram.bytes.size();

        std::optional<ProtocolMessage> msg_opt = coreNetwork_.handleHeartbeat(protocolMessage, nowMs);
        if (msg_opt) {
            queueResponseToUe(*msg_opt, nowMs);
        }
        return;
    }

    if (protocolMessage.header.messageType == MessageType::Data) {
        metrics_.packetsDelivered += 1;
        metrics_.bytesDelivered += datagram.bytes.size();

        coreNetwork_.handleData(protocolMessage, nowMs);
        return;
    }

    if (protocolMessage.header.messageType == MessageType::DetachRequest) {
        metrics_.packetsDelivered += 1;
        metrics_.bytesDelivered += datagram.bytes.size();

        std::optional<ProtocolMessage> msg_opt = coreNetwork_.handleDetachRequest(protocolMessage, nowMs);
        if (msg_opt) {
            queueResponseToUe(*msg_opt, nowMs);
        }
        return;
    }

    metrics_.packetsDropped += 1;

    ProtocolMessage msg = ProtocolMessage{};
    msg.header.messageType = MessageType::Error;
    msg.header.ueId = protocolMessage.header.ueId;
    msg.header.sessionId = protocolMessage.header.sessionId;
    msg.header.sequenceNumber = protocolMessage.header.sequenceNumber;
    msg.header.timestampMs = nowMs;

    queueResponseToUe(msg, nowMs);
}

std::vector<Datagram> AccessNode::flushOutgoing() {
    std::vector<Datagram> datagrams;
    datagrams.reserve(outgoing_.size());
    while (!outgoing_.empty()) {
        datagrams.push_back(std::move(outgoing_.front()));
        outgoing_.pop_front();
    }
    return datagrams;
}

void AccessNode::queueResponseToUe(const ProtocolMessage& message, std::uint64_t nowMs) {
    std::vector<std::uint8_t> encoded = FrameCodec::encode(message);

    Datagram response{};
    response.fromNodeId = nodeId_;
    response.toNodeId = ueNodeId_;
    response.enqueueTimeMs = nowMs;
    response.controlPlane = true;
    response.bytes = encoded;

    metrics_.packetsSent += 1;
    metrics_.bytesSent += response.bytes.size();

    outgoing_.push_back(response);
}

}  // namespace miniran
