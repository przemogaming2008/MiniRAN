#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "miniran/protocol/frame_codec.h"
#include "miniran/protocol/protocol_message.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(frame_codec_rejects_unknown_message_type)
{
    ProtocolMessage message = makeMessage(
        MessageType::AttachRequest,
        1,
        0,
        1,
        100
    );

    std::vector<std::uint8_t> encoded = FrameCodec::encode(message);

    ASSERT_TRUE(encoded.size() >= 2);

    encoded[1] = 255;

    std::string error;
    const auto decoded = FrameCodec::decode(encoded, error);

    ASSERT_TRUE(!decoded.has_value());
    ASSERT_TRUE(error.find("message type") != std::string::npos ||
                error.find("Message type") != std::string::npos);
}