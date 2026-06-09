#include "miniran/core/core_network.h"
#include "miniran/nodes/access_node.h"
#include "miniran/nodes/ue.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(component_reattach_after_clean_detach) {
    SessionTimers timers;
    timers.attachTimeoutMs = 150;
    timers.detachTimeoutMs = 150;
    timers.heartbeatIntervalMs = 200;
    timers.inactivityTimeoutMs = 700;
    timers.maxAttachRetries = 3;
    timers.maxDetachRetries = 2;

    Ue ue(
        7,
        7,
        1000,
        TransportMode::Tcp,
        timers
    );

    AccessNode accessNode(
        1000,
        CoreNetwork(timers)
    );

    ue.startAttach(0);

    auto ueAttachDatagrams = ue.flushOutgoing();
    ASSERT_EQ(ueAttachDatagrams.size(), 1U);

    accessNode.onDatagram(ueAttachDatagrams[0], 10);

    auto attachResponses = accessNode.flushOutgoing();
    ASSERT_EQ(attachResponses.size(), 1U);

    ue.onDatagram(attachResponses[0], 20);

    ASSERT_TRUE(ue.isAttached());

    const auto firstSessionId = ue.sessionId();

    ASSERT_TRUE(firstSessionId != 0U);

    ue.startDetach(100);

    auto ueDetachDatagrams = ue.flushOutgoing();
    ASSERT_EQ(ueDetachDatagrams.size(), 1U);

    accessNode.onDatagram(ueDetachDatagrams[0], 110);

    auto detachResponses = accessNode.flushOutgoing();
    ASSERT_EQ(detachResponses.size(), 1U);

    ue.onDatagram(detachResponses[0], 120);

    ASSERT_TRUE(ue.detachConfirmed());
    ASSERT_EQ(ue.sessionId(), 0U);
    ASSERT_EQ(ue.lastSessionId(), firstSessionId);
    ASSERT_EQ(accessNode.coreNetwork().activeSessionCount(), 0U);

    ue.startAttach(200);

    auto secondAttachDatagrams = ue.flushOutgoing();
    ASSERT_EQ(secondAttachDatagrams.size(), 1U);

    accessNode.onDatagram(secondAttachDatagrams[0], 210);

    auto secondAttachResponses = accessNode.flushOutgoing();
    ASSERT_EQ(secondAttachResponses.size(), 1U);

    ue.onDatagram(secondAttachResponses[0], 220);

    ASSERT_TRUE(ue.isAttached());

    const auto secondSessionId = ue.sessionId();

    ASSERT_TRUE(secondSessionId != 0U);
    ASSERT_TRUE(secondSessionId != firstSessionId);
    ASSERT_EQ(accessNode.coreNetwork().activeSessionCount(), 1U);
}