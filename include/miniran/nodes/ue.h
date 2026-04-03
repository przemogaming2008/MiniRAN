#pragma once

#include <cstdint>
#include <deque>
#include <vector>

#include "miniran/common/metrics.h"
#include "miniran/protocol/session_manager.h"
#include "miniran/transport/datagram.h"
#include "miniran/transport/transport_mode.h"

namespace miniran {

class Ue {
public:
    Ue(std::uint32_t nodeId, std::uint32_t accessNodeId, TransportMode transportMode, SessionTimers timers = {});

    std::uint32_t nodeId() const;
    SessionState state() const;
    bool isAttached() const;
    const FlowMetrics& metrics() const;

    void startAttach(std::uint64_t nowMs);
    void startDetach(std::uint64_t nowMs);
    void sendTraffic(const std::vector<std::uint8_t>& payload, std::uint64_t nowMs);
    void tick(std::uint64_t nowMs);
    void onDatagram(const Datagram& datagram, std::uint64_t nowMs);
    std::vector<Datagram> flushOutgoing();

private:
    std::uint32_t nodeId_ = 0;
    std::uint32_t accessNodeId_ = 0;
    TransportMode transportMode_ = TransportMode::Tcp;
    SessionManager sessionManager_;
    FlowMetrics metrics_{};
    std::deque<Datagram> outgoing_;
};

}  // namespace miniran
