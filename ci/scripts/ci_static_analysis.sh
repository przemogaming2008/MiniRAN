#!/usr/bin/env bash
set -Eeuo pipefail

: "${CI_OUT:=ci_out}"
: "${CI_LOG_DIR:=$CI_OUT/logs}"
: "${CI_REPORT_DIR:=$CI_OUT/reports}"
: "${STATIC_BUILD_DIR:=build/static-analysis}"
: "${MINIRAN_STATIC_ANALYSIS_STRICT:=0}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LOG_DIR="$ROOT_DIR/$CI_LOG_DIR"
REPORT_DIR="$ROOT_DIR/$CI_REPORT_DIR"
BUILD_DIR="$ROOT_DIR/$STATIC_BUILD_DIR"

mkdir -p "$LOG_DIR" "$REPORT_DIR" "$BUILD_DIR"

LOG="$LOG_DIR/static-analysis.log"
REPORT="$REPORT_DIR/static-analysis.xml"

failures=0
warnings=0
tests=0

log() {
  echo "$*" | tee -a "$LOG"
}

warn() {
  log "[STATIC] WARNING: $*"
  warnings=$((warnings + 1))
}

fail() {
  log "[STATIC] ERROR: $*"
  failures=$((failures + 1))
}

xml_escape() {
  sed \
    -e 's/&/\&amp;/g' \
    -e 's/</\&lt;/g' \
    -e 's/>/\&gt;/g' \
    -e 's/"/\&quot;/g' \
    -e "s/'/\&apos;/g"
}

write_junit() {
  {
    echo '<?xml version="1.0" encoding="UTF-8"?>'
    echo "<testsuite name=\"static-analysis\" tests=\"$tests\" failures=\"$failures\" skipped=\"0\">"
    cat "$REPORT.tmp" 2>/dev/null || true
    echo '</testsuite>'
  } > "$REPORT"
}

add_pass_case() {
  local name="$1"
  tests=$((tests + 1))
  {
    echo "  <testcase classname=\"static-analysis\" name=\"$name\"/>"
  } >> "$REPORT.tmp"
}

add_fail_case() {
  local name="$1"
  local message="$2"
  tests=$((tests + 1))
  failures=$((failures + 1))

  {
    echo "  <testcase classname=\"static-analysis\" name=\"$name\">"
    echo "    <failure message=\"$name\">"
    printf '%s' "$message" | xml_escape
    echo ""
    echo "    </failure>"
    echo "  </testcase>"
  } >> "$REPORT.tmp"
}

add_warning_case() {
  local name="$1"
  local message="$2"
  tests=$((tests + 1))

  {
    echo "  <testcase classname=\"static-analysis\" name=\"$name\">"
    echo "    <system-out>"
    printf '%s' "$message" | xml_escape
    echo ""
    echo "    </system-out>"
    echo "  </testcase>"
  } >> "$REPORT.tmp"
}

run_or_warn_missing() {
  local tool="$1"

  if command -v "$tool" >/dev/null 2>&1; then
    return 0
  fi

  local message="tool not found: $tool"

  if [[ "$MINIRAN_STATIC_ANALYSIS_STRICT" == "1" ]]; then
    fail "$message"
    add_fail_case "$tool-missing" "$message"
  else
    warn "$message"
    add_warning_case "$tool-missing" "$message"
  fi

  return 1
}

: > "$LOG"
: > "$REPORT.tmp"

log "MiniRAN static analysis"
log "Root: $ROOT_DIR"
log "Build dir: $BUILD_DIR"
log "Strict mode: $MINIRAN_STATIC_ANALYSIS_STRICT"
log ""

log "[STATIC] Configuring compile_commands.json"

if cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DMINIRAN_BUILD_TESTS=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    >> "$LOG" 2>&1; then
  add_pass_case "cmake-configure-static-analysis"
else
  add_fail_case "cmake-configure-static-analysis" "CMake configure failed. See static-analysis.log."
  write_junit
  exit 1
fi

log ""

if run_or_warn_missing "cppcheck"; then
  log "[STATIC] Running cppcheck"

  CPPCHECK_LOG="$LOG_DIR/static-cppcheck.log"

  if cppcheck \
      --enable=warning,style,performance,portability \
      --std=c++17 \
      --inline-suppr \
      --project="$BUILD_DIR/compile_commands.json" \
      --suppress=missingIncludeSystem \
      --error-exitcode=1 \
      > "$CPPCHECK_LOG" 2>&1; then
    add_pass_case "cppcheck"
    log "[STATIC] cppcheck passed"
  else
    add_fail_case "cppcheck" "cppcheck failed. See static-cppcheck.log."
    log "[STATIC] cppcheck failed"
  fi
fi

log ""

if run_or_warn_missing "clang-tidy"; then
  log "[STATIC] Running clang-tidy"

  CLANG_TIDY_LOG="$LOG_DIR/static-clang-tidy.log"
  : > "$CLANG_TIDY_LOG"

  mapfile -t cpp_files < <(
    find "$ROOT_DIR/src" "$ROOT_DIR/include" "$ROOT_DIR/tests" \
      -type f \( -name "*.cpp" -o -name "*.h" \) \
      | sort
  )

  clang_tidy_failed=0

  for file in "${cpp_files[@]}"; do
    echo "[clang-tidy] ${file#$ROOT_DIR/}" >> "$CLANG_TIDY_LOG"

    if ! clang-tidy "$file" -p "$BUILD_DIR" >> "$CLANG_TIDY_LOG" 2>&1; then
      clang_tidy_failed=1
    fi
  done

  if [[ "$clang_tidy_failed" -eq 0 ]]; then
    add_pass_case "clang-tidy"
    log "[STATIC] clang-tidy passed"
  else
    add_fail_case "clang-tidy" "clang-tidy failed. See static-clang-tidy.log."
    log "[STATIC] clang-tidy failed"
  fi
fi

log ""

if run_or_warn_missing "shellcheck"; then
  log "[STATIC] Running shellcheck"

  SHELLCHECK_LOG="$LOG_DIR/static-shellcheck.log"

  if shellcheck "$ROOT_DIR"/ci/scripts/*.sh > "$SHELLCHECK_LOG" 2>&1; then
    add_pass_case "shellcheck"
    log "[STATIC] shellcheck passed"
  else
    add_fail_case "shellcheck" "shellcheck failed. See static-shellcheck.log."
    log "[STATIC] shellcheck failed"
  fi
fi

log ""

if run_or_warn_missing "cmake-format"; then
  log "[STATIC] Running cmake-format check"

  CMAKE_FORMAT_LOG="$LOG_DIR/static-cmake-format.log"

  if cmake-format --check "$ROOT_DIR/CMakeLists.txt" > "$CMAKE_FORMAT_LOG" 2>&1; then
    add_pass_case "cmake-format"
    log "[STATIC] cmake-format passed"
  else
    add_fail_case "cmake-format" "cmake-format failed. See static-cmake-format.log."
    log "[STATIC] cmake-format failed"
  fi
fi

write_junit

log ""
log "[STATIC] Warnings: $warnings"
log "[STATIC] Failures: $failures"
log "[STATIC] Report: $REPORT"

if [[ "$failures" -ne 0 ]]; then
  log "[STATIC] Static analysis failed."
  exit 1
fi

log "[STATIC] Static analysis passed."