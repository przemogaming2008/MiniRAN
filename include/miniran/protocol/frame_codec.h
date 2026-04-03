#pragma once

#include <optional>
#include <string>
#include <vector>

#include "miniran/protocol/protocol_message.h"

namespace miniran {

class FrameCodec {
public:
    static std::vector<std::uint8_t> encode(const ProtocolMessage& message);
    static std::optional<ProtocolMessage> decode(const std::vector<std::uint8_t>& bytes, std::string& error);
};

}  // namespace miniran
