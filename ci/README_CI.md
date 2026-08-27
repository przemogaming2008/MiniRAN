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

The sanitizer script uses a separate build directory:

    SANITIZE_BUILD_DIR=build/sanitize

Scripts create directories when needed.

## Scripts

### ci_env_report.sh

Purpose:

- checks environment before build
- verifies required tools
- verifies required project files
- verifies required CI scripts
- verifies required scenarios
- verifies Jenkins/CMake pipeline contracts
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

### ci_test_sanitizers.sh

Purpose:

- creates a separate sanitizer build
- enables sanitizer flags when supported
- runs unit tests
- runs component tests
- runs one CLI smoke scenario

Build directory:

    build/sanitize

Outputs:

    ci_out/logs/sanitize-build.log
    ci_out/logs/sanitize-unit-ctest.log
    ci_out/logs/sanitize-unit-internal.log
    ci_out/logs/sanitize-component-ctest.log
    ci_out/logs/sanitize-component-internal.log
    ci_out/logs/sanitize-cli-smoke.log

    ci_out/reports/sanitize-unit-ctest.xml
    ci_out/reports/sanitize-unit-internal.xml
    ci_out/reports/sanitize-component-ctest.xml
    ci_out/reports/sanitize-component-internal.xml

Run:

    bash ci/scripts/ci_test_sanitizers.sh

This script is part of the normal Jenkins pipeline.

On GCC/Clang it enables AddressSanitizer and UndefinedBehaviorSanitizer when supported.

On MSVC it enables AddressSanitizer. On Windows, the script also tries to find the Visual Studio ASan runtime DLL:

    clang_rt.asan_dynamic-x86_64.dll

### ci_static_analysis.sh

Purpose:

- configures a separate static analysis build
- creates `compile_commands.json`
- runs static analysis tools when available
- writes a JUnit report

Build directory:

    build/static-analysis

Tools used when available:

- cppcheck
- clang-tidy
- shellcheck
- cmake-format

Default mode:

    MINIRAN_STATIC_ANALYSIS_STRICT=0

In default mode, missing tools are warnings.

Strict mode:

    MINIRAN_STATIC_ANALYSIS_STRICT=1

In strict mode, missing tools or analysis failures fail the stage.

Outputs:

    ci_out/logs/static-analysis.log
    ci_out/reports/static-analysis.xml

Optional tool logs:

    ci_out/logs/static-cppcheck.log
    ci_out/logs/static-clang-tidy.log
    ci_out/logs/static-shellcheck.log
    ci_out/logs/static-cmake-format.log

Run:

    bash ci/scripts/ci_static_analysis.sh

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
    bash ci/scripts/ci_test_sanitizers.sh
    bash ci/scripts/ci_static_analysis.sh
    bash ci/scripts/ci_collect_logs.sh

This sequence should match the normal Jenkins pipeline.

## Binary locations

On MSVC/Visual Studio builds, binaries are usually in:

    build/ci/Debug/
    build/ci/Release/

On single-config generators, binaries may be in:

    build/ci/

Sanitizer binaries are usually in:

    build/sanitize/Debug/
    build/sanitize/Release/

or, on single-config generators:

    build/sanitize/

CI scripts search common locations automatically.

## Failure rule

Every test script should return non-zero when its stage fails.

JUnit should be written when possible.

Jenkins should archive raw logs even if summary archive creation fails.

A sanitizer failure is treated as a real CI failure.