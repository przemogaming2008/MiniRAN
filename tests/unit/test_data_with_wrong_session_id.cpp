#include "miniran/core/core_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(data_with_wrong_session_id){
    CoreNetwork core;

    ProtocolMessage attach{};
    attach.header.messageType = MessageType::AttachRequest;
    attach.header.ueId = 7;
    attach.header.sequenceNumber = 1;
    attach.header.timestampMs = 100;

    auto attachResponse = core.handleAttachRequest(attach, 100);
    ASSERT_TRUE(attachResponse.has_value());
    ASSERT_EQ(attachResponse->header.messageType, MessageType::AttachAccept);

    std::uint32_t correctSessionId = attachResponse->header.sessionId;

    ProtocolMessage data{};
    data.header.messageType = MessageType::Data;
    data.header.ueId = 7;
    data.header.sessionId = correctSessionId;
    data.header.sequenceNumber = 2;
    data.header.payloadLength = 50;
    data.header.timestampMs = 110;

    core.handleData(data, 110);

    ASSERT_EQ(core.deliveredPackets(), 1U);
    ASSERT_EQ(core.deliveredBytes(), 50U);

    ProtocolMessage badData = data;
    badData.header.sessionId = correctSessionId + 1;

    badData.header.sequenceNumber = 3;
    badData.header.timestampMs = 120;

    core.handleData(badData, 120);

    ASSERT_EQ(core.deliveredPackets(), 1U);
    ASSERT_EQ(core.deliveredBytes(), 50U);
}