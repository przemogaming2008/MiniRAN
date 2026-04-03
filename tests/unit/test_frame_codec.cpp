#include <vector>

#include "miniran/protocol/frame_codec.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(frame_codec_round_trip_preserves_message_fields) {
    const ProtocolMessage original = makeMessage(
        MessageType::Data, 7, 1001, 55, 123456, std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6});

    const auto bytes = FrameCodec::encode(original);
    std::string error;
    const auto decoded = FrameCodec::decode(bytes, error);

    ASSERT_TRUE(decoded.has_value());
    ASSERT_EQ(decoded->header.version, 1);
    ASSERT_EQ(decoded->header.sessionId, 1001U);
    ASSERT_EQ(decoded->header.ueId, 7U);
    ASSERT_EQ(decoded->header.sequenceNumber, 55U);
    ASSERT_EQ(decoded->header.timestampMs, 123456U);
    ASSERT_EQ(decoded->header.payloadLength, 6U);
    ASSERT_EQ(decoded->payload.size(), 6U);
    ASSERT_EQ(decoded->payload[0], 1U);
    ASSERT_EQ(decoded->payload[5], 6U);
}

TEST_CASE(frame_codec_rejects_payload_length_mismatch) {
    const ProtocolMessage original = makeMessage(MessageType::Heartbeat, 7, 1001, 1, 20, std::vector<std::uint8_t>{9, 9, 9});
    auto bytes = FrameCodec::encode(original);
    bytes.pop_back();

    std::string error;
    const auto decoded = FrameCodec::decode(bytes, error);

    ASSERT_TRUE(!decoded.has_value());
    ASSERT_NE(error, std::string());
}
