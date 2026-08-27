# MiniRAN CI Runbook

This file explains how to run MiniRAN CI and where to look after a failure.

Keep it simple: always start with the first failing stage.

## 1. Required environment

The agent or local machine needs:

- Bash
- Git
- CMake 3.21 or newer
- CTest 3.21 or newer
- C++17 compiler
- tar
- gzip
- write access to the workspace

For the sanitizer stage:

- GCC/Clang can use AddressSanitizer and UndefinedBehaviorSanitizer
- MSVC can use AddressSanitizer
- on Windows/MSVC, Visual Studio ASan runtime may be needed:

  clang_rt.asan_dynamic-x86_64.dll

The Jenkinsfile uses:

    agent any

The agent does not need a special Jenkins label. The preflight script checks the real tools and required project files.

## 2. Full local CI run

Run from repository root:

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

Expected output directories:

    ci_out/logs/
    ci_out/reports/
    ci_out/artifacts/

Normal build directory:

    build/ci

Sanitizer build directory:

    build/sanitize

## 3. Jenkins stages

### 00 Clean workspace outputs

Removes old generated files:

    build/ci
    ci_out

Then creates:

    ci_out/logs
    ci_out/reports
    ci_out/artifacts

If this fails, check workspace permissions.

### 01 Preflight - środowisko

Runs:

    bash ci/scripts/ci_env_report.sh

Main log:

    ci_out/logs/env.txt

Check this log when:

- tool is missing
- CMake or CTest is too old
- compiler is missing
- required CI script is missing
- required scenario CFG file is missing
- Jenkinsfile does not match expected pipeline scripts
- CMake test targets or labels are missing
- workspace is not writable
- disk space is low
- repository root is wrong

### 02 Configure + Build

Runs:

    bash ci/scripts/ci_build.sh

Main log:

    ci_out/logs/build.log

Check this log when:

- CMake configure fails
- compilation fails
- source file is missing from CMake
- compiler environment is wrong

### 03 Unit tests

Runs:

    bash ci/scripts/ci_test_unit.sh

Logs:

    ci_out/logs/unit-ctest.log
    ci_out/logs/unit-internal.log

Reports:

    ci_out/reports/unit-ctest.xml
    ci_out/reports/unit-internal.xml

Use `unit-internal.xml` to find the exact failing `TEST_CASE`.

### 04 Component tests

Runs:

    bash ci/scripts/ci_test_component.sh

Logs:

    ci_out/logs/component-ctest.log
    ci_out/logs/component-internal.log

Reports:

    ci_out/reports/component-ctest.xml
    ci_out/reports/component-internal.xml

Component failures usually mean that full scenario behavior is broken.

### 05 CLI scenarios

Runs:

    bash ci/scripts/ci_run_cli_scenarios.sh

Log:

    ci_out/logs/cli-scenarios.log

Report:

    ci_out/reports/cli-scenarios.xml

Each scenario should be visible as a separate JUnit testcase.

### 06 Mega CI Gate

Runs:

    bash ci/scripts/ci_mega_gate.sh

Logs:

    ci_out/logs/mega-gate.log
    ci_out/logs/mega-internal.log

Reports:

    ci_out/reports/mega-gate.xml
    ci_out/reports/mega-internal.xml

Mega Gate is the final functional safety check. Treat a Mega Gate failure as release-blocking.

### 07 Sanitizers

Runs:

    bash ci/scripts/ci_test_sanitizers.sh

Build directory:

    build/sanitize

Logs:

    ci_out/logs/sanitize-build.log
    ci_out/logs/sanitize-unit-ctest.log
    ci_out/logs/sanitize-unit-internal.log
    ci_out/logs/sanitize-component-ctest.log
    ci_out/logs/sanitize-component-internal.log
    ci_out/logs/sanitize-cli-smoke.log

Reports:

    ci_out/reports/sanitize-unit-ctest.xml
    ci_out/reports/sanitize-unit-internal.xml
    ci_out/reports/sanitize-component-ctest.xml
    ci_out/reports/sanitize-component-internal.xml

This stage checks memory and undefined-behavior problems that normal functional tests may not catch.

A sanitizer failure is a real CI failure.

On Windows/MSVC, if this stage fails with missing `clang_rt.asan_dynamic-x86_64.dll`, check the Visual Studio C++ AddressSanitizer runtime installation.

### 08 Static analysis

Runs:

    bash ci/scripts/ci_static_analysis.sh

Build directory:

    build/static-analysis

Main log:

    ci_out/logs/static-analysis.log

Report:

    ci_out/reports/static-analysis.xml

Optional extra logs may be created when tools are available:

    ci_out/logs/static-cppcheck.log
    ci_out/logs/static-clang-tidy.log
    ci_out/logs/static-shellcheck.log
    ci_out/logs/static-cmake-format.log

This stage checks source quality with external static analysis tools.

Supported tools:

- cppcheck
- clang-tidy
- shellcheck
- cmake-format

Default mode:

    MINIRAN_STATIC_ANALYSIS_STRICT=0

In default mode, missing tools are reported as warnings.

Strict mode:

    MINIRAN_STATIC_ANALYSIS_STRICT=1

In strict mode, missing tools or static analysis failures fail the stage.

## 4. Post actions

After every build Jenkins tries to:

1. collect logs
2. archive `ci_out/**/*`
3. publish JUnit XML reports

Important rule:

A failure in log collection must not block raw artifact archiving or JUnit publishing.

Artifact archiving and JUnit publishing should be separate attempts.

This protects diagnostics when something breaks.

## 5. Most useful files after failure

Start here:

    ci_out/artifacts/summary.md

