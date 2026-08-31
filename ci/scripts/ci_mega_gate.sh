#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

mkdir -p "$ROOT_DIR/$CI_LOG_DIR" "$ROOT_DIR/$CI_REPORT_DIR"

stage_status=0

if ctest --test-dir "$ROOT_DIR/$BUILD_DIR" \
  -C "${CTEST_CONFIG:-Debug}" \
  -L mega \
  --output-on-failure \
  --output-log "$ROOT_DIR/$CI_LOG_DIR/mega-gate.log" \
  --output-junit "$ROOT_DIR/$CI_REPORT_DIR/mega-gate.xml"; then
  echo "[CI] Mega CTest passed."
else
  echo "[CI] ERROR: Mega CTest failed; continuing to create detailed internal JUnit."
  stage_status=1
fi

MEGA_BIN="$ROOT_DIR/$BUILD_DIR/miniran_mega_tests"

if [[ -x "$ROOT_DIR/$BUILD_DIR/Debug/miniran_mega_tests.exe" ]]; then
  MEGA_BIN="$ROOT_DIR/$BUILD_DIR/Debug/miniran_mega_tests.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/Release/miniran_mega_tests.exe" ]]; then
  MEGA_BIN="$ROOT_DIR/$BUILD_DIR/Release/miniran_mega_tests.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/miniran_mega_tests.exe" ]]; then
  MEGA_BIN="$ROOT_DIR/$BUILD_DIR/miniran_mega_tests.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/miniran_mega_tests" ]]; then
  MEGA_BIN="$ROOT_DIR/$BUILD_DIR/miniran_mega_tests"
else
  echo "[CI] ERROR: miniran_mega_tests binary not found" | tee -a "$ROOT_DIR/$CI_LOG_DIR/mega-internal.log"
  exit 1
fi

if "$MEGA_BIN" --junit "$ROOT_DIR/$CI_REPORT_DIR/mega-internal.xml" \
  2>&1 | tee "$ROOT_DIR/$CI_LOG_DIR/mega-internal.log"; then
  echo "[CI] Mega internal JUnit completed."
else
  echo "[CI] ERROR: Mega internal test binary failed."
  stage_status=1
fi

exit "$stage_status"
