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
    // (void)datagram;
    // (void)nowMs;
    // TODO(student):
    // 1. Decode datagram bytes using FrameCodec.
    std::string error;
    std::optional<ProtocolMessage> decode_opt = FrameCodec::decode(datagram.bytes,error);

    if(decode_opt){
        ProtocolMessage protocolMessage = *decode_opt;
        // 2. Route messages by type:
        //    - AttachRequest  -> coreNetwork_.handleAttachRequest()
        //    - Heartbeat      -> coreNetwork_.handleHeartbeat()
        //    - Data           -> coreNetwork_.handleData()
        //    - DetachRequest  -> coreNetwork_.handleDetachRequest()
        if(protocolMessage.header.messageType == MessageType::AttachRequest){
            // 4. Update AccessNode metrics when appropriate.
            metrics_.packetsDelivered += 1;
            metrics_.bytesDelivered += datagram.bytes.size();
            std::optional<ProtocolMessage> msg_opt = coreNetwork_.handleAttachRequest(protocolMessage,nowMs);
            if(msg_opt){
                // 3. For responses produced by CoreNetwork, encode them and queue a reply datagram to UE.
                ProtocolMessage msg = *msg_opt;
                std::vector<std::uint8_t> encode_msg = FrameCodec::encode(msg);
                Datagram datagram = Datagram{};
                datagram.fromNodeId = nodeId_;
                datagram.toNodeId = ueNodeId_;
                datagram.enqueueTimeMs = nowMs;
                datagram.controlPlane = true;
                datagram.bytes = encode_msg;

                metrics_.packetsSent += 1;
                metrics_.bytesSent += datagram.bytes.size();
                outgoing_.push_back(datagram);
                return;
            }else{
                //no answer from core
                return;
            }
        }else if(protocolMessage.header.messageType == MessageType::Heartbeat){
            metrics_.packetsDelivered += 1;
            metrics_.bytesDelivered += datagram.bytes.size();
            std::optional<ProtocolMessage> msg_opt = coreNetwork_.handleHeartbeat(protocolMessage,nowMs);
            if(msg_opt){
                // 3. For responses produced by CoreNetwork, encode them and queue a reply datagram to UE.
                ProtocolMessage msg = *msg_opt;
                std::vector<std::uint8_t> encode_msg = FrameCodec::encode(msg);
                Datagram datagram = Datagram{};
                datagram.fromNodeId = nodeId_;
                datagram.toNodeId = ueNodeId_;
                datagram.enqueueTimeMs = nowMs;
                datagram.controlPlane = true;
                datagram.bytes = encode_msg;

                metrics_.packetsSent += 1;
                metrics_.bytesSent += datagram.bytes.size();
                outgoing_.push_back(datagram);
                return;
            }else{
                //no answer from core
                return;
            }
        }else if(protocolMessage.header.messageType == MessageType::Data){
            metrics_.packetsDelivered += 1;
            metrics_.bytesDelivered += datagram.bytes.size();
            coreNetwork_.handleData(protocolMessage,nowMs);
            return;

        }else if(protocolMessage.header.messageType == MessageType::DetachRequest){
            metrics_.packetsDelivered += 1;
            metrics_.bytesDelivered += datagram.bytes.size();
            std::optional<ProtocolMessage> msg_opt = coreNetwork_.handleDetachRequest(protocolMessage,nowMs);
            if(msg_opt){
                // 3. For responses produced by CoreNetwork, encode them and queue a reply datagram to UE.
                ProtocolMessage msg = *msg_opt;
                std::vector<std::uint8_t> encode_msg = FrameCodec::encode(msg);
                Datagram datagram = Datagram{};
                datagram.fromNodeId = nodeId_;
                datagram.toNodeId = ueNodeId_;
                datagram.enqueueTimeMs = nowMs;
                datagram.controlPlane = true;
                datagram.bytes = encode_msg;

                metrics_.packetsSent += 1;
                metrics_.bytesSent += datagram.bytes.size();    
                outgoing_.push_back(datagram);
                return;
            }else{
                //no answer from core
                return;
            }
        }else{
            metrics_.packetsDropped += 1;
            //inappropriate header
            //Unsupported message type: send Error response
            ProtocolMessage msg = ProtocolMessage{};
            msg.header.messageType = MessageType::Error;
            msg.header.ueId = protocolMessage.header.ueId;
            msg.header.sessionId = protocolMessage.header.sessionId;
            msg.header.sequenceNumber = protocolMessage.header.sequenceNumber;
            msg.header.timestampMs = nowMs;

            std::vector<std::uint8_t> encoded = FrameCodec::encode(msg);

            Datagram response{};
            response.fromNodeId = nodeId_;
            response.toNodeId = ueNodeId_;
            response.enqueueTimeMs = nowMs;
            response.controlPlane = true;
            response.bytes = encoded;

            metrics_.packetsSent += 1;
            metrics_.bytesSent += response.bytes.size();

            outgoing_.push_back(response);
            return;

        }
        
        

    } else {
        metrics_.packetsDropped += 1;
        //message issue
        //Malformed frame: drop silently
        return;
    }
    
    return;
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

}  // namespace miniran
