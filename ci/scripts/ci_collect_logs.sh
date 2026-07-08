#!/usr/bin/env bash
set -Eeuo pipefail

: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"
: "${CI_ARTIFACT_DIR:=ci_out/artifacts}"

mkdir -p "$CI_LOG_DIR" "$CI_REPORT_DIR" "$CI_ARTIFACT_DIR"

{
  echo "MiniRAN CI collected files"
  echo "Date: $(date -Iseconds)"
  echo ""
  find ci_out -type f 2>/dev/null || true
} > "$CI_ARTIFACT_DIR/ci-files.txt"