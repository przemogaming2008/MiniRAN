#!/usr/bin/env bash
set -Eeuo pipefail

: "${BUILD_DIR:=build/ci}"
: "${CI_OUT:=ci_out}"
: "${CI_LOG_DIR:=ci_out/logs}"
: "${CI_REPORT_DIR:=ci_out/reports}"
: "${CI_ARTIFACT_DIR:=ci_out/artifacts}"
: "${STATIC_BUILD_DIR:=build/static-analysis}"
: "${COVERAGE_BUILD_DIR:=build/coverage}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

mkdir -p "$ROOT_DIR/$CI_LOG_DIR" "$ROOT_DIR/$CI_REPORT_DIR" "$ROOT_DIR/$CI_ARTIFACT_DIR"

SUMMARY="$ROOT_DIR/$CI_ARTIFACT_DIR/summary.md"
ARCHIVE="$ROOT_DIR/$CI_ARTIFACT_DIR/miniran-ci-logs.tar.gz"
ARCHIVE_TMP="$ROOT_DIR/$CI_OUT/miniran-ci-logs.tar.gz.tmp"

count_files() {
  local dir="$1"
  local pattern="$2"

  if [[ ! -d "$dir" ]]; then
    echo 0
    return
  fi

  find "$dir" -type f -name "$pattern" | wc -l | tr -d ' '
}

write_missing_report_note() {
  local report="$1"

  if [[ ! -f "$ROOT_DIR/$CI_REPORT_DIR/$report" ]]; then
    echo "- Missing report: \`$CI_REPORT_DIR/$report\`"
  fi
}

write_missing_log_note() {
  local log_file="$1"

  if [[ ! -f "$ROOT_DIR/$CI_LOG_DIR/$log_file" ]]; then
    echo "- Missing log: \`$CI_LOG_DIR/$log_file\`"
  fi
}

write_missing_artifact_note() {
  local artifact="$1"

  if [[ ! -e "$ROOT_DIR/$CI_ARTIFACT_DIR/$artifact" ]]; then
    echo "- Missing artifact: \`$CI_ARTIFACT_DIR/$artifact\`"
  fi
}

