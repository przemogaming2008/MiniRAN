#!/usr/bin/env bash
set -Eeuo pipefail

: "${SANITIZE_BUILD_DIR:=build/sanitize}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"
: "${CI_ARTIFACT_DIR:=ci_out/artifacts}"
: "${CI_BUILD_JOBS:=2}"
: "${SANITIZE_CTEST_CONFIG:=Debug}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

mkdir -p "$ROOT_DIR/$CI_LOG_DIR" "$ROOT_DIR/$CI_REPORT_DIR" "$ROOT_DIR/$CI_ARTIFACT_DIR"

SANITIZE_BUILD_LOG="$ROOT_DIR/$CI_LOG_DIR/sanitize-build.log"

find_binary() {
  local binary_name="$1"
  local candidate

  for candidate in \
    "$ROOT_DIR/$SANITIZE_BUILD_DIR/Debug/${binary_name}.exe" \
    "$ROOT_DIR/$SANITIZE_BUILD_DIR/Release/${binary_name}.exe" \
    "$ROOT_DIR/$SANITIZE_BUILD_DIR/${binary_name}.exe" \
    "$ROOT_DIR/$SANITIZE_BUILD_DIR/Debug/${binary_name}" \
    "$ROOT_DIR/$SANITIZE_BUILD_DIR/Release/${binary_name}" \
    "$ROOT_DIR/$SANITIZE_BUILD_DIR/${binary_name}"
  do
    if [[ -x "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  echo "[CI] ERROR: Cannot find executable: $binary_name" >&2
  return 1
}

add_msvc_asan_runtime_to_path() {
  local dll_name="clang_rt.asan_dynamic-x86_64.dll"
  local dll_path=""

  case "${OSTYPE:-}" in
    msys*|cygwin*|win32*) ;;
    *) return 0 ;;
  esac

  IFS=':' read -ra path_entries <<< "$PATH"

  for path_entry in "${path_entries[@]}"; do
    if [[ -f "$path_entry/$dll_name" ]]; then
      echo "[CI] ASan runtime already available: $path_entry/$dll_name"
      return 0
    fi
  done

  if [[ -d "/c/Program Files/Microsoft Visual Studio" ]]; then
    dll_path="$(
      find "/c/Program Files/Microsoft Visual Studio" \
        -path "*/bin/Hostx64/x64/$dll_name" \
        -print \
        -quit \
        2>/dev/null || true
    )"
  fi

  if [[ -z "$dll_path" && -d "/c/Program Files (x86)/Microsoft Visual Studio" ]]; then
    dll_path="$(
      find "/c/Program Files (x86)/Microsoft Visual Studio" \
        -path "*/bin/Hostx64/x64/$dll_name" \
        -print \
        -quit \
        2>/dev/null || true
    )"
  fi

  if [[ -z "$dll_path" ]]; then
    echo "[CI] ERROR: Cannot find $dll_name." >&2
    echo "[CI] MSVC AddressSanitizer runtime is missing or not available." >&2
    echo "[CI] Install Visual Studio C++ AddressSanitizer support or run from a Developer Shell." >&2
    return 1
  fi

  local dll_dir
  dll_dir="$(dirname "$dll_path")"

  export PATH="$dll_dir:$PATH"

  echo "[CI] Added ASan runtime directory to PATH: $dll_dir"
}

echo "[CI] Configure sanitizer build" | tee "$SANITIZE_BUILD_LOG"

rm -rf "$ROOT_DIR/$SANITIZE_BUILD_DIR"

cmake -S "$ROOT_DIR" -B "$ROOT_DIR/$SANITIZE_BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMINIRAN_BUILD_TESTS=ON \
  -DMINIRAN_ENABLE_SANITIZERS=ON \
  2>&1 | tee -a "$SANITIZE_BUILD_LOG"

echo "[CI] Build sanitizer binaries" | tee -a "$SANITIZE_BUILD_LOG"
echo "[CI] Build jobs: $CI_BUILD_JOBS" | tee -a "$SANITIZE_BUILD_LOG"

cmake --build "$ROOT_DIR/$SANITIZE_BUILD_DIR" --parallel "$CI_BUILD_JOBS" \
  2>&1 | tee -a "$SANITIZE_BUILD_LOG"

add_msvc_asan_runtime_to_path

echo "[CI] Run sanitizer unit tests"

ctest --test-dir "$ROOT_DIR/$SANITIZE_BUILD_DIR" \
  -C "$SANITIZE_CTEST_CONFIG" \
  -L unit \
  --output-on-failure \
  --output-log "$ROOT_DIR/$CI_LOG_DIR/sanitize-unit-ctest.log" \
  --output-junit "$ROOT_DIR/$CI_REPORT_DIR/sanitize-unit-ctest.xml"

UNIT_BIN="$(find_binary miniran_unit_tests)"

"$UNIT_BIN" --junit "$ROOT_DIR/$CI_REPORT_DIR/sanitize-unit-internal.xml" \
  2>&1 | tee "$ROOT_DIR/$CI_LOG_DIR/sanitize-unit-internal.log"

echo "[CI] Run sanitizer component tests"

ctest --test-dir "$ROOT_DIR/$SANITIZE_BUILD_DIR" \
  -C "$SANITIZE_CTEST_CONFIG" \
  -L component \
  --output-on-failure \
  --output-log "$ROOT_DIR/$CI_LOG_DIR/sanitize-component-ctest.log" \
  --output-junit "$ROOT_DIR/$CI_REPORT_DIR/sanitize-component-ctest.xml"

COMPONENT_BIN="$(find_binary miniran_component_tests)"

"$COMPONENT_BIN" --junit "$ROOT_DIR/$CI_REPORT_DIR/sanitize-component-internal.xml" \
  2>&1 | tee "$ROOT_DIR/$CI_LOG_DIR/sanitize-component-internal.log"

echo "[CI] Run sanitizer CLI smoke"

CLI_BIN="$(find_binary miniran_cli)"

SCENARIO_FILE="${SANITIZER_SMOKE_SCENARIO:-}"

if [[ -z "$SCENARIO_FILE" ]]; then
  SCENARIO_FILE="$(find "$ROOT_DIR/scenarios" -maxdepth 1 -type f -name "*.cfg" | sort | head -n 1 || true)"
elif [[ "$SCENARIO_FILE" != /* ]]; then
  SCENARIO_FILE="$ROOT_DIR/$SCENARIO_FILE"
fi

if [[ -z "$SCENARIO_FILE" || ! -f "$SCENARIO_FILE" ]]; then
  echo "[CI] ERROR: No sanitizer CLI smoke scenario found." >&2
  echo "[CI] Set SANITIZER_SMOKE_SCENARIO=scenarios/name.cfg if needed." >&2
  exit 1
fi

echo "[CI] Sanitizer CLI smoke scenario: $SCENARIO_FILE" \
  | tee "$ROOT_DIR/$CI_LOG_DIR/sanitize-cli-smoke.log"

"$CLI_BIN" "$SCENARIO_FILE" \
  2>&1 | tee -a "$ROOT_DIR/$CI_LOG_DIR/sanitize-cli-smoke.log"

echo "[CI] Sanitizer checks completed."
