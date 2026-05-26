#pragma once

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

#include "miniran/common/metrics.h"
#include "miniran/core/core_network.h"
#include "miniran/transport/datagram.h"

namespace miniran {

class AccessNode {
public:
    AccessNode(std::uint32_t nodeId, CoreNetwork coreNetwork);

    std::uint32_t nodeId() const;
    const FlowMetrics& metrics() const;
    const CoreNetwork& coreNetwork() const;
    CoreNetwork& coreNetwork();

    void tick(std::uint64_t nowMs);
    void onDatagram(const Datagram& datagram, std::uint64_t nowMs);

    bool queueDownlinkToUe(const ProtocolMessage& message, std::uint64_t nowMs);

    std::vector<Datagram> flushOutgoing();

private:
    void queueResponseToUe(
        const ProtocolMessage& message,
        std::uint64_t nowMs,
        std::uint32_t targetNodeId
    );

    void queueDatagramToNode(
        const ProtocolMessage& message,
        std::uint64_t nowMs,
        std::uint32_t targetNodeId,
        bool controlPlane
    );

    void rememberUeRoute(std::uint32_t ueId, std::uint32_t nodeId);

    std::uint32_t nodeId_ = 0;

    CoreNetwork coreNetwork_;
    FlowMetrics metrics_{};

    std::unordered_map<std::uint32_t, std::uint32_t> ueNodeByUeId_;
    std::deque<Datagram> outgoing_;
};

}  // namespace miniran
