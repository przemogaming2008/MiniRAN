#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

mkdir -p "$ROOT_DIR/$CI_LOG_DIR" "$ROOT_DIR/$CI_REPORT_DIR"

LOG="$ROOT_DIR/$CI_LOG_DIR/cli-scenarios.log"
REPORT="$ROOT_DIR/$CI_REPORT_DIR/cli-scenarios.xml"

: > "$LOG"

CLI_BIN=""

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

  cat > "$REPORT" <<XML
<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="cli_scenarios" tests="1" failures="1" errors="0">
  <testcase classname="MiniRAN.CLI" name="cli_binary_exists">
    <failure message="miniran_cli binary not found">miniran_cli binary not found in $ROOT_DIR/$BUILD_DIR</failure>
  </testcase>
</testsuite>
XML

  exit 1
fi

SCENARIOS=(
  "scenarios/tcp_basic.cfg"
  "scenarios/udp_lossy.cfg"
  "scenarios/ci_tcp_heavy.cfg"
  "scenarios/ci_udp_loss_15.cfg"
  "scenarios/ci_low_bandwidth.cfg"
)

xml_escape() {
  local value="$1"
  value="${value//&/&amp;}"
  value="${value//</&lt;}"
  value="${value//>/&gt;}"
  value="${value//\"/&quot;}"
  value="${value//\'/&apos;}"
  printf '%s' "$value"
}

test_count=0
failure_count=0
testcases=""

for scenario in "${SCENARIOS[@]}"; do
  test_count=$((test_count + 1))

  safe_name="${scenario//\//_}"
  safe_name="${safe_name//./_}"
  scenario_log="$ROOT_DIR/$CI_LOG_DIR/cli-${safe_name}.log"

  echo "[CI] Running CLI scenario: $scenario" | tee -a "$LOG"

  if "$CLI_BIN" "$ROOT_DIR/$scenario" > "$scenario_log" 2>&1; then
    status=0
    echo "[CI] PASS: $scenario" | tee -a "$LOG"
  else
    status=$?
    failure_count=$((failure_count + 1))
    echo "[CI] FAIL: $scenario, exit code: $status" | tee -a "$LOG"
  fi

  cat "$scenario_log" >> "$LOG"
  echo "" >> "$LOG"

  escaped_scenario="$(xml_escape "$scenario")"
  escaped_log_path="$(xml_escape "$scenario_log")"

  if [[ "$status" -eq 0 ]]; then
    testcases="${testcases}
  <testcase classname=\"MiniRAN.CLI\" name=\"$escaped_scenario\"/>"
  else
    testcases="${testcases}
  <testcase classname=\"MiniRAN.CLI\" name=\"$escaped_scenario\">
    <failure message=\"Scenario failed with exit code $status\">Scenario: $escaped_scenario
Exit code: $status
Log: $escaped_log_path</failure>
  </testcase>"
  fi
done

cat > "$REPORT" <<XML
<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="cli_scenarios" tests="$test_count" failures="$failure_count" errors="0">$testcases
</testsuite>
XML

echo "[CI] CLI scenarios finished. tests=$test_count failures=$failure_count" | tee -a "$LOG"
echo "[CI] JUnit report: $REPORT" | tee -a "$LOG"

if [[ "$failure_count" -ne 0 ]]; then
  exit 1
fi

exit 0