{
  echo "# MiniRAN CI summary"
  echo ""
  echo "## Build metadata"
  echo ""
  echo "- Build URL: ${BUILD_URL:-local-run}"
  echo "- Job name: ${JOB_NAME:-local}"
  echo "- Build number: ${BUILD_NUMBER:-0}"
  echo "- Git branch: ${BRANCH_NAME:-unknown}"
  echo "- Git commit: ${GIT_COMMIT:-unknown}"
  echo "- Workspace: ${WORKSPACE:-$ROOT_DIR}"
  echo ""
  echo "## CI paths"
  echo ""
  echo "- Build directory: \`$BUILD_DIR\`"
  echo "- Static analysis build directory: \`$STATIC_BUILD_DIR\`"
  echo "- Coverage build directory: \`$COVERAGE_BUILD_DIR\`"
  echo "- CI output directory: \`$CI_OUT\`"
  echo "- Logs directory: \`$CI_LOG_DIR\`"
  echo "- Reports directory: \`$CI_REPORT_DIR\`"
  echo "- Artifacts directory: \`$CI_ARTIFACT_DIR\`"
  echo ""
  echo "## Report counts"
  echo ""
  echo "- JUnit XML reports: $(count_files "$ROOT_DIR/$CI_REPORT_DIR" "*.xml")"
  echo "- Log files: $(count_files "$ROOT_DIR/$CI_LOG_DIR" "*.log")"
  echo "- Text logs: $(count_files "$ROOT_DIR/$CI_LOG_DIR" "*.txt")"
  echo ""
  echo "## Expected reports"
  echo ""
  write_missing_report_note "unit-ctest.xml"
  write_missing_report_note "unit-internal.xml"
  write_missing_report_note "component-ctest.xml"
  write_missing_report_note "component-internal.xml"
  write_missing_report_note "cli-scenarios.xml"
  write_missing_report_note "mega-gate.xml"
  write_missing_report_note "mega-internal.xml"
  write_missing_report_note "sanitize-unit-ctest.xml"
  write_missing_report_note "sanitize-unit-internal.xml"
  write_missing_report_note "sanitize-component-ctest.xml"
  write_missing_report_note "sanitize-component-internal.xml"
  write_missing_report_note "static-analysis.xml"
  write_missing_report_note "coverage.xml"
  echo ""
  echo "## Expected artifacts"
  echo ""
  write_missing_artifact_note "coverage-summary.txt"
  echo ""
  echo "## Expected logs"
  echo ""
  write_missing_log_note "env.txt"
  write_missing_log_note "build.log"
  write_missing_log_note "unit-ctest.log"
  write_missing_log_note "unit-internal.log"
  write_missing_log_note "component-ctest.log"
  write_missing_log_note "component-internal.log"
  write_missing_log_note "cli-scenarios.log"
  write_missing_log_note "mega-gate.log"
  write_missing_log_note "mega-internal.log"
  write_missing_log_note "sanitize-build.log"
  write_missing_log_note "sanitize-unit-ctest.log"
  write_missing_log_note "sanitize-unit-internal.log"
  write_missing_log_note "sanitize-component-ctest.log"
  write_missing_log_note "sanitize-component-internal.log"
  write_missing_log_note "sanitize-cli-smoke.log"
  write_missing_log_note "static-analysis.log"
  write_missing_log_note "coverage.log"
  write_missing_log_note "coverage-gcov.log"
  echo ""
  echo "## Optional static analysis tool logs"
  echo ""
  for optional_log in static-cppcheck.log static-clang-tidy.log static-shellcheck.log static-cmake-format.log; do
    if [[ -f "$ROOT_DIR/$CI_LOG_DIR/$optional_log" ]]; then
      echo "- Found optional log: \`$CI_LOG_DIR/$optional_log\`"
    else
      echo "- Optional log not present: \`$CI_LOG_DIR/$optional_log\`"
    fi
  done
  echo ""
  echo "## Files collected"
  echo ""
  if [[ -d "$ROOT_DIR/$CI_OUT" ]]; then
    find "$ROOT_DIR/$CI_OUT" -maxdepth 4 -type f | sort | sed "s#^$ROOT_DIR/##"
  else
    echo "No CI output directory found."
  fi
  echo ""
  echo "## Last log lines"
  echo ""
  if [[ -d "$ROOT_DIR/$CI_LOG_DIR" ]]; then
    for log_file in "$ROOT_DIR/$CI_LOG_DIR"/*; do
      if [[ -f "$log_file" ]]; then
        echo "### $(basename "$log_file")"
        echo ""
        echo '```text'
        tail -n 20 "$log_file"
        echo '```'
        echo ""
      fi
    done
  else
    echo "No log directory found."
  fi
} > "$SUMMARY"

if ! command -v tar >/dev/null 2>&1; then
  echo "[CI] ERROR: tar not found. Cannot create CI log archive." >&2
  exit 1
fi

tar_inputs=(
  "$CI_LOG_DIR"
  "$CI_REPORT_DIR"
  "$CI_ARTIFACT_DIR/summary.md"
)

if [[ -f "$ROOT_DIR/$CI_ARTIFACT_DIR/coverage-summary.txt" ]]; then
  tar_inputs+=("$CI_ARTIFACT_DIR/coverage-summary.txt")
fi

if [[ -d "$ROOT_DIR/$CI_ARTIFACT_DIR/coverage-gcov" ]]; then
  tar_inputs+=("$CI_ARTIFACT_DIR/coverage-gcov")
fi

rm -f "$ARCHIVE_TMP"

tar -czf "$ARCHIVE_TMP" \
  -C "$ROOT_DIR" \
  "${tar_inputs[@]}"

mv "$ARCHIVE_TMP" "$ARCHIVE"

echo "[CI] Collected CI summary: $SUMMARY"
echo "[CI] Created CI archive: $ARCHIVE"