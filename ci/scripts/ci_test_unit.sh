#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"

ROOT_DIR="$(pwd)"

mkdir -p "$ROOT_DIR/$CI_LOG_DIR" "$ROOT_DIR/$CI_REPORT_DIR"

ctest --test-dir "$ROOT_DIR/$BUILD_DIR" \
  -C "${CTEST_CONFIG:-Debug}" \
  -L unit \
  --output-on-failure \
  --output-log "$ROOT_DIR/$CI_LOG_DIR/unit-ctest.log" \
  --output-junit "$ROOT_DIR/$CI_REPORT_DIR/unit-ctest.xml"

UNIT_BIN="$ROOT_DIR/$BUILD_DIR/miniran_unit_tests"

if [[ -x "$ROOT_DIR/$BUILD_DIR/Debug/miniran_unit_tests.exe" ]]; then
  UNIT_BIN="$ROOT_DIR/$BUILD_DIR/Debug/miniran_unit_tests.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/Release/miniran_unit_tests.exe" ]]; then
  UNIT_BIN="$ROOT_DIR/$BUILD_DIR/Release/miniran_unit_tests.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/miniran_unit_tests.exe" ]]; then
  UNIT_BIN="$ROOT_DIR/$BUILD_DIR/miniran_unit_tests.exe"
fi

"$UNIT_BIN" --junit "$ROOT_DIR/$CI_REPORT_DIR/unit-internal.xml" \
  2>&1 | tee "$ROOT_DIR/$CI_LOG_DIR/unit-internal.log"
