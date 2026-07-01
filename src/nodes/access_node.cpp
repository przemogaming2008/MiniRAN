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


    //Route messages by type:
    //    - AttachRequest  -> coreNetwork_.handleAttachRequest()
    //    - Heartbeat      -> coreNetwork_.handleHeartbeat()
    //    - Data           -> coreNetwork_.handleData()
    //    - DetachRequest  -> coreNetwork_.handleDetachRequest()
    if (protocolMessage.header.messageType == MessageType::AttachRequest) {
        auto response = coreNetwork_.handleAttachRequest(protocolMessage, nowMs);

        if (response && response->header.messageType == MessageType::AttachAccept) {
            ++metrics_.packetsDelivered;
            metrics_.bytesDelivered += datagram.bytes.size();

            rememberUeRoute(protocolMessage.header.ueId, datagram.fromNodeId);
        }

        if (response) {
            queueResponseToUe(*response, nowMs, datagram.fromNodeId);
        }

        return;
    }

    if (protocolMessage.header.messageType == MessageType::Heartbeat) {
        auto response = coreNetwork_.handleHeartbeat(protocolMessage, nowMs);

        if (response && response->header.messageType == MessageType::HeartbeatAck) {
            ++metrics_.packetsDelivered;
            metrics_.bytesDelivered += datagram.bytes.size();

            rememberUeRoute(protocolMessage.header.ueId, datagram.fromNodeId);
        }

        if (response) {
            queueResponseToUe(*response, nowMs, datagram.fromNodeId);
        }

        return;
    }

    if (protocolMessage.header.messageType == MessageType::Data) {
        const std::size_t packetsBefore =
            coreNetwork_.deliveredPacketsForUe(protocolMessage.header.ueId);

        if (coreNetwork_.hasActiveSession(
                protocolMessage.header.ueId,
                protocolMessage.header.sessionId
            )) {
            rememberUeRoute(protocolMessage.header.ueId, datagram.fromNodeId);
        }

        auto response = coreNetwork_.handleData(protocolMessage, nowMs);

        const std::size_t packetsAfter =
            coreNetwork_.deliveredPacketsForUe(protocolMessage.header.ueId);

        if (packetsAfter > packetsBefore) {
            ++metrics_.packetsDelivered;
            metrics_.bytesDelivered += datagram.bytes.size();
        }

        if (response) {
            queueResponseToUe(*response, nowMs, datagram.fromNodeId);
        }

        return;
    }

    if (protocolMessage.header.messageType == MessageType::DetachRequest) {
        auto response = coreNetwork_.handleDetachRequest(protocolMessage, nowMs);

        if (response && response->header.messageType == MessageType::DetachAccept) {
            ++metrics_.packetsDelivered;
            metrics_.bytesDelivered += datagram.bytes.size();
        }

        if (response) {
            queueResponseToUe(*response, nowMs, datagram.fromNodeId);
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
    if (message.header.messageType != MessageType::DownlinkData) {
        metrics_.packetsDropped += 1;
        return false;
    }

    if (message.header.ueId == 0 || message.header.sessionId == 0) {
        metrics_.packetsDropped += 1;
        return false;
    }

    if (!coreNetwork_.hasActiveSession(message.header.ueId, message.header.sessionId)) {
        metrics_.packetsDropped += 1;
        return false;
    }

    const auto route = ueNodeByUeId_.find(message.header.ueId);

    if (route == ueNodeByUeId_.end()) {
        metrics_.packetsDropped += 1;
        return false;
    }

    queueDatagramToNode(
        message,
        nowMs,
        route->second,
        false
    );

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