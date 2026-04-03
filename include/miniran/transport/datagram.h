#pragma once

#include <cstdint>
#include <vector>

namespace miniran {

struct Datagram {
    std::uint32_t fromNodeId = 0;
    std::uint32_t toNodeId = 0;
    std::uint64_t enqueueTimeMs = 0;
    std::uint64_t deliverAtMs = 0;
    bool controlPlane = false;
    std::uint64_t serialNumber = 0;
    std::vector<std::uint8_t> bytes;
};

}  // namespace miniran
