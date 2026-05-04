#include "miniran/core/core_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(detach_with_wrong_session_id){
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

    ProtocolMessage detach{};
    detach.header.messageType = MessageType::DetachRequest;
    detach.header.ueId = 7;
    detach.header.sessionId = correctSessionId + 1;
    detach.header.sequenceNumber = 2;
    detach.header.timestampMs = 120;

    auto detachResponse = core.handleDetachRequest(detach, 120);
    ASSERT_TRUE(detachResponse.has_value());
    ASSERT_EQ(detachResponse->header.messageType, MessageType::Error);

    ASSERT_EQ(core.activeSessionCount(), 1U);
}