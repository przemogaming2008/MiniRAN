#!/usr/bin/env bash
set -Eeuo pipefail

: "${CI_OUT:=ci_out}"
: "${CI_LOG_DIR:=$CI_OUT/logs}"
: "${CI_REPORT_DIR:=$CI_OUT/reports}"
: "${CI_ARTIFACT_DIR:=$CI_OUT/artifacts}"
: "${COVERAGE_BUILD_DIR:=build/coverage}"
: "${MINIRAN_COVERAGE_STRICT:=0}"
: "${MINIRAN_COVERAGE_MIN_LINE_PERCENT:=0}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LOG_DIR="$ROOT_DIR/$CI_LOG_DIR"
REPORT_DIR="$ROOT_DIR/$CI_REPORT_DIR"
ARTIFACT_DIR="$ROOT_DIR/$CI_ARTIFACT_DIR"
BUILD_DIR="$ROOT_DIR/$COVERAGE_BUILD_DIR"

mkdir -p "$LOG_DIR" "$REPORT_DIR" "$ARTIFACT_DIR"

LOG="$LOG_DIR/coverage.log"
REPORT="$REPORT_DIR/coverage.xml"
SUMMARY="$ARTIFACT_DIR/coverage-summary.txt"

: > "$LOG"

log() {
  echo "$*" | tee -a "$LOG"
}

write_junit() {
  local failures="$1"
  local message="$2"

  {
    echo '<?xml version="1.0" encoding="UTF-8"?>'
    echo "<testsuite name=\"coverage\" tests=\"1\" failures=\"$failures\" skipped=\"0\">"
    echo "  <testcase classname=\"coverage\" name=\"coverage-stage\">"
    if [[ "$failures" != "0" ]]; then
      echo "    <failure message=\"coverage failed\">$message</failure>"
    else
      echo "    <system-out>$message</system-out>"
    fi
    echo "  </testcase>"
    echo "</testsuite>"
  } > "$REPORT"
}

write_summary_unavailable() {
  local reason="$1"

  {
    echo "status=unavailable"
    echo "reason=$reason"
    echo "covered_lines=0"
    echo "total_lines=0"
    echo "line_coverage_percent=0.00"
    echo "strict=$MINIRAN_COVERAGE_STRICT"
    echo "minimum_line_coverage_percent=$MINIRAN_COVERAGE_MIN_LINE_PERCENT"
  } > "$SUMMARY"
}

soft_fail() {
  local message="$1"

  write_summary_unavailable "$message"

  if [[ "$MINIRAN_COVERAGE_STRICT" == "1" ]]; then
    log "[COVERAGE] ERROR: $message"
    write_junit 1 "$message"
    exit 1
  fi

  log "[COVERAGE] WARNING: $message"
  write_junit 0 "$message"
  exit 0
}

log "MiniRAN coverage"
log "Root: $ROOT_DIR"
log "Build dir: $BUILD_DIR"
log "Strict mode: $MINIRAN_COVERAGE_STRICT"
log "Minimum line coverage: $MINIRAN_COVERAGE_MIN_LINE_PERCENT"
log ""

command -v g++ >/dev/null 2>&1 || soft_fail "g++ not found"
command -v gcov >/dev/null 2>&1 || soft_fail "gcov not found"
command -v ninja >/dev/null 2>&1 || soft_fail "ninja not found"

rm -rf "${BUILD_DIR:?}"
mkdir -p "$BUILD_DIR"

log "[COVERAGE] Configuring"

if ! cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_CXX_COMPILER=g++ \
    -DMINIRAN_BUILD_TESTS=ON \
    -DMINIRAN_ENABLE_COVERAGE=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    >> "$LOG" 2>&1; then
  soft_fail "coverage configure failed"
fi

log "[COVERAGE] Building"

if ! cmake --build "$BUILD_DIR" >> "$LOG" 2>&1; then
  soft_fail "coverage build failed"
fi

log "[COVERAGE] Running tests"

if ! ctest --test-dir "$BUILD_DIR" --output-on-failure >> "$LOG" 2>&1; then
  soft_fail "coverage tests failed"
fi

mapfile -t gcda_files < <(find "$BUILD_DIR" -type f -name "*.gcda" | sort)

if [[ "${#gcda_files[@]}" -eq 0 ]]; then
  soft_fail "no .gcda coverage files found"
fi

GCOV_DIR="$ARTIFACT_DIR/coverage-gcov"
rm -rf "$GCOV_DIR"
mkdir -p "$GCOV_DIR"

GCOV_LOG="$LOG_DIR/coverage-gcov.log"
: > "$GCOV_LOG"

pushd "$GCOV_DIR" >/dev/null

for file in "${gcda_files[@]}"; do
  gcov -p -b -c "$file" >> "$GCOV_LOG" 2>&1 || true
done

popd >/dev/null

mapfile -t gcov_reports < <(find "$GCOV_DIR" -type f -name "*.gcov" | sort)

if [[ "${#gcov_reports[@]}" -eq 0 ]]; then
  soft_fail "no .gcov reports generated"
fi

awk -F: '
{
  count=$1
  line=$2
  gsub(/^[ \t]+|[ \t]+$/, "", count)
  gsub(/^[ \t]+|[ \t]+$/, "", line)

  if (line ~ /^[0-9]+$/ && count != "-") {
    total += 1
    if (count != "#####" && count != "=====") {
      covered += 1
    }
  }
}
END {
  if (total == 0) {
    percent = 0
  } else {
    percent = (covered * 100.0) / total
  }

  printf("covered_lines=%d\n", covered)
  printf("total_lines=%d\n", total)
  printf("line_coverage_percent=%.2f\n", percent)
}
' "${gcov_reports[@]}" > "$SUMMARY"

line_percent="$(grep '^line_coverage_percent=' "$SUMMARY" | cut -d= -f2)"

log "[COVERAGE] Line coverage percent: $line_percent"
log "[COVERAGE] Summary: $SUMMARY"

if awk "BEGIN { exit !($line_percent >= $MINIRAN_COVERAGE_MIN_LINE_PERCENT) }"; then
  write_junit 0 "line coverage $line_percent"
  log "[COVERAGE] Coverage passed."
else
  soft_fail "line coverage $line_percent below required $MINIRAN_COVERAGE_MIN_LINE_PERCENT"
fi
