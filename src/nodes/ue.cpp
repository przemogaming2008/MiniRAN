#include "miniran/nodes/ue.h"

#include <string>

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

SessionState Ue::state() const {
    return sessionManager_.state();
}

bool Ue::isAttached() const {
    return sessionManager_.isAttached();
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
    //Update uplink metrics.
    uplinkMetrics_.packetsSent += 1;
    uplinkMetrics_.bytesSent += payload.size();
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
        protocolMetrics_.heartbeatsSent += 1;
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
    datagram.toNodeId = accessNodeId_;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = true;
    datagram.bytes = encoded_msg;

    outgoing_.push_back(datagram);
}

void Ue::onDatagram(const Datagram& datagram, std::uint64_t nowMs) {

    //Decode incoming bytes.
    std::string error;
    std::optional<ProtocolMessage> protocolMessage_opt= FrameCodec::decode(datagram.bytes, error);
    //Handle AttachAccept / DetachAccept / HeartbeatAck / Error.
    if(protocolMessage_opt){
        ProtocolMessage protocolMessage = *protocolMessage_opt;

        if (protocolMessage.header.ueId != ueId()) {
            protocolMetrics_.invalidMessagesDropped += 1;
            return;
        }

        if(protocolMessage.header.messageType == MessageType::AttachAccept){
            //Update session state via SessionManager. (inside SessionManager methods)
            sessionManager_.onAttachAccepted(protocolMessage.header.sessionId,nowMs);
        } else if (protocolMessage.header.messageType == MessageType::DetachAccept){
            if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
                protocolMetrics_.invalidMessagesDropped += 1;
                return;
            }
            sessionManager_.onDetachAccepted(nowMs);
        } else if (protocolMessage.header.messageType == MessageType::HeartbeatAck){
            if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
                protocolMetrics_.invalidMessagesDropped += 1;
                return;
            }
            protocolMetrics_.heartbeatAcksReceived += 1;
            sessionManager_.onHeartbeatResponse(nowMs);
        } else if (protocolMessage.header.messageType == MessageType::Error){
            protocolMetrics_.protocolErrorsReceived += 1;

        } else if (protocolMessage.header.messageType == MessageType::Data) {
            if (protocolMessage.header.sessionId != sessionManager_.sessionId()) {
                protocolMetrics_.invalidMessagesDropped += 1;
                return;
            }

            if (!sessionManager_.isAttached()) {
                protocolMetrics_.invalidMessagesDropped += 1;
                return;
            }

            downlinkMetrics_.packetsDelivered += 1;
            downlinkMetrics_.bytesDelivered += protocolMessage.payload.size();

        } else {
            protocolMetrics_.invalidMessagesDropped += 1;
        }
    } else {
        protocolMetrics_.invalidMessagesDropped += 1;
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
