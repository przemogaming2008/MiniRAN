#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"
mkdir -p "$CI_LOG_DIR" "$CI_REPORT_DIR"

# Wersja szybka: raport z CTest. Wersja lepsza: binarka testowa sama zapisuje JUnit per test case.
ctest --test-dir "$BUILD_DIR" \
  -L unit \
  --output-on-failure \
  --output-log "$CI_LOG_DIR/unit-ctest.log" \
  --output-junit "$CI_REPORT_DIR/unit-ctest.xml"

# Jeżeli dodasz obsługę --junit do test_framework.h, możesz dodatkowo odpalić:
# "$BUILD_DIR/miniran_unit_tests" --junit "$CI_REPORT_DIR/unit-internal.xml" \
#   2>&1 | tee "$CI_LOG_DIR/unit-internal.log"
