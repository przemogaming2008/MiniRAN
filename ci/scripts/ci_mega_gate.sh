#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"
mkdir -p "$CI_LOG_DIR" "$CI_REPORT_DIR"

ctest --test-dir "$BUILD_DIR" \
  -L mega \
  --output-on-failure \
  --output-log "$CI_LOG_DIR/mega-gate.log" \
  --output-junit "$CI_REPORT_DIR/mega-gate.xml"
