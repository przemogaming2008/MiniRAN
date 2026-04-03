# Architecture notes

## Logical data flow

UE -> VirtualNetwork -> AccessNode -> CoreNetwork

- `UE` owns session state and decides what to send.
- `VirtualNetwork` models delay, loss, bandwidth and optional reordering.
- `AccessNode` decodes datagrams and routes them by message type.
- `CoreNetwork` decides if a session exists, is active and whether data should be counted.

## Attach phase

1. UE sends `AttachRequest`
2. AccessNode forwards it to CoreNetwork
3. CoreNetwork allocates a session id and answers with `AttachAccept`
4. UE changes state from `Attaching` to `Attached`

## Traffic phase

- Traffic generator creates a stream of payloads
- UE wraps each payload into a `Data` message
- AccessNode validates session state
- CoreNetwork counts delivered bytes and packets

## Stability phase

- UE periodically sends `Heartbeat`
- Access/Core refresh `last_seen_ms`
- missing heartbeats should eventually trigger session expiration

## Detach phase

1. UE sends `DetachRequest`
2. CoreNetwork removes session
3. AccessNode returns `DetachAccept`
4. UE moves to `Released`
