# Implementation checklist

## Mandatory

- [X] Implement `SessionManager::beginAttach`
- [X] Implement `SessionManager::onAttachAccepted`
- [X] Implement `SessionManager::beginDetach`
- [X] Implement `SessionManager::onDetachAccepted`
- [X] Implement `SessionManager::onTick`
- [X] Implement `CoreNetwork::handleAttachRequest`
- [X] Implement `CoreNetwork::handleDetachRequest`
- [X] Implement `CoreNetwork::handleHeartbeat`
- [X] Implement `CoreNetwork::handleData`
- [X] Implement `CoreNetwork::expireInactiveSessions`
- [X] Implement `AccessNode::onDatagram`
- [X] Implement `Ue::startAttach`
- [X] Implement `Ue::startDetach`
- [X] Implement `Ue::sendTraffic`
- [X] Implement `Ue::tick`
- [X] Implement `Ue::onDatagram`
- [X] Make all tests green

## Strongly recommended

- [ ] Add logging for attach attempts and retries
- [X] Add tests for duplicate attach request
- [X] Add tests for detach without active session
- [X] Add tests for zero-length payload rejection
- [X] Add tests for inactivity timeout
- [ ] Add tests for multiple UEs (extension)
