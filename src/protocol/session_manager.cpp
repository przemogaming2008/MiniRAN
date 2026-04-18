#include "miniran/protocol/session_manager.h"
namespace miniran {

SessionManager::SessionManager(std::uint32_t ueId, SessionTimers timers) : ueId_(ueId), timers_(timers) {}

std::uint32_t SessionManager::ueId() const {
    return ueId_;
}

std::uint32_t SessionManager::sessionId() const {
    return sessionId_;
}

SessionState SessionManager::state() const {
    return state_;
}

bool SessionManager::isAttached() const {
    return state_ == SessionState::Attached;
}

bool SessionManager::canSendData() const {
    return isAttached();
}

std::uint32_t SessionManager::nextSequenceNumber() {
    return nextSequenceNumber_++;
}

bool SessionManager::beginAttach(std::uint64_t nowMs) {
    // (void)nowMs;
    // TODO(student):
    // 1. Allow transition Idle/Released -> Attaching.

    // Allow re-attach after a previously rejected/failed attach attempt.
    if( state_ == SessionState::Idle || state_ == SessionState::Released || state_ == SessionState::Rejected){

        state_ = SessionState::Attaching;

        // 2. Reset attach retry counter.
        attachRetryCount_= 0;
        // 3. Remember the time of the control transmission.
        lastControlTxMs_ = nowMs;
        // 4. Return true only when a new AttachRequest should be sent.
        return true;
    }
    
    return false;
}

bool SessionManager::onAttachAccepted(std::uint32_t sessionId, std::uint64_t nowMs) {
    // (void)sessionId;
    // (void)nowMs;
    // TODO(student):
    // 1. Accept the session only if the current state is Attaching.
    if(state() != SessionState::Attaching){
        return false;
    }
    // 2. Store the assigned session id.
    sessionId_ = sessionId;
    // 3. Move to Attached.
    state_ = SessionState::Attached;
    // 4. Refresh activity timestamps.
    lastControlTxMs_ = nowMs;
    lastHeartbeatAckMs_ = nowMs;
    return true;
}

bool SessionManager::beginDetach(std::uint64_t nowMs) {
    //(void)nowMs;
    // TODO(student):
    // 1. Allow transition Attached -> Detaching.
    if( state_ == SessionState::Attached){
        state_ = SessionState::Detaching;
        // 2. Reset detach retry counter.
        detachRetryCount_ = 0;
        // 3. Remember last control tx time.
        lastControlTxMs_ = nowMs;
        return true;
    }
    
    return false;
}

bool SessionManager::onDetachAccepted(std::uint64_t nowMs) {
    //(void)nowMs;
    // TODO(student):
    // 1. Accept only if state is Detaching.
    if(state() != SessionState::Detaching){
        return false;
    }
    // 2. Move to Released.
    state_ = SessionState::Released;
    // 3. Optionally clear session id or keep it for post-analysis.
    sessionId_ = 0;
    lastControlTxMs_ = nowMs;
    lastHeartbeatAckMs_ = nowMs;
    return true;
}

void SessionManager::onHeartbeatResponse(std::uint64_t nowMs) {
    // TODO(student):
    // Update the last successful activity timestamp.
    lastHeartbeatAckMs_ = nowMs;
}

RetryDecision SessionManager::onTick(std::uint64_t nowMs) {
    //(void)nowMs;
    // TODO(student):
    // - If Attaching and timeout expired, request retransmission of AttachRequest.
    if(state_ == SessionState::Attaching && (nowMs - lastControlTxMs_) >= timers_.attachTimeoutMs){
        if (attachRetryCount_ < timers_.maxAttachRetries) {
            ++attachRetryCount_;
            lastControlTxMs_ = nowMs;
            return {true, MessageType::AttachRequest};
        } else {
            // - If retries are exhausted, decide whether to move to Rejected/Released.
            state_ = SessionState::Rejected;
            attachRetryCount_ = 0;
            return {};
        }
    }
    // - If Detaching and timeout expired, request retransmission of DetachRequest.
    if (state_ == SessionState::Detaching &&
        (nowMs - lastControlTxMs_) >= timers_.detachTimeoutMs)
    {
        if (detachRetryCount_ < timers_.maxDetachRetries) {
            ++detachRetryCount_;
            lastControlTxMs_ = nowMs;
            return {true, MessageType::DetachRequest};
        } else {
            //detach retries exhausted.
            state_ = SessionState::Released;
            detachRetryCount_ = 0;
            return {};
        }
    }
    // - If Attached and heartbeat interval elapsed, you may also request a Heartbeat.
    if (state_ == SessionState::Attached &&
        (nowMs - lastControlTxMs_) >= timers_.heartbeatIntervalMs)
    {
        lastControlTxMs_ = nowMs;
        return {true, MessageType::Heartbeat};
    }

    return {};

}

void SessionManager::reset() {
    state_ = SessionState::Idle;
    sessionId_ = 0;
    nextSequenceNumber_ = 1;
    lastControlTxMs_ = 0;
    lastHeartbeatAckMs_ = 0;
    attachRetryCount_ = 0;
    detachRetryCount_ = 0;
}

}  // namespace miniran
