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

check_tool git
check_tool cmake
check_tool ctest

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
