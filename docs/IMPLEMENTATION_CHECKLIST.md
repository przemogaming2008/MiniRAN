# Implementation checklist

## Mandatory

- [ ] Implement `SessionManager::beginAttach`
- [ ] Implement `SessionManager::onAttachAccepted`
- [ ] Implement `SessionManager::beginDetach`
- [ ] Implement `SessionManager::onDetachAccepted`
- [ ] Implement `SessionManager::onTick`
- [ ] Implement `CoreNetwork::handleAttachRequest`
- [ ] Implement `CoreNetwork::handleDetachRequest`
- [ ] Implement `CoreNetwork::handleHeartbeat`
- [ ] Implement `CoreNetwork::handleData`
- [ ] Implement `CoreNetwork::expireInactiveSessions`
- [ ] Implement `AccessNode::onDatagram`
- [ ] Implement `Ue::startAttach`
- [ ] Implement `Ue::startDetach`
- [ ] Implement `Ue::sendTraffic`
- [ ] Implement `Ue::tick`
- [ ] Implement `Ue::onDatagram`
- [ ] Make all tests green

## Strongly recommended

- [ ] Add logging for attach attempts and retries
- [ ] Add tests for duplicate attach request
- [ ] Add tests for detach without active session
- [ ] Add tests for zero-length payload rejection
- [ ] Add tests for inactivity timeout
- [ ] Add tests for multiple UEs (extension)
