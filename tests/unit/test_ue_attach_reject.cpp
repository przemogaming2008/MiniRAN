#include "miniran/nodes/ue.h"
#include "miniran/protocol/frame_codec.h"
#include "support/test_framework.h"

using namespace miniran;

namespace {

Datagram makeAttachRejectDatagram(std::uint32_t accessNodeId,
                                  std::uint32_t ueId,
                                  std::uint64_t nowMs)
{
    ProtocolMessage reject = makeMessage(
        MessageType::AttachReject,
        ueId,
        0,
        1,
        nowMs
    );

    Datagram datagram{};
    datagram.fromNodeId = accessNodeId;
    datagram.toNodeId = ueId;
    datagram.enqueueTimeMs = nowMs;
    datagram.controlPlane = true;
    datagram.bytes = FrameCodec::encode(reject);

    return datagram;
}

}  // namespace

TEST_CASE(ue_enters_rejected_state_after_attach_reject)
{
    constexpr std::uint32_t ueId = 7;
    constexpr std::uint32_t accessNodeId = 1000;

    SessionTimers timers{};
    Ue ue(ueId, accessNodeId, timers);

    ue.startAttach(0);

    ASSERT_EQ(ue.state(), SessionState::Attaching);
    ASSERT_TRUE(!ue.flushOutgoing().empty());

    const Datagram reject =
        makeAttachRejectDatagram(accessNodeId, ueId, 10);

    ue.onDatagram(reject, 10);

    ASSERT_EQ(ue.state(), SessionState::Rejected);
    ASSERT_TRUE(!ue.isAttached());
}

TEST_CASE(ue_can_start_new_attach_after_rejected_state)
{
    constexpr std::uint32_t ueId = 7;
    constexpr std::uint32_t accessNodeId = 1000;

    SessionTimers timers{};
    Ue ue(ueId, accessNodeId, timers);

    ue.startAttach(0);

    const Datagram reject =
        makeAttachRejectDatagram(accessNodeId, ueId, 10);

    ue.onDatagram(reject, 10);

    ASSERT_EQ(ue.state(), SessionState::Rejected);

    ue.startAttach(20);

    ASSERT_EQ(ue.state(), SessionState::Attaching);
    ASSERT_TRUE(!ue.flushOutgoing().empty());
}