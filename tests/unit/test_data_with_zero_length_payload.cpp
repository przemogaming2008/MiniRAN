#include "miniran/core/core_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(data_with_zero_length_payload_is_rejected){
    CoreNetwork core;

    ProtocolMessage attach{};
    attach.header.messageType = MessageType::AttachRequest;
    attach.header.ueId = 7;
    attach.header.sequenceNumber = 1;
    attach.header.timestampMs = 100;

    auto attachResponse = core.handleAttachRequest(attach, 100);
    ASSERT_TRUE(attachResponse.has_value());
    ASSERT_EQ(attachResponse->header.messageType, MessageType::AttachAccept);

    std::uint32_t sessionId = attachResponse->header.sessionId;

    ProtocolMessage data{};
    data.header.messageType = MessageType::Data;
    data.header.ueId = 7;
    data.header.sessionId = sessionId;
    data.header.sequenceNumber = 2;
    data.header.payloadLength = 0;
    data.header.timestampMs = 110;
    data.payload = {};

    core.handleData(data, 110);


    ASSERT_EQ(core.deliveredPackets(), 0U);
    ASSERT_EQ(core.deliveredBytes(), 0U);
}