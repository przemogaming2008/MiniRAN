#include "miniran/core/core_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(duplicate_attach_request){
    CoreNetwork core;

    ProtocolMessage request{};
    request.header.messageType = MessageType::AttachRequest;
    request.header.ueId = 7;
    request.header.sessionId = 0;
    request.header.sequenceNumber = 1;
    request.header.timestampMs = 100;

    auto response1 = core.handleAttachRequest(request, 100);
    ASSERT_TRUE(response1.has_value());
    ASSERT_EQ(response1->header.messageType, MessageType::AttachAccept);

    auto firstSessionId = response1->header.sessionId;
    ASSERT_NE(firstSessionId, 0U);
    ASSERT_EQ(core.activeSessionCount(), 1U);

    //duplicate attach for the same UE
    request.header.sequenceNumber = 2;
    request.header.timestampMs = 120;

    auto response2 = core.handleAttachRequest(request, 120);
    ASSERT_TRUE(response2.has_value());
    ASSERT_EQ(response2->header.messageType, MessageType::AttachAccept);

    auto secondSessionId = response2->header.sessionId;

    // policy: reuse existing session instead of creating a new one
    ASSERT_EQ(firstSessionId, secondSessionId);
    ASSERT_EQ(core.activeSessionCount(), 1U);
}