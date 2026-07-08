#!/usr/bin/env bash
set -Eeuo pipefail

: "${CI_LOG_DIR:=ci_out/logs}"
mkdir -p "$CI_LOG_DIR"

{
  echo "MiniRAN CI environment report"
  echo "Date: $(date -Iseconds)"
  echo "Job: ${JOB_NAME:-local}"
  echo "Build: ${BUILD_NUMBER:-0}"
  echo "Branch: ${BRANCH_NAME:-unknown}"
  echo "Commit: ${GIT_COMMIT:-unknown}"
  echo "Workspace: ${WORKSPACE:-$(pwd)}"
  echo ""
  echo "Tools:"
  git --version || true
  cmake --version || true
  ctest --version || true
  c++ --version || true
} | tee "$CI_LOG_DIR/env.txt"
