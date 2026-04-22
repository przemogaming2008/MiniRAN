#include "miniran/protocol/session_manager.h"
#include "support/test_framework.h"

using namespace miniran;

TEST_CASE(detach_in_invalid_states){
    SessionManager session(7);

    //Idle -> detach not allowed
    ASSERT_TRUE(!session.beginDetach(100));
    ASSERT_EQ(session.state(), SessionState::Idle);

    //Attaching -> detach not allowed
    ASSERT_TRUE(session.beginAttach(110));
    ASSERT_EQ(session.state(), SessionState::Attaching);

    ASSERT_TRUE(!session.beginDetach(120));
    ASSERT_EQ(session.state(), SessionState::Attaching);

    //Move to Attached
    session.onAttachAccepted(1000, 130);
    ASSERT_EQ(session.state(), SessionState::Attached);

    //Now detach is allowed
    ASSERT_TRUE(session.beginDetach(140));
    ASSERT_EQ(session.state(), SessionState::Detaching);

    //Simulate successful detach -> Released
    session.onDetachAccepted(150);
    ASSERT_EQ(session.state(), SessionState::Released);

    //Released -> detach not allowed
    ASSERT_TRUE(!session.beginDetach(160));
    ASSERT_EQ(session.state(), SessionState::Released);
}