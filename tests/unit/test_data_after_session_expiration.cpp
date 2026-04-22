#include "miniran/core/core_network.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(data_after_session_expiration){
    SessionTimers timers{};
    timers.inactivityTimeoutMs = 100;

    CoreNetwork core(timers);

    ProtocolMessage attach{};
    attach.header.messageType = MessageType::AttachRequest;
    attach.header.ueId = 7;
    attach.header.sequenceNumber = 1;
    attach.header.timestampMs = 100;

    auto attachResp = core.handleAttachRequest(attach, 100);
    ASSERT_TRUE(attachResp.has_value());

    std::uint32_t sessionId = attachResp->header.sessionId;

    core.expireInactiveSessions(250);
    ASSERT_EQ(core.activeSessionCount(), 0U);


    ProtocolMessage data{};
    data.header.messageType = MessageType::Data;
    data.header.ueId = 7;
    data.header.sessionId = sessionId;
    data.header.sequenceNumber = 2;
    data.header.payloadLength = 50;
    data.header.timestampMs = 260;

    core.handleData(data, 260);

    ASSERT_EQ(core.deliveredPackets(), 0U);
    ASSERT_EQ(core.deliveredBytes(), 0U);
}