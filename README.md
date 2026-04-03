# MiniRAN ProtoLab

MiniRAN ProtoLab is an educational C++17 project template for implementing a **simplified telecom protocol** with three visible phases:

1. **Attach UE**
2. **Generate and transfer traffic**
3. **Detach UE**

The project is inspired by the separation of **control plane** and **user plane** known from mobile networks, but it is intentionally much smaller, easier and deterministic enough for unit/component testing.

## Why this template exists

The template already gives the student:
- a ready CMake project,
- a message codec,
- a virtual network with TCP/UDP-like behavior,
- a traffic generator,
- a minimal test framework,
- initial unit tests and component tests,
- scenario configuration files.

The student must still implement the most important telecom logic:
- session state machine,
- UE behavior,
- access node behavior,
- simplified core session handling,
- attach/detach retry rules,
- heartbeat stability handling,
- delivery accounting for data phase.

## Important note

This is **not** a real 3GPP implementation. There is no real radio stack, NAS, NGAP, GTP-U, authentication, encryption or handover. The project is an **educational protocol laboratory**.

## Expected initial status

After extracting the template and building it:
- infrastructure code should compile,
- some **unit tests pass immediately**,
- the tests related to **session handling and end-to-end scenarios are expected to fail** until the student implements the missing logic.

This is intentional.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the example CLI:

```bash
./build/miniran_cli scenarios/tcp_basic.cfg
```

## Project tree

```text
include/miniran/
  common/
  protocol/
  transport/
  traffic/
  core/
  nodes/
  simulation/
src/
  common/
  protocol/
  transport/
  traffic/
  core/
  nodes/
  simulation/
tests/
  unit/
  component/
  support/
scenarios/
```

## Suggested implementation order

1. `SessionManager`
2. `CoreNetwork`
3. `AccessNode`
4. `Ue`
5. Run tests, debug, extend logging
6. Improve scenario metrics and add missing tests

## Ready files vs TODO files

### Mostly ready
- `frame_codec.*`
- `virtual_network.*`
- `traffic_generator.*`
- `scenario_config.*`
- test infrastructure

### Main student work
- `session_manager.*`
- `core_network.*`
- `access_node.*`
- `ue.*`
- making component tests pass

## Design assumptions

- one UE in the provided component scenarios,
- one access node,
- simplified logical core inside the access side,
- uplink data path is enough for the first version,
- TCP mode is reliable and ordered,
- UDP mode may lose or reorder packets,
- control-plane reliability for UDP should be handled by the student with timeouts/retries.

## Commands useful during work

```bash
cmake --build build --target miniran_tests
./build/miniran_tests
./build/miniran_cli scenarios/udp_lossy.cfg
```

## Final deliverable expected from the student

A working implementation should:
- attach a UE,
- keep the session stable with heartbeat logic,
- transfer simulated traffic,
- measure basic throughput and delivery statistics,
- detach the UE cleanly,
- pass all provided tests,
- preferably add extra tests for edge cases.
