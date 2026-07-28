#!/usr/bin/env bash
set -Eeuo pipefail

: "${CI_LOG_DIR:=ci_out/logs}"

ROOT_DIR="$(pwd)"
mkdir -p "$ROOT_DIR/$CI_LOG_DIR"

LOG="$ROOT_DIR/$CI_LOG_DIR/env.txt"

{
  echo "MiniRAN CI environment report"
  echo "Date: $(date -Iseconds)"
  echo "Job: ${JOB_NAME:-local}"
  echo "Build: ${BUILD_NUMBER:-0}"
  echo "Branch: ${BRANCH_NAME:-unknown}"
  echo "Commit: ${GIT_COMMIT:-unknown}"
  echo "Workspace: ${WORKSPACE:-$ROOT_DIR}"
  echo ""
  echo "Tools:"
} | tee "$LOG"

missing=0

check_tool() {
  local tool="$1"

  if command -v "$tool" >/dev/null 2>&1; then
    {
      echo ""
      echo "[$tool]"
      "$tool" --version
    } | tee -a "$LOG"
  else
    echo "[CI] ERROR: required tool not found: $tool" | tee -a "$LOG"
    missing=1
  fi
}
version_at_least() {
  local actual="$1"
  local required="$2"

  local actual_major actual_minor actual_patch
  local required_major required_minor required_patch

  IFS='.' read -r actual_major actual_minor actual_patch <<< "$actual"
  IFS='.' read -r required_major required_minor required_patch <<< "$required"

  actual_major="${actual_major:-0}"
  actual_minor="${actual_minor:-0}"
  actual_patch="${actual_patch:-0}"

  required_major="${required_major:-0}"
  required_minor="${required_minor:-0}"
  required_patch="${required_patch:-0}"

  if (( actual_major > required_major )); then
    return 0
  fi

  if (( actual_major < required_major )); then
    return 1
  fi

  if (( actual_minor > required_minor )); then
    return 0
  fi

  if (( actual_minor < required_minor )); then
    return 1
  fi

  (( actual_patch >= required_patch ))
}

tool_version() {
  local tool="$1"

  "$tool" --version | head -n 1 | grep -Eo '[0-9]+(\.[0-9]+)+' | head -n 1
}

check_tool_min_version() {
  local tool="$1"
  local required="$2"

  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "[CI] ERROR: required tool not found: $tool" | tee -a "$LOG"
    missing=1
    return
  fi

  local actual
  actual="$(tool_version "$tool")"

  if [[ -z "$actual" ]]; then
    echo "[CI] ERROR: cannot detect $tool version" | tee -a "$LOG"
    missing=1
    return
  fi

  {
    echo ""
    echo "[$tool]"
    "$tool" --version
  } | tee -a "$LOG"

  if ! version_at_least "$actual" "$required"; then
    echo "[CI] ERROR: $tool version $actual is too old. Required: $required or newer." | tee -a "$LOG"
    missing=1
  fi
}

check_tool git
check_tool_min_version cmake 3.21.0
check_tool_min_version ctest 3.21.0

if command -v c++ >/dev/null 2>&1; then
  {
    echo ""
    echo "[c++]"
    c++ --version
  } | tee -a "$LOG"
elif command -v cl >/dev/null 2>&1; then
  {
    echo ""
    echo "[cl]"
    cl 2>&1 | head -n 5
  } | tee -a "$LOG"
else
  echo "[CI] ERROR: required C++ compiler not found: c++ or cl" | tee -a "$LOG"
  missing=1
fi

if [[ "$missing" -ne 0 ]]; then
  echo "" | tee -a "$LOG"
  echo "[CI] Preflight failed: missing required tools." | tee -a "$LOG"
  exit 1
fi

echo "" | tee -a "$LOG"
echo "[CI] Preflight passed." | tee -a "$LOG"
