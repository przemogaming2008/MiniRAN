#include "miniran/core/core_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(detach_after_session_expiration){
    SessionTimers timers{};
    timers.inactivityTimeoutMs = 100;

    CoreNetwork core(timers);

    ProtocolMessage attach{};
    attach.header.messageType = MessageType::AttachRequest;
    attach.header.ueId = 7;
    attach.header.sequenceNumber = 1;
    attach.header.timestampMs = 100;

    auto attachResponse = core.handleAttachRequest(attach, 100); 
    ASSERT_TRUE(attachResponse.has_value());
    ASSERT_EQ(attachResponse->header.messageType, MessageType::AttachAccept);

    std::uint32_t sessionId = attachResponse->header.sessionId;

    core.expireInactiveSessions(250);
    ASSERT_EQ(core.activeSessionCount(), 0U);

    ProtocolMessage detach{};
    detach.header.messageType = MessageType::DetachRequest;
    detach.header.ueId = 7;
    detach.header.sessionId = sessionId;
    detach.header.sequenceNumber = 2;
    detach.header.timestampMs = 260;
    //Idempotent detach: missing session => DetachAccept
    auto detachResponse = core.handleDetachRequest(detach, 260);
    ASSERT_TRUE(detachResponse.has_value());
    ASSERT_EQ(detachResponse->header.messageType, MessageType::DetachAccept);
}