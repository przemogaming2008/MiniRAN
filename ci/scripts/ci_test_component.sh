#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"
mkdir -p "$CI_LOG_DIR" "$CI_REPORT_DIR"

ctest --test-dir "$BUILD_DIR" \
  -C "${CTEST_CONFIG:-Debug}" \
  -L component \
  --output-on-failure \
  --output-log "$CI_LOG_DIR/component-ctest.log" \
  --output-junit "$CI_REPORT_DIR/component-ctest.xml"
