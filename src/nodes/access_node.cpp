#include "miniran/nodes/access_node.h"

#include <utility>

#include "miniran/protocol/frame_codec.h"

namespace miniran {

AccessNode::AccessNode(std::uint32_t nodeId, CoreNetwork coreNetwork)
    : nodeId_(nodeId), coreNetwork_(std::move(coreNetwork)) {}

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

    //Decode datagram bytes using FrameCodec.
    std::string error;
    std::optional<ProtocolMessage> decode_opt = FrameCodec::decode(datagram.bytes, error);

    if (!decode_opt) {
        metrics_.packetsDropped += 1;
        //Malformed frames are dropped silently because no valid header exists to build a protocol-level Error response.
        return;
    }

    ProtocolMessage protocolMessage = *decode_opt;

    if (protocolMessage.header.ueId != 0) {
        rememberUeRoute(protocolMessage.header.ueId, datagram.fromNodeId);
    }

    //Route messages by type:
    //    - AttachRequest  -> coreNetwork_.handleAttachRequest()
    //    - Heartbeat      -> coreNetwork_.handleHeartbeat()
    //    - Data           -> coreNetwork_.handleData()
    //    - DetachRequest  -> coreNetwork_.handleDetachRequest()
    if (protocolMessage.header.messageType == MessageType::AttachRequest) {
        metrics_.packetsDelivered += 1;
        metrics_.bytesDelivered += datagram.bytes.size();

        std::optional<ProtocolMessage> msg_opt =
            coreNetwork_.handleAttachRequest(protocolMessage, nowMs);

        if (msg_opt) {
            queueResponseToUe(*msg_opt, nowMs, datagram.fromNodeId);
        }

        return;
    }

    if (protocolMessage.header.messageType == MessageType::Heartbeat) {
        metrics_.packetsDelivered += 1;
        metrics_.bytesDelivered += datagram.bytes.size();

        std::optional<ProtocolMessage> msg_opt =
            coreNetwork_.handleHeartbeat(protocolMessage, nowMs);

        if (msg_opt) {
            queueResponseToUe(*msg_opt, nowMs, datagram.fromNodeId);
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

        std::optional<ProtocolMessage> msg_opt =
            coreNetwork_.handleDetachRequest(protocolMessage, nowMs);

        if (msg_opt) {
            queueResponseToUe(*msg_opt, nowMs, datagram.fromNodeId);
        }

        return;
    }

    metrics_.packetsDropped += 1;

    //Unsupported message type: send Error response.
    ProtocolMessage msg = ProtocolMessage{};
    msg.header.messageType = MessageType::Error;
    msg.header.ueId = protocolMessage.header.ueId;
    msg.header.sessionId = protocolMessage.header.sessionId;
    msg.header.sequenceNumber = protocolMessage.header.sequenceNumber;
    msg.header.timestampMs = nowMs;

    queueResponseToUe(msg, nowMs, datagram.fromNodeId);
}

bool AccessNode::queueDownlinkToUe(const ProtocolMessage& message, std::uint64_t nowMs) {
    const auto route = ueNodeByUeId_.find(message.header.ueId);

    if (route == ueNodeByUeId_.end()) {
        metrics_.packetsDropped += 1;
        return false;
    }

    const bool controlPlane =
        message.header.messageType != MessageType::DownlinkData;

    queueDatagramToNode(message, nowMs, route->second, controlPlane);
    return true;
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

void AccessNode::queueResponseToUe(
    const ProtocolMessage& message,
    std::uint64_t nowMs,
    std::uint32_t targetNodeId
) {
    queueDatagramToNode(message, nowMs, targetNodeId, true);
}

void AccessNode::queueDatagramToNode(
    const ProtocolMessage& message,
    std::uint64_t nowMs,
    std::uint32_t targetNodeId,
    bool controlPlane
) {
    std::vector<std::uint8_t> encoded = FrameCodec::encode(message);

    Datagram response{};
    response.fromNodeId = nodeId_;
    response.toNodeId = targetNodeId;
    response.enqueueTimeMs = nowMs;
    response.controlPlane = controlPlane;
    response.bytes = encoded;

    metrics_.packetsSent += 1;
    metrics_.bytesSent += response.bytes.size();

    outgoing_.push_back(response);
}

void AccessNode::rememberUeRoute(std::uint32_t ueId, std::uint32_t nodeId) {
    ueNodeByUeId_[ueId] = nodeId;
}

}  // namespace miniran