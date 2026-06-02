#include "miniran/protocol/session_manager.h"

namespace miniran {

SessionManager::SessionManager(std::uint32_t ueId, SessionTimers timers)
    : ueId_(ueId), timers_(timers) {}

std::uint32_t SessionManager::ueId() const {
    return ueId_;
}

std::uint32_t SessionManager::sessionId() const {
    return sessionId_;
}

std::uint32_t SessionManager::lastSessionId() const {
    return lastSessionId_;
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

bool SessionManager::detachConfirmed() const {
    return detachConfirmed_;
}

std::uint32_t SessionManager::nextSequenceNumber() {
    return nextSequenceNumber_++;
}

bool SessionManager::beginAttach(std::uint64_t nowMs) {
    if (state_ == SessionState::Idle ||
        state_ == SessionState::Released ||
        state_ == SessionState::Rejected) {

        state_ = SessionState::Attaching;

        // New attach must start without an active session id.
        sessionId_ = 0;

        attachRetryCount_ = 0;
        detachRetryCount_ = 0;
        detachConfirmed_ = false;

        lastControlTxMs_ = nowMs;
        lastHeartbeatAckMs_ = nowMs;
        lastHeartbeatTxMs_ = nowMs;

        return true;
    }

    return false;
}

bool SessionManager::onAttachAccepted(std::uint32_t sessionId, std::uint64_t nowMs) {
    if (state_ != SessionState::Attaching) {
        return false;
    }

    if (sessionId == 0) {
        return false;
    }

    sessionId_ = sessionId;
    lastSessionId_ = sessionId_;

    state_ = SessionState::Attached;
    detachConfirmed_ = false;

    lastControlTxMs_ = nowMs;
    lastHeartbeatAckMs_ = nowMs;
    lastHeartbeatTxMs_ = nowMs;

    return true;
}

bool SessionManager::beginDetach(std::uint64_t nowMs) {
    if (state_ == SessionState::Attached) {
        detachConfirmed_ = false;

        state_ = SessionState::Detaching;

        detachRetryCount_ = 0;
        lastControlTxMs_ = nowMs;

        return true;
    }

    return false;
}

bool SessionManager::onDetachAccepted(std::uint64_t nowMs) {
    if (state_ != SessionState::Detaching) {
        return false;
    }

    detachConfirmed_ = true;

    lastSessionId_ = sessionId_;
    sessionId_ = 0;

    state_ = SessionState::Released;

    lastControlTxMs_ = nowMs;
    lastHeartbeatAckMs_ = nowMs;
    lastHeartbeatTxMs_ = nowMs;

    return true;
}

void SessionManager::onHeartbeatResponse(std::uint64_t nowMs) {
    lastHeartbeatAckMs_ = nowMs;
}

RetryDecision SessionManager::onTick(std::uint64_t nowMs) {
    if (state_ == SessionState::Attaching &&
        (nowMs - lastControlTxMs_) >= timers_.attachTimeoutMs) {

        if (attachRetryCount_ < timers_.maxAttachRetries) {
            ++attachRetryCount_;
            lastControlTxMs_ = nowMs;
            return {true, MessageType::AttachRequest};
        }

        state_ = SessionState::Rejected;
        sessionId_ = 0;
        attachRetryCount_ = 0;
        detachConfirmed_ = false;
        return {};
    }

    if (state_ == SessionState::Detaching &&
        (nowMs - lastControlTxMs_) >= timers_.detachTimeoutMs) {

        if (detachRetryCount_ < timers_.maxDetachRetries) {
            ++detachRetryCount_;
            lastControlTxMs_ = nowMs;
            return {true, MessageType::DetachRequest};
        }

        lastSessionId_ = sessionId_;
        sessionId_ = 0;

        state_ = SessionState::Released;
        detachConfirmed_ = false;
        detachRetryCount_ = 0;
        return {};
    }

    if (state_ == SessionState::Attached &&
        (nowMs - lastHeartbeatAckMs_) >= timers_.inactivityTimeoutMs) {

        lastSessionId_ = sessionId_;
        sessionId_ = 0;

        state_ = SessionState::Released;
        detachConfirmed_ = false;
        return {};
    }

    if (state_ == SessionState::Attached &&
        (nowMs - lastHeartbeatTxMs_) >= timers_.heartbeatIntervalMs) {

        lastHeartbeatTxMs_ = nowMs;
        return {true, MessageType::Heartbeat};
    }

    return {};
}

void SessionManager::reset() {
    state_ = SessionState::Idle;

    sessionId_ = 0;
    lastSessionId_ = 0;
    nextSequenceNumber_ = 1;

    lastControlTxMs_ = 0;
    lastHeartbeatAckMs_ = 0;
    lastHeartbeatTxMs_ = 0;

    attachRetryCount_ = 0;
    detachRetryCount_ = 0;

    detachConfirmed_ = false;
}

}  // namespace miniran
