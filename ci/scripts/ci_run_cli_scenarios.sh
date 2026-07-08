#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"

mkdir -p "$CI_LOG_DIR" "$CI_REPORT_DIR"

LOG="$CI_LOG_DIR/cli-scenarios.log"
: > "$LOG"

CLI_BIN="$BUILD_DIR/miniran_cli"

if [[ -x "$BUILD_DIR/Debug/miniran_cli.exe" ]]; then
  CLI_BIN="$BUILD_DIR/Debug/miniran_cli.exe"
elif [[ -x "$BUILD_DIR/Release/miniran_cli.exe" ]]; then
  CLI_BIN="$BUILD_DIR/Release/miniran_cli.exe"
elif [[ -x "$BUILD_DIR/miniran_cli.exe" ]]; then
  CLI_BIN="$BUILD_DIR/miniran_cli.exe"
elif [[ -x "$BUILD_DIR/miniran_cli" ]]; then
  CLI_BIN="$BUILD_DIR/miniran_cli"
else
  echo "[CI] ERROR: miniran_cli binary not found in $BUILD_DIR" | tee -a "$LOG"
  exit 1
fi

for scenario in \
  scenarios/tcp_basic.cfg \
  scenarios/udp_lossy.cfg \
  scenarios/ci_tcp_heavy.cfg \
  scenarios/ci_udp_loss_15.cfg \
  scenarios/ci_low_bandwidth.cfg
do
  echo "[CI] Running CLI scenario: $scenario" | tee -a "$LOG"
  "$CLI_BIN" "$scenario" 2>&1 | tee -a "$LOG"
done

cat > "$CI_REPORT_DIR/cli-scenarios.xml" <<XML
<testsuite name="cli_scenarios" tests="1" failures="0">
  <testcase classname="MiniRAN.CLI" name="cli_scenarios"/>
</testsuite>
XML
