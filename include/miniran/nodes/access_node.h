#pragma once

#include <cstdint>
#include <deque>
#include <vector>

#include "miniran/common/metrics.h"
#include "miniran/core/core_network.h"
#include "miniran/transport/datagram.h"

namespace miniran {

class AccessNode {
public:
    AccessNode(std::uint32_t nodeId, std::uint32_t ueNodeId, CoreNetwork coreNetwork);

    std::uint32_t nodeId() const;
    const FlowMetrics& metrics() const;
    const CoreNetwork& coreNetwork() const;
    CoreNetwork& coreNetwork();

    void tick(std::uint64_t nowMs);
    void onDatagram(const Datagram& datagram, std::uint64_t nowMs);
    std::vector<Datagram> flushOutgoing();

private:
    std::uint32_t nodeId_ = 0;
    std::uint32_t ueNodeId_ = 0;
    CoreNetwork coreNetwork_;
    FlowMetrics metrics_{};
    std::deque<Datagram> outgoing_;
};

}  // namespace miniran
