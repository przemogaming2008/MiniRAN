#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_OUT:=ci_out}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"
: "${CI_ARTIFACT_DIR:=ci_out/artifacts}"

mkdir -p "$CI_LOG_DIR" "$CI_REPORT_DIR" "$CI_ARTIFACT_DIR"

{
  echo "# MiniRAN CI summary"
  echo ""
  echo "Build URL: ${BUILD_URL:-local-run}"
  echo "Job name: ${JOB_NAME:-local}"
  echo "Build number: ${BUILD_NUMBER:-0}"
  echo "Git branch: ${BRANCH_NAME:-unknown}"
  echo "Git commit: ${GIT_COMMIT:-unknown}"
  echo ""
  echo "## Ważne pliki"
  find "$CI_OUT" -maxdepth 3 -type f | sort
} > "$CI_ARTIFACT_DIR/summary.md"

tar -czf "$CI_ARTIFACT_DIR/miniran-ci-logs.tar.gz" "$CI_LOG_DIR" "$CI_REPORT_DIR" "$CI_ARTIFACT_DIR/summary.md"