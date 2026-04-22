#include "miniran/core/core_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(data_after_detach){
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
    data.header.payloadLength = 50;
    data.header.timestampMs = 110;

    core.handleData(data, 110);
    ASSERT_EQ(core.deliveredPackets(), 1U);
    ASSERT_EQ(core.deliveredBytes(), 50U);

    ProtocolMessage detach{};
    detach.header.messageType = MessageType::DetachRequest;
    detach.header.ueId = 7;
    detach.header.sessionId = sessionId;
    detach.header.sequenceNumber = 3;
    detach.header.timestampMs = 120;

    auto detachResponse = core.handleDetachRequest(detach, 120);
    ASSERT_TRUE(detachResponse.has_value());
    ASSERT_EQ(detachResponse->header.messageType, MessageType::DetachAccept);

    data.header.sequenceNumber = 4;
    data.header.timestampMs = 130;
    core.handleData(data, 130);

    ASSERT_EQ(core.deliveredPackets(), 1U);
    ASSERT_EQ(core.deliveredBytes(), 50U);
}