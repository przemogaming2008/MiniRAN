#include "miniran/protocol/session_manager.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(reattach_after_released){
    SessionManager session(7);

    ASSERT_TRUE(session.beginAttach(100));
    ASSERT_EQ(session.state(), SessionState::Attaching);

    session.onAttachAccepted(1000, 110);
    ASSERT_EQ(session.state(), SessionState::Attached);

    ASSERT_TRUE(session.beginDetach(200));
    ASSERT_EQ(session.state(), SessionState::Detaching);

    session.onDetachAccepted(210);
    ASSERT_EQ(session.state(), SessionState::Released);

    // Released is not terminal: a new attach attempt is allowed.
    ASSERT_TRUE(session.beginAttach(300));
    ASSERT_EQ(session.state(), SessionState::Attaching);
}