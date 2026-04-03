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
    (void)datagram;
    (void)nowMs;
    // TODO(student):
    // 1. Decode datagram bytes using FrameCodec.
    // 2. Route messages by type:
    //    - AttachRequest  -> coreNetwork_.handleAttachRequest()
    //    - Heartbeat      -> coreNetwork_.handleHeartbeat()
    //    - Data           -> coreNetwork_.handleData()
    //    - DetachRequest  -> coreNetwork_.handleDetachRequest()
    // 3. For responses produced by CoreNetwork, encode them and queue a reply datagram to UE.
    // 4. Update AccessNode metrics when appropriate.
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
