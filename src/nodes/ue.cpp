#include "miniran/nodes/ue.h"

#include <string>

#include "miniran/protocol/frame_codec.h"

namespace miniran {

Ue::Ue(std::uint32_t nodeId, std::uint32_t accessNodeId, TransportMode transportMode, SessionTimers timers)
    : nodeId_(nodeId), accessNodeId_(accessNodeId), transportMode_(transportMode), sessionManager_(nodeId, timers) {}

std::uint32_t Ue::nodeId() const {
    return nodeId_;
}

SessionState Ue::state() const {
    return sessionManager_.state();
}

bool Ue::isAttached() const {
    return sessionManager_.isAttached();
}

const FlowMetrics& Ue::metrics() const {
    return metrics_;
}

void Ue::startAttach(std::uint64_t nowMs) {
    //(void)nowMs;
    // TODO(student):
    // 1. Ask SessionManager if attach may start.
    if(sessionManager_.beginAttach(nowMs) == true){
        // 2. Create AttachRequest message.
        ProtocolMessage msg = makeMessage(
            MessageType::AttachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );
        // 3. Encode it using FrameCodec.
        std::vector<std::uint8_t> encoded_msg = FrameCodec::encode(msg); 
        // 4. Push a control-plane datagram to outgoing_.
        Datagram datagram = Datagram{};
        
        datagram.fromNodeId = nodeId_;
        datagram.toNodeId = accessNodeId_ ;
        datagram.enqueueTimeMs = nowMs;
        datagram.controlPlane = true;
        datagram.bytes = encoded_msg;

        outgoing_.push_back(datagram);
    }

}

void Ue::startDetach(std::uint64_t nowMs) {
    //(void)nowMs;
    // TODO(student):
    // Build and queue DetachRequest when session is active.
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
    std::vector<std::uint8_t> encoded_msg = FrameCodec::encode(msg);

    Datagram datagram = Datagram{};
        
        datagram.fromNodeId = nodeId_;
        datagram.toNodeId = accessNodeId_ ;
        datagram.enqueueTimeMs = nowMs;
        datagram.controlPlane = true;
        datagram.bytes = encoded_msg;

        outgoing_.push_back(datagram);
}

void Ue::sendTraffic(const std::vector<std::uint8_t>& payload, std::uint64_t nowMs) {
    (void)payload;
    (void)nowMs;
    // TODO(student):
    // 1. Check if data can be sent.
    // 2. Wrap payload into a Data message.
    // 3. Update UE metrics_.
    // 4. Push a user-plane datagram to outgoing_.
}

void Ue::tick(std::uint64_t nowMs) {
    (void)nowMs;
    // TODO(student):
    // React to SessionManager::onTick().
    // Possible actions: retransmit AttachRequest / DetachRequest / send Heartbeat.
}

void Ue::onDatagram(const Datagram& datagram, std::uint64_t nowMs) {
    // (void)datagram;
    // (void)nowMs;
    // TODO(student):
    // 1. Decode incoming bytes.
    std::string error;
    std::optional<ProtocolMessage> protocolMessage_opt= FrameCodec::decode(datagram.bytes, error);
    // 2. Handle AttachAccept / DetachAccept / HeartbeatAck / Error.
    if(protocolMessage_opt){
        ProtocolMessage protocolMessage = *protocolMessage_opt;

        if(protocolMessage.header.messageType == MessageType::AttachAccept){
            // 3. Update session state via SessionManager. (inside SessionManager methods)
            sessionManager_.onAttachAccepted(protocolMessage.header.sessionId,nowMs);
        } else if (protocolMessage.header.messageType == MessageType::DetachAccept){
            sessionManager_.onDetachAccepted(nowMs);
        } else if (protocolMessage.header.messageType == MessageType::HeartbeatAck){
            sessionManager_.onHeartbeatResponse(nowMs);
        } else if (protocolMessage.header.messageType == MessageType::Error){
            //error message type, to do?
        } else {
            //inapropriate messagetype
        }

    } else {
        // opt, error
    }

    
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
