#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"

ROOT_DIR="$(pwd)"

mkdir -p "$ROOT_DIR/$CI_LOG_DIR" "$ROOT_DIR/$CI_REPORT_DIR"

LOG="$ROOT_DIR/$CI_LOG_DIR/cli-scenarios.log"
: > "$LOG"

CLI_BIN="$ROOT_DIR/$BUILD_DIR/miniran_cli"

if [[ -x "$ROOT_DIR/$BUILD_DIR/Debug/miniran_cli.exe" ]]; then
  CLI_BIN="$ROOT_DIR/$BUILD_DIR/Debug/miniran_cli.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/Release/miniran_cli.exe" ]]; then
  CLI_BIN="$ROOT_DIR/$BUILD_DIR/Release/miniran_cli.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/miniran_cli.exe" ]]; then
  CLI_BIN="$ROOT_DIR/$BUILD_DIR/miniran_cli.exe"
elif [[ -x "$ROOT_DIR/$BUILD_DIR/miniran_cli" ]]; then
  CLI_BIN="$ROOT_DIR/$BUILD_DIR/miniran_cli"
else
  echo "[CI] ERROR: miniran_cli binary not found in $ROOT_DIR/$BUILD_DIR" | tee -a "$LOG"
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
  "$CLI_BIN" "$ROOT_DIR/$scenario" 2>&1 | tee -a "$LOG"
done

cat > "$ROOT_DIR/$CI_REPORT_DIR/cli-scenarios.xml" <<XML
<testsuite name="cli_scenarios" tests="1" failures="0">
  <testcase classname="MiniRAN.CLI" name="cli_scenarios"/>
</testsuite>
XML
