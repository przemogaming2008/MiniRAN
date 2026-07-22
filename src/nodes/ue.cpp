#include "miniran/nodes/ue.h"

#include <string>

#include "miniran/protocol/frame_codec.h"

namespace miniran {

Ue::Ue(std::uint32_t nodeId, std::uint32_t accessNodeId, TransportMode transportMode, SessionTimers timers)
    : nodeId_(nodeId), accessNodeId_(accessNodeId), sessionManager_(nodeId, timers) {
    (void)transportMode;
}

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

    //Ask SessionManager if attach may start.
    if(sessionManager_.beginAttach(nowMs) == true){
        //Create AttachRequest message.
        ProtocolMessage msg = makeMessage(
            MessageType::AttachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );
        //Encode it using FrameCodec.
        std::vector<std::uint8_t> encoded_msg = FrameCodec::encode(msg); 
        //Push a control-plane datagram to outgoing_.
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

    //Build and queue DetachRequest when session is active.
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

    //Check if data can be sent.
    if(!sessionManager_.canSendData()){
        return;
    }
    //Wrap payload into a Data message.
    ProtocolMessage msg = makeMessage(
        MessageType::Data,
        sessionManager_.ueId(),
        sessionManager_.sessionId(),
        sessionManager_.nextSequenceNumber(),
        nowMs,
        payload
    );
    //Update UE metrics_.
    metrics_.bytesSent += payload.size();
    metrics_.packetsSent += 1;
    //Push a user-plane datagram to outgoing_.
    Datagram datagram{};  //datagram.controlPlane = false;
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    
    datagram.bytes = FrameCodec::encode(msg);

    outgoing_.push_back(datagram);
}

void Ue::tick(std::uint64_t nowMs) {

    //React to SessionManager::onTick().
    //Possible actions: retransmit AttachRequest / DetachRequest / send Heartbeat.
    RetryDecision decision = sessionManager_.onTick(nowMs);

    if (!decision.shouldRetransmit) {
        return;
    }

    std::vector<std::uint8_t> encoded_msg;
    if (decision.messageType == MessageType::AttachRequest) {
        ProtocolMessage msg = makeMessage(
            MessageType::AttachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );
        encoded_msg = FrameCodec::encode(msg); 
    }

    else if (decision.messageType == MessageType::DetachRequest) {
        ProtocolMessage msg = makeMessage(
            MessageType::DetachRequest,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );
        encoded_msg = FrameCodec::encode(msg); 
    }

    else if (decision.messageType == MessageType::Heartbeat) {
        ProtocolMessage msg = makeMessage(
            MessageType::Heartbeat,
            sessionManager_.ueId(),
            sessionManager_.sessionId(),
            sessionManager_.nextSequenceNumber(),
            nowMs
        );
        encoded_msg = FrameCodec::encode(msg); 
    }
    else {
        return;
    } 
    Datagram datagram = Datagram{};
    
    datagram.fromNodeId = nodeId_;
    datagram.toNodeId = accessNodeId_ ;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = true;
    datagram.bytes = encoded_msg;

    metrics_.packetsSent += 1;
    metrics_.bytesSent += datagram.bytes.size();

    outgoing_.push_back(datagram);
}

void Ue::onDatagram(const Datagram& datagram, std::uint64_t nowMs) {

    //Decode incoming bytes.
    std::string error;
    std::optional<ProtocolMessage> protocolMessage_opt= FrameCodec::decode(datagram.bytes, error);
    //Handle AttachAccept / DetachAccept / HeartbeatAck / Error.
    if(protocolMessage_opt){
        ProtocolMessage protocolMessage = *protocolMessage_opt;

        if (protocolMessage.header.ueId != nodeId()) {
            return;
        }

        if (protocolMessage.header.messageType == MessageType::AttachAccept) {
            if (protocolMessage.header.sessionId == 0) {
                return;
            }

            sessionManager_.onAttachAccepted(protocolMessage.header.sessionId, nowMs);
        } else if (protocolMessage.header.messageType == MessageType::DetachAccept) {
            if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
                return;
            }
            sessionManager_.onDetachAccepted(nowMs);
        } else if (protocolMessage.header.messageType == MessageType::HeartbeatAck){
            if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
                return;
            }
            sessionManager_.onHeartbeatResponse(nowMs);
        } else if (protocolMessage.header.messageType == MessageType::Error){
            //error message type, to do?
        } else {
            //inapropriate messagetype
        }

    } else {
        //opt, error
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
