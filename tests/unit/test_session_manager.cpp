#include "miniran/protocol/session_manager.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(session_manager_moves_from_idle_to_attaching) {
    SessionManager manager(7);

    ASSERT_EQ(manager.state(), SessionState::Idle);
    ASSERT_TRUE(manager.beginAttach(0));
    ASSERT_EQ(manager.state(), SessionState::Attaching);
}

TEST_CASE(session_manager_accepts_attach_and_enables_data) {
    SessionManager manager(7);

    ASSERT_TRUE(manager.beginAttach(0));
    ASSERT_TRUE(manager.onAttachAccepted(1001, 50));
    ASSERT_EQ(manager.state(), SessionState::Attached);
    ASSERT_TRUE(manager.canSendData());
    ASSERT_EQ(manager.sessionId(), 1001U);
}
TEST_CASE(detach_retry_exhaustion_does_not_report_released)
{
    SessionTimers timers{};
    timers.detachTimeoutMs = 100;
    timers.maxDetachRetries = 2;

    SessionManager manager(1, timers);

    ASSERT_TRUE(manager.beginAttach(0));
    ASSERT_TRUE(manager.onAttachAccepted(42, 10));
    ASSERT_TRUE(manager.beginDetach(20));

    auto firstRetry = manager.onTick(120);
    ASSERT_TRUE(firstRetry.shouldRetransmit);
    ASSERT_EQ(firstRetry.messageType, MessageType::DetachRequest);
    ASSERT_EQ(manager.state(), SessionState::Detaching);

    auto secondRetry = manager.onTick(220);
    ASSERT_TRUE(secondRetry.shouldRetransmit);
    ASSERT_EQ(secondRetry.messageType, MessageType::DetachRequest);
    ASSERT_EQ(manager.state(), SessionState::Detaching);

    auto exhausted = manager.onTick(320);
    ASSERT_TRUE(!exhausted.shouldRetransmit);

    ASSERT_EQ(manager.state(), SessionState::Rejected);
    ASSERT_EQ(manager.sessionId(), 0U);
}

TEST_CASE(reattach_is_possible_after_failed_detach_retry_exhaustion)
{
    SessionTimers timers{};
    timers.detachTimeoutMs = 100;
    timers.maxDetachRetries = 1;

    SessionManager manager(1, timers);

    ASSERT_TRUE(manager.beginAttach(0));
    ASSERT_TRUE(manager.onAttachAccepted(42, 10));
    ASSERT_TRUE(manager.beginDetach(20));

    auto retry = manager.onTick(120);
    ASSERT_TRUE(retry.shouldRetransmit);
    ASSERT_EQ(retry.messageType, MessageType::DetachRequest);

    auto exhausted = manager.onTick(220);
    ASSERT_TRUE(!exhausted.shouldRetransmit);

    ASSERT_EQ(manager.state(), SessionState::Rejected);
    ASSERT_EQ(manager.sessionId(), 0U);

    ASSERT_TRUE(manager.beginAttach(300));
    ASSERT_EQ(manager.state(), SessionState::Attaching);
    ASSERT_EQ(manager.sessionId(), 0U);
}