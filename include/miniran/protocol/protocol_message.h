#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace miniran {

enum class MessageType : std::uint8_t {
    AttachRequest = 1,
    AttachAccept = 2,
    AttachReject = 3,
    Heartbeat = 4,
    HeartbeatAck = 5,
    Data = 6,
    DetachRequest = 7,
    DetachAccept = 8,
    Error = 9,
};

enum class SessionState : std::uint8_t {
    Idle = 0,
    Attaching = 1,
    Attached = 2,
    Detaching = 3,
    Released = 4,
    Rejected = 5,
};

struct ProtocolHeader {
    std::uint8_t version = 1;
    MessageType messageType = MessageType::Error;
    std::uint8_t flags = 0;
    std::uint8_t headerLength = 28;
    std::uint32_t sessionId = 0;
    std::uint32_t ueId = 0;
    std::uint32_t sequenceNumber = 0;
    std::uint64_t timestampMs = 0;
    std::uint32_t payloadLength = 0;
};

struct ProtocolMessage {
    ProtocolHeader header;
    std::vector<std::uint8_t> payload;
};

std::string toString(MessageType type);
std::string toString(SessionState state);

ProtocolMessage makeMessage(MessageType type,
                            std::uint32_t ueId,
                            std::uint32_t sessionId,
                            std::uint32_t sequenceNumber,
                            std::uint64_t timestampMs,
                            const std::vector<std::uint8_t>& payload = {});

}  // namespace miniran