Then open the log for the first failing stage:

    ci_out/logs/env.txt
    ci_out/logs/build.log
    ci_out/logs/unit-ctest.log
    ci_out/logs/unit-internal.log
    ci_out/logs/component-ctest.log
    ci_out/logs/component-internal.log
    ci_out/logs/cli-scenarios.log
    ci_out/logs/mega-gate.log
    ci_out/logs/mega-internal.log
    ci_out/logs/sanitize-build.log
    ci_out/logs/sanitize-unit-ctest.log
    ci_out/logs/sanitize-unit-internal.log
    ci_out/logs/sanitize-component-ctest.log
    ci_out/logs/sanitize-component-internal.log
    ci_out/logs/sanitize-cli-smoke.log

JUnit reports are in:

    ci_out/reports/

Archived package:

    ci_out/artifacts/miniran-ci-logs.tar.gz

If the tar archive is missing, still use raw files under `ci_out/`.

## 6. Common problems

### Job waits in queue

Possible reason:

- no Jenkins agent is online
- all executors are busy
- agent has wrong OS or missing tools

Check:

- Jenkins node list
- build queue message
- Jenkins system log

The Jenkinsfile uses `agent any`, so there is no required label like `bash`.

### Preflight fails

Open:

    ci_out/logs/env.txt

Fix missing tools, missing files, old CMake/CTest, compiler setup, disk space or permissions.

### Build fails

Open:

    ci_out/logs/build.log

Fix compiler errors or CMake configuration.

### Tests fail

Open the matching internal XML and log:

    ci_out/reports/*-internal.xml
    ci_out/logs/*-internal.log

The internal XML should show the exact `TEST_CASE`.

### CLI scenario fails

Open:

    ci_out/logs/cli-scenarios.log
    ci_out/reports/cli-scenarios.xml

Then run the failing scenario manually, for example:

    ./build/ci/Debug/miniran_cli.exe scenarios/tcp_basic.cfg

### Mega Gate fails

Open:

    ci_out/logs/mega-gate.log
    ci_out/logs/mega-internal.log
    ci_out/reports/mega-internal.xml

Mega Gate failures usually mean that a final project-level contract is broken.

### Sanitizer fails

Open:

    ci_out/logs/sanitize-build.log

Then check the exact failing layer:

    ci_out/logs/sanitize-unit-ctest.log
    ci_out/logs/sanitize-unit-internal.log
    ci_out/logs/sanitize-component-ctest.log
    ci_out/logs/sanitize-component-internal.log
    ci_out/logs/sanitize-cli-smoke.log

On Windows/MSVC, a missing ASan DLL usually means that this file is not available in PATH:

    clang_rt.asan_dynamic-x86_64.dll

The sanitizer script tries to find it automatically under Visual Studio.

### JUnit is missing

If tests started and no XML exists, this is a CI error.

Check:

    ci_out/logs/
    ci_out/reports/

### Artifact archive is missing

Use raw files under:

    ci_out/

The tar.gz archive is useful, but it is not the only diagnostic source.

### SCM or credentials failure

This happens before project scripts start.

Check Jenkins job configuration:

- repository URL
- branch name
- credentials
- checkout log
- Jenkins system log

### Static analysis fails

Open:

    ci_out/logs/static-analysis.log
    ci_out/reports/static-analysis.xml

Then check tool-specific logs if they exist:

    ci_out/logs/static-cppcheck.log
    ci_out/logs/static-clang-tidy.log
    ci_out/logs/static-shellcheck.log
    ci_out/logs/static-cmake-format.log

If tools are missing and strict mode is disabled, this should be a warning.

If strict mode is enabled, install the missing tools or disable strict mode.

## 7. Manual verification commands

Check aggregate target:

    cmake --build build/ci --target miniran_tests

Check CTest labels:

    ctest --test-dir build/ci -C Debug -N
    ctest --test-dir build/ci -C Debug -N -L all

Run aggregate test:

    ctest --test-dir build/ci -C Debug -L all --output-on-failure

Run unit binary manually on Windows/MSVC:

    ./build/ci/Debug/miniran_unit_tests.exe --junit ci_out/reports/manual-unit.xml

Run CLI manually on Windows/MSVC:

    ./build/ci/Debug/miniran_cli.exe scenarios/tcp_basic.cfg

Run sanitizer stage manually:

    bash ci/scripts/ci_test_sanitizers.sh

Run static analysis manually:

    bash ci/scripts/ci_static_analysis.sh

Run static analysis in strict mode:

    MINIRAN_STATIC_ANALYSIS_STRICT=1 bash ci/scripts/ci_static_analysis.sh
    
## 8. Controlled failure check

Use this only to test diagnostics. Do not commit controlled failures.

### Example unit failure

1. Add temporary `ASSERT_TRUE(false);` inside one unit test.
2. Run unit stage.
3. Expected result:
   - unit stage fails
   - JUnit XML is created
   - exact `TEST_CASE` is visible
   - logs are archived
4. Remove the temporary change.

### Example build failure

1. Add a temporary syntax error in one `.cpp` file.
2. Run build.
3. Expected result:
   - build stage fails
   - `build.log` is archived
   - missing JUnit does not hide the build failure
4. Remove the temporary change.

### Example sanitizer failure

1. Add a temporary memory error in a test.
2. Run:

   bash ci/scripts/ci_test_sanitizers.sh
3. Expected result:

   - sanitizer stage fails
   - sanitizer logs are written
   - JUnit reports are written when possible
   - raw `ci_out` files can still be archived
4. Remove the temporary change.

## 9. Generated files policy

Do not commit generated files:

    build/
    ci_out/
    Testing/
    *.log
    *.xml

Before commit:

    git status --short
