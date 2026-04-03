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
