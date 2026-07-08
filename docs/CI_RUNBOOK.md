
# MiniRAN CI Runbook

## Lokalny baseline

cmake -S . -B build

cmake --build build

ctest --test-dir build --output-on-failure

./build/miniran_tests

./build/miniran_cli scenarios/tcp_basic.cfg

Jeżeli te komendy nie działają lokalnie,
Jenkins też nie będzie działał.
