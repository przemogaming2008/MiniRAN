#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

mkdir -p "$ROOT_DIR/$CI_LOG_DIR" "$ROOT_DIR/$CI_REPORT_DIR"

ctest --test-dir "$ROOT_DIR/$BUILD_DIR" \
  -C "${CTEST_CONFIG:-Debug}" \
  -L component \
  --output-on-failure \
  --output-log "$ROOT_DIR/$CI_LOG_DIR/component-ctest.log" \
  --output-junit "$ROOT_DIR/$CI_REPORT_DIR/component-ctest.xml"

COMPONENT_BIN="$ROOT_DIR/$BUILD_DIR/miniran_component_tests"

if [[ -x "$ROOT_DIR/$BUILD_DIR/Debug/miniran_component_tests.exe" ]]; then
  COMPONENT_BIN="$ROOT_DIR/$BUILD_DIR/Debug/miniran_component_tests.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/Release/miniran_component_tests.exe" ]]; then
  COMPONENT_BIN="$ROOT_DIR/$BUILD_DIR/Release/miniran_component_tests.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/miniran_component_tests.exe" ]]; then
  COMPONENT_BIN="$ROOT_DIR/$BUILD_DIR/miniran_component_tests.exe"
fi

"$COMPONENT_BIN" --junit "$ROOT_DIR/$CI_REPORT_DIR/component-internal.xml" \
  2>&1 | tee "$ROOT_DIR/$CI_LOG_DIR/component-internal.log"