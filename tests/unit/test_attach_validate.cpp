#include "miniran/core/core_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(attach_rejects_zero_ue_id){
    CoreNetwork core;

    ProtocolMessage attach{};
    attach.header.messageType = MessageType::AttachRequest;
    attach.header.ueId = 0;
    attach.header.sequenceNumber = 1;
    attach.header.sessionId = 0;
    attach.header.timestampMs = 100;

    auto response = core.handleAttachRequest(attach, 100);

    ASSERT_TRUE(response.has_value());
    ASSERT_EQ(response->header.messageType, MessageType::Error);
    ASSERT_EQ(core.activeSessionCount(), 0U);
}
TEST_CASE(attach_rejects_zero_sequence_number){
    CoreNetwork core;

    ProtocolMessage attach{};
    attach.header.messageType = MessageType::AttachRequest;
    attach.header.ueId = 7;
    attach.header.sequenceNumber = 0;
    attach.header.sessionId = 0;
    attach.header.timestampMs = 100;

    auto response = core.handleAttachRequest(attach, 100);

    ASSERT_TRUE(response.has_value());
    ASSERT_EQ(response->header.messageType, MessageType::Error);
    ASSERT_EQ(core.activeSessionCount(), 0U);
}
TEST_CASE(attach_rejects_nonzero_session_id){
    CoreNetwork core;

    ProtocolMessage attach{};
    attach.header.messageType = MessageType::AttachRequest;
    attach.header.ueId = 7;
    attach.header.sequenceNumber = 1;
    attach.header.sessionId = 1234;
    attach.header.timestampMs = 100;

    auto response = core.handleAttachRequest(attach, 100);

    ASSERT_TRUE(response.has_value());
    ASSERT_EQ(response->header.messageType, MessageType::Error);
    ASSERT_EQ(core.activeSessionCount(), 0U);
}
TEST_CASE(duplicate_attach_returns_existing_session_id){
    CoreNetwork core;

    ProtocolMessage attach{};
    attach.header.messageType = MessageType::AttachRequest;
    attach.header.ueId = 7;
    attach.header.sequenceNumber = 1;
    attach.header.sessionId = 0;
    attach.header.timestampMs = 100;

    auto firstResponse = core.handleAttachRequest(attach, 100);
    ASSERT_TRUE(firstResponse.has_value());
    ASSERT_EQ(firstResponse->header.messageType, MessageType::AttachAccept);

    std::uint32_t firstSessionId = firstResponse->header.sessionId;

    attach.header.sequenceNumber = 2;
    attach.header.timestampMs = 150;

    auto secondResponse = core.handleAttachRequest(attach, 150);
    ASSERT_TRUE(secondResponse.has_value());
    ASSERT_EQ(secondResponse->header.messageType, MessageType::AttachAccept);
    ASSERT_EQ(secondResponse->header.sessionId, firstSessionId);

    ASSERT_EQ(core.activeSessionCount(), 1U);
}