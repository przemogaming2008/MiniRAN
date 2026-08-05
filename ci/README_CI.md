# MiniRAN CI Scripts

This directory contains scripts used locally and by Jenkins.

Run scripts from the repository root unless stated otherwise.

## Output directories

Default output paths:

    BUILD_DIR=build/ci
    CI_OUT=ci_out
    CI_LOG_DIR=ci_out/logs
    CI_REPORT_DIR=ci_out/reports
    CI_ARTIFACT_DIR=ci_out/artifacts

Scripts create directories when needed.

## Scripts

### ci_env_report.sh

Purpose:

- checks environment before build
- verifies required tools
- writes environment information

Output:

    ci_out/logs/env.txt

Run:

    bash ci/scripts/ci_env_report.sh

### ci_build.sh

Purpose:

- configures CMake
- builds the project

Output:

    ci_out/logs/build.log

Run:

    bash ci/scripts/ci_build.sh

### ci_test_unit.sh

Purpose:

- runs unit tests through CTest
- runs internal unit test binary with JUnit output

Outputs:

    ci_out/logs/unit-ctest.log
    ci_out/logs/unit-internal.log
    ci_out/reports/unit-ctest.xml
    ci_out/reports/unit-internal.xml

Run:

    bash ci/scripts/ci_test_unit.sh

### ci_test_component.sh

Purpose:

- runs component tests through CTest
- runs internal component test binary with JUnit output

Outputs:

    ci_out/logs/component-ctest.log
    ci_out/logs/component-internal.log
    ci_out/reports/component-ctest.xml
    ci_out/reports/component-internal.xml

Run:

    bash ci/scripts/ci_test_component.sh

### ci_run_cli_scenarios.sh

Purpose:

- runs CLI scenarios
- creates one JUnit testcase per scenario

Outputs:

    ci_out/logs/cli-scenarios.log
    ci_out/reports/cli-scenarios.xml

Run:

    bash ci/scripts/ci_run_cli_scenarios.sh

### ci_mega_gate.sh

Purpose:

- runs the final mega CI gate
- writes CTest and internal JUnit reports

Outputs:

    ci_out/logs/mega-gate.log
    ci_out/logs/mega-internal.log
    ci_out/reports/mega-gate.xml
    ci_out/reports/mega-internal.xml

Run:

    bash ci/scripts/ci_mega_gate.sh

### ci_collect_logs.sh

Purpose:

- writes CI summary
- counts reports and logs
- stores useful last log lines
- creates tar.gz archive when possible

Outputs:

    ci_out/artifacts/summary.md
    ci_out/artifacts/miniran-ci-logs.tar.gz

Run:

    bash ci/scripts/ci_collect_logs.sh

If tar or gzip fails, Jenkins should still archive raw `ci_out` files.

## Recommended full local run

    bash ci/scripts/ci_env_report.sh
    bash ci/scripts/ci_build.sh
    bash ci/scripts/ci_test_unit.sh
    bash ci/scripts/ci_test_component.sh
    bash ci/scripts/ci_run_cli_scenarios.sh
    bash ci/scripts/ci_mega_gate.sh
    bash ci/scripts/ci_collect_logs.sh

## Binary locations

On MSVC/Visual Studio builds, binaries are usually in:

    build/ci/Debug/
    build/ci/Release/

On single-config generators, binaries may be in:

    build/ci/

CI scripts search common locations automatically.

## Failure rule

Every test script should return non-zero when its stage fails.

JUnit should be written when possible.

Jenkins should archive raw logs even if summary archive creation fails.
