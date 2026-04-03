#include "miniran/protocol/frame_codec.h"

#include <sstream>

#include "miniran/common/byte_buffer.h"

namespace miniran {

namespace {
constexpr std::uint8_t kProtocolVersion = 1;
constexpr std::uint8_t kHeaderLength = 28;
}  // namespace

std::string toString(MessageType type) {
    switch (type) {
        case MessageType::AttachRequest:
            return "AttachRequest";
        case MessageType::AttachAccept:
            return "AttachAccept";
        case MessageType::AttachReject:
            return "AttachReject";
        case MessageType::Heartbeat:
            return "Heartbeat";
        case MessageType::HeartbeatAck:
            return "HeartbeatAck";
        case MessageType::Data:
            return "Data";
        case MessageType::DetachRequest:
            return "DetachRequest";
        case MessageType::DetachAccept:
            return "DetachAccept";
        case MessageType::Error:
            return "Error";
    }
    return "Unknown";
}

std::string toString(SessionState state) {
    switch (state) {
        case SessionState::Idle:
            return "Idle";
        case SessionState::Attaching:
            return "Attaching";
        case SessionState::Attached:
            return "Attached";
        case SessionState::Detaching:
            return "Detaching";
        case SessionState::Released:
            return "Released";
        case SessionState::Rejected:
            return "Rejected";
    }
    return "Unknown";
}

ProtocolMessage makeMessage(MessageType type,
                            std::uint32_t ueId,
                            std::uint32_t sessionId,
                            std::uint32_t sequenceNumber,
                            std::uint64_t timestampMs,
                            const std::vector<std::uint8_t>& payload) {
    ProtocolMessage message;
    message.header.version = kProtocolVersion;
    message.header.messageType = type;
    message.header.flags = 0;
    message.header.headerLength = kHeaderLength;
    message.header.sessionId = sessionId;
    message.header.ueId = ueId;
    message.header.sequenceNumber = sequenceNumber;
    message.header.timestampMs = timestampMs;
    message.header.payloadLength = static_cast<std::uint32_t>(payload.size());
    message.payload = payload;
    return message;
}

std::vector<std::uint8_t> FrameCodec::encode(const ProtocolMessage& message) {
    ByteBufferWriter writer;
    writer.writeU8(message.header.version);
    writer.writeU8(static_cast<std::uint8_t>(message.header.messageType));
    writer.writeU8(message.header.flags);
    writer.writeU8(message.header.headerLength);
    writer.writeU32(message.header.sessionId);
    writer.writeU32(message.header.ueId);
    writer.writeU32(message.header.sequenceNumber);
    writer.writeU64(message.header.timestampMs);
    writer.writeU32(static_cast<std::uint32_t>(message.payload.size()));
    writer.writeBytes(message.payload);
    return writer.bytes();
}

std::optional<ProtocolMessage> FrameCodec::decode(const std::vector<std::uint8_t>& bytes, std::string& error) {
    ByteBufferReader reader(bytes);
    ProtocolMessage message;
    std::uint8_t messageType = 0;

    if (!reader.readU8(message.header.version) || !reader.readU8(messageType) || !reader.readU8(message.header.flags) ||
        !reader.readU8(message.header.headerLength) || !reader.readU32(message.header.sessionId) ||
        !reader.readU32(message.header.ueId) || !reader.readU32(message.header.sequenceNumber) ||
        !reader.readU64(message.header.timestampMs) || !reader.readU32(message.header.payloadLength)) {
        error = "Frame is shorter than the mandatory header.";
        return std::nullopt;
    }

    if (message.header.version != kProtocolVersion) {
        std::ostringstream stream;
        stream << "Unsupported protocol version: " << static_cast<int>(message.header.version);
        error = stream.str();
        return std::nullopt;
    }

    if (message.header.headerLength != kHeaderLength) {
        std::ostringstream stream;
        stream << "Unsupported header length: " << static_cast<int>(message.header.headerLength);
        error = stream.str();
        return std::nullopt;
    }

    message.header.messageType = static_cast<MessageType>(messageType);

    if (!reader.readBytes(message.header.payloadLength, message.payload)) {
        error = "Payload length does not match frame size.";
        return std::nullopt;
    }

    if (reader.hasRemaining()) {
        error = "Frame has trailing bytes after payload.";
        return std::nullopt;
    }

    error.clear();
    return message;
}

}  // namespace miniran
