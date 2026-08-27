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
REPORT_TMP="$REPORT.tmp"
COMPILE_COMMANDS="$BUILD_DIR/compile_commands.json"

CMAKE_GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  CMAKE_GENERATOR_ARGS=(-G Ninja)
fi

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

error_log() {
  log "[STATIC] ERROR: $*"
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
    cat "$REPORT_TMP" 2>/dev/null || true
    echo '</testsuite>'
  } > "$REPORT"
}

add_pass_case() {
  local name="$1"
  tests=$((tests + 1))
  echo "  <testcase classname=\"static-analysis\" name=\"$name\"/>" >> "$REPORT_TMP"
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
  } >> "$REPORT_TMP"
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
  } >> "$REPORT_TMP"
}

record_problem() {
  local name="$1"
  local message="$2"

  if [[ "$MINIRAN_STATIC_ANALYSIS_STRICT" == "1" ]]; then
    error_log "$message"
    add_fail_case "$name" "$message"
  else
    warn "$message"
    add_warning_case "$name" "$message"
  fi
}

require_or_record_missing() {
  local tool="$1"

  if command -v "$tool" >/dev/null 2>&1; then
    return 0
  fi

  record_problem "$tool-missing" "tool not found: $tool"
  return 1
}

: > "$LOG"
: > "$REPORT_TMP"

log "MiniRAN static analysis"
log "Root: $ROOT_DIR"
log "Build dir: $BUILD_DIR"
log "Strict mode: $MINIRAN_STATIC_ANALYSIS_STRICT"
log ""

log "[STATIC] Configuring static analysis build"

configure_static_build() {
  local generator_name="$1"
  shift

  rm -rf "${BUILD_DIR:?}"
  mkdir -p "$BUILD_DIR"

  log "[STATIC] CMake generator: $generator_name"

  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    "$@" \
    -DMINIRAN_BUILD_TESTS=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    >> "$LOG" 2>&1
}

configured=0

if command -v ninja >/dev/null 2>&1; then
  if configure_static_build "Ninja" -G Ninja; then
    configured=1
    add_pass_case "cmake-configure-static-analysis"
  else
    warn "Ninja configure failed; retrying with default CMake generator."
  fi
fi

if [[ "$configured" -eq 0 ]]; then
  if configure_static_build "default"; then
    configured=1
    add_pass_case "cmake-configure-static-analysis"
  else
    error_log "CMake configure failed"
    add_fail_case "cmake-configure-static-analysis" "CMake configure failed with Ninja/default generator. See static-analysis.log."
    write_junit
    exit 1
  fi
fi

if [[ -f "$COMPILE_COMMANDS" ]]; then
  add_pass_case "compile-commands-present"
  log "[STATIC] compile_commands.json found"
else
  record_problem "compile-commands-missing" "compile_commands.json not produced by this CMake generator; clang-tidy will be skipped."
fi

log ""

if require_or_record_missing "cppcheck"; then
  log "[STATIC] Running cppcheck"

  CPPCHECK_LOG="$LOG_DIR/static-cppcheck.log"

  if [[ -f "$COMPILE_COMMANDS" ]]; then
    if cppcheck \
        --enable=warning,style,performance,portability \
        --std=c++17 \
        --inline-suppr \
        --project="$COMPILE_COMMANDS" \
        --suppress=missingIncludeSystem \
        --error-exitcode=1 \
        > "$CPPCHECK_LOG" 2>&1; then
      add_pass_case "cppcheck"
      log "[STATIC] cppcheck passed"
    else
      record_problem "cppcheck" "cppcheck reported findings. See static-cppcheck.log."
    fi
  else
    if cppcheck \
        --enable=warning,style,performance,portability \
        --std=c++17 \
        --inline-suppr \
        -I "$ROOT_DIR/include" \
        --suppress=missingIncludeSystem \
        --error-exitcode=1 \
        "$ROOT_DIR/src" "$ROOT_DIR/include" "$ROOT_DIR/tests" \
        > "$CPPCHECK_LOG" 2>&1; then
      add_pass_case "cppcheck"
      log "[STATIC] cppcheck passed"
    else
      record_problem "cppcheck" "cppcheck reported findings. See static-cppcheck.log."
    fi
  fi
fi

log ""

if require_or_record_missing "clang-tidy"; then
  if [[ ! -f "$COMPILE_COMMANDS" ]]; then
    record_problem "clang-tidy-skipped" "clang-tidy skipped because compile_commands.json is missing."
  else
    log "[STATIC] Running clang-tidy"

    CLANG_TIDY_LOG="$LOG_DIR/static-clang-tidy.log"
    : > "$CLANG_TIDY_LOG"

    mapfile -t cpp_files < <(
      find "$ROOT_DIR/src" "$ROOT_DIR/tests" \
        -type f -name "*.cpp" \
        | sort
    )

    clang_tidy_failed=0

    for file in "${cpp_files[@]}"; do
      echo "[clang-tidy] ${file#"$ROOT_DIR"/}" >> "$CLANG_TIDY_LOG"

      if ! clang-tidy \
          --checks='clang-analyzer-*,bugprone-*,performance-*' \
          --header-filter='.*' \
          "$file" \
          -p "$BUILD_DIR" \
          >> "$CLANG_TIDY_LOG" 2>&1; then
        clang_tidy_failed=1
      fi
    done

    if [[ "$clang_tidy_failed" -eq 0 ]]; then
      add_pass_case "clang-tidy"
      log "[STATIC] clang-tidy passed"
    else
      record_problem "clang-tidy" "clang-tidy reported findings. See static-clang-tidy.log."
    fi
  fi
fi

log ""

if require_or_record_missing "shellcheck"; then
  log "[STATIC] Running shellcheck"

  SHELLCHECK_LOG="$LOG_DIR/static-shellcheck.log"

  mapfile -t shell_scripts < <(
    find "$ROOT_DIR/ci/scripts" -maxdepth 1 -type f -name "*.sh" | sort
  )

  if shellcheck "${shell_scripts[@]}" > "$SHELLCHECK_LOG" 2>&1; then
    add_pass_case "shellcheck"
    log "[STATIC] shellcheck passed"
  else
    record_problem "shellcheck" "shellcheck reported findings. See static-shellcheck.log."
  fi
fi

log ""

if require_or_record_missing "cmake-format"; then
  log "[STATIC] Running cmake-format check"

  CMAKE_FORMAT_LOG="$LOG_DIR/static-cmake-format.log"

  if cmake-format --check "$ROOT_DIR/CMakeLists.txt" > "$CMAKE_FORMAT_LOG" 2>&1; then
    add_pass_case "cmake-format"
    log "[STATIC] cmake-format passed"
  else
    record_problem "cmake-format" "cmake-format reported formatting differences. See static-cmake-format.log."
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
