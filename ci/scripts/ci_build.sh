#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"
: "${CI_ARTIFACT_DIR:=ci_out/artifacts}"

mkdir -p "$BUILD_DIR" "$CI_LOG_DIR" "$CI_REPORT_DIR" "$CI_ARTIFACT_DIR"

echo "[CI] Configure MiniRAN" | tee "$CI_LOG_DIR/build.log"
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMINIRAN_BUILD_TESTS=ON \
  2>&1 | tee -a "$CI_LOG_DIR/build.log"

echo "[CI] Build MiniRAN" | tee -a "$CI_LOG_DIR/build.log"
: "${CI_BUILD_JOBS:=2}"

echo "[CI] Build jobs: $CI_BUILD_JOBS" | tee -a "$CI_LOG_DIR/build.log"

cmake --build "$BUILD_DIR" --parallel "$CI_BUILD_JOBS" \
  2>&1 | tee -a "$CI_LOG_DIR/build.log"

