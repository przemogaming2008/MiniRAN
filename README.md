# MiniRAN CI Factory

MiniRAN CI Factory is a small C++17 project that simulates a simplified telecom flow:

1. UE attach
2. user traffic
3. heartbeat/session supervision
4. UE detach
5. CI reports and logs

The goal of the project is not only working C++ code. The goal is also a reliable CI pipeline that can show clearly what failed and where to look.

## What is implemented

The project contains:

- C++17 library `miniran_lib`
- CLI application `miniran_cli`
- unit tests
- component tests
- mega CI gate tests
- CMake test targets
- Jenkins pipeline
- Bash CI scripts
- JUnit XML reports
- collected logs and artifacts

## What this project is not

MiniRAN is not a real 3GPP implementation.

It does not implement real radio, real TCP/UDP sockets, authentication, encryption, paging, handover or multiple cells.

It is a deterministic educational simulation used for testing protocol logic and CI quality.

## Required tools

Minimum local tools:

- CMake 3.21 or newer
- CTest 3.21 or newer
- C++17 compiler
- Bash-compatible shell
- tar
- gzip

On Windows, Git Bash with MSVC is supported.

## Repository layout

    include/miniran/      public headers
    src/                  implementation
    tests/unit/           unit tests
    tests/component/      component and mega tests
    tests/support/        small test framework
    scenarios/            scenario configuration files
    ci/scripts/           local and Jenkins CI scripts
    ci/jenkins/           Jenkins operating notes
    docs/                 project documentation
    Jenkinsfile           Jenkins pipeline
    CMakeLists.txt        CMake build configuration

## Local quick start

Run from repository root:

    bash ci/scripts/ci_env_report.sh
    bash ci/scripts/ci_build.sh
    bash ci/scripts/ci_test_unit.sh
    bash ci/scripts/ci_test_component.sh
    bash ci/scripts/ci_run_cli_scenarios.sh
    bash ci/scripts/ci_mega_gate.sh
    bash ci/scripts/ci_collect_logs.sh

Generated output goes to:

    ci_out/logs/
    ci_out/reports/
    ci_out/artifacts/

## Manual CMake usage

Clean configure and build:

    cmake -S . -B build/ci
    cmake --build build/ci

Run all CTest tests:

    ctest --test-dir build/ci -C Debug --output-on-failure

Run aggregate baseline test target:

    cmake --build build/ci --target miniran_tests
    ctest --test-dir build/ci -C Debug -L all --output-on-failure

Run CLI manually:

    ./build/ci/Debug/miniran_cli.exe scenarios/tcp_basic.cfg

On single-config generators the executable may be:

    ./build/ci/miniran_cli scenarios/tcp_basic.cfg

## Test targets

CMake creates these test binaries:

- `miniran_unit_tests`
- `miniran_component_tests`
- `miniran_mega_tests`
- `miniran_tests`

CTest tests:

- `unit_tests` with label `unit`
- `component_tests` with label `component`
- `mega_gate_tests` with label `mega`
- `all_tests` with label `all`

`miniran_tests` is the simple aggregate baseline binary. It contains unit and component tests. Mega gate stays separate.

## CLI exit codes

`miniran_cli` returns:

- `0` healthy scenario result
- `1` missing CLI argument
- `2` invalid scenario configuration
- `3` scenario ran but result was unhealthy

A TCP-like scenario is healthy only when generated traffic is delivered completely and there are no rejected network submissions.

## Jenkins

The Jenkins pipeline is defined in:

    Jenkinsfile

The pipeline uses:

    agent any

The real environment is checked by:

    ci/scripts/ci_env_report.sh

So the agent can have any Jenkins label, but it must have the required tools.

## More documentation

Read:

- `docs/CI_RUNBOOK.md`
- `ci/README_CI.md`
- `ci/jenkins/README_LOG_RECORDER.md`
