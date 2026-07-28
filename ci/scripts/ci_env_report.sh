#!/usr/bin/env bash
set -Eeuo pipefail

: "${CI_LOG_DIR:=ci_out/logs}"
: "${MIN_CMAKE_VERSION:=3.21.0}"
: "${MIN_CTEST_VERSION:=3.21.0}"
: "${MIN_FREE_SPACE_KB:=524288}"

ROOT_DIR="$(pwd)"
mkdir -p "$ROOT_DIR/$CI_LOG_DIR"

LOG="$ROOT_DIR/$CI_LOG_DIR/env.txt"
missing=0

{
  echo "MiniRAN CI environment report"
  echo "Date: $(date -Iseconds)"
  echo "Job: ${JOB_NAME:-local}"
  echo "Build: ${BUILD_NUMBER:-0}"
  echo "Branch: ${BRANCH_NAME:-unknown}"
  echo "Commit: ${GIT_COMMIT:-unknown}"
  echo "Workspace: ${WORKSPACE:-$ROOT_DIR}"
  echo "Root: $ROOT_DIR"
  echo ""
} | tee "$LOG"

fail_check() {
  echo "[CI] ERROR: $*" | tee -a "$LOG"
  missing=1
}

pass_check() {
  echo "[CI] OK: $*" | tee -a "$LOG"
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

check_tool() {
  local tool="$1"

  if ! command -v "$tool" >/dev/null 2>&1; then
    fail_check "required tool not found: $tool"
    return
  fi

  {
    echo ""
    echo "[$tool]"
    "$tool" --version
  } | tee -a "$LOG"

  pass_check "found tool: $tool"
}

check_tool_min_version() {
  local tool="$1"
  local required="$2"

  if ! command -v "$tool" >/dev/null 2>&1; then
    fail_check "required tool not found: $tool"
    return
  fi

  local actual
  actual="$(tool_version "$tool" || true)"

  if [[ -z "$actual" ]]; then
    fail_check "cannot detect $tool version"
    return
  fi

  {
    echo ""
    echo "[$tool]"
    "$tool" --version
  } | tee -a "$LOG"

  if version_at_least "$actual" "$required"; then
    pass_check "$tool version $actual satisfies minimum $required"
  else
    fail_check "$tool version $actual is too old. Required: $required or newer"
  fi
}

check_repo_root() {
  echo "" | tee -a "$LOG"
  echo "[repo-root]" | tee -a "$LOG"

  if [[ ! -f "$ROOT_DIR/CMakeLists.txt" ]]; then
    fail_check "current directory is not repository root: missing CMakeLists.txt"
  fi

  if [[ ! -d "$ROOT_DIR/src" ]]; then
    fail_check "current directory is not repository root: missing src/"
  fi

  if [[ ! -d "$ROOT_DIR/ci/scripts" ]]; then
    fail_check "current directory is not repository root: missing ci/scripts/"
  fi

  if [[ -f "$ROOT_DIR/CMakeLists.txt" && -d "$ROOT_DIR/src" && -d "$ROOT_DIR/ci/scripts" ]]; then
    pass_check "current directory looks like repository root"
  fi
}

check_required_files() {
  echo "" | tee -a "$LOG"
  echo "[required-files]" | tee -a "$LOG"

  local files=(
    "CMakeLists.txt"
    "Jenkinsfile"
    "ci/scripts/ci_env_report.sh"
    "ci/scripts/ci_build.sh"
    "ci/scripts/ci_test_unit.sh"
    "ci/scripts/ci_test_component.sh"
    "ci/scripts/ci_run_cli_scenarios.sh"
    "ci/scripts/ci_mega_gate.sh"
    "ci/scripts/ci_collect_logs.sh"
    "src/main.cpp"
    "scenarios/tcp_basic.cfg"
    "scenarios/udp_lossy.cfg"
  )

  for file in "${files[@]}"; do
    if [[ ! -f "$ROOT_DIR/$file" ]]; then
      fail_check "required file missing: $file"
    fi
  done

  pass_check "required file check completed"
}

check_write_access() {
  echo "" | tee -a "$LOG"
  echo "[write-access]" | tee -a "$LOG"

  local probe_dir="$ROOT_DIR/ci_out/preflight"
  local probe_file="$probe_dir/write-test.tmp"

  mkdir -p "$probe_dir"

  if echo "write-test" > "$probe_file"; then
    rm -f "$probe_file"
    pass_check "workspace is writable"
  else
    fail_check "workspace is not writable: $ROOT_DIR"
  fi
}

check_free_space() {
  echo "" | tee -a "$LOG"
  echo "[disk-space]" | tee -a "$LOG"

  if ! command -v df >/dev/null 2>&1; then
    fail_check "df not found, cannot check free disk space"
    return
  fi

  local free_kb
  free_kb="$(df -Pk "$ROOT_DIR" | awk 'NR==2 {print $4}')"

  if [[ -z "$free_kb" ]]; then
    fail_check "cannot determine free disk space"
    return
  fi

  echo "Free space [KB]: $free_kb" | tee -a "$LOG"

  if (( free_kb < MIN_FREE_SPACE_KB )); then
    fail_check "not enough free disk space. Required at least ${MIN_FREE_SPACE_KB} KB"
  else
    pass_check "free disk space is sufficient"
  fi
}

check_cpp_toolchain() {
  echo "" | tee -a "$LOG"
  echo "[cpp-toolchain]" | tee -a "$LOG"

  if command -v c++ >/dev/null 2>&1; then
    c++ --version | tee -a "$LOG"
    pass_check "C++ compiler found: c++"
    return
  fi

  if command -v g++ >/dev/null 2>&1; then
    g++ --version | tee -a "$LOG"
    pass_check "C++ compiler found: g++"
    return
  fi

  if command -v clang++ >/dev/null 2>&1; then
    clang++ --version | tee -a "$LOG"
    pass_check "C++ compiler found: clang++"
    return
  fi

  if command -v cl >/dev/null 2>&1; then
    cl 2>&1 | head -n 5 | tee -a "$LOG"
    pass_check "C++ compiler found: cl"
    return
  fi

  local vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
  if [[ -x "$vswhere" ]]; then
    local vs_installation
    vs_installation="$("$vswhere" -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath 2>/dev/null || true)"

    if [[ -n "$vs_installation" ]]; then
      echo "Visual Studio installation: $vs_installation" | tee -a "$LOG"
      pass_check "Visual Studio/MSBuild toolchain detected"
      return
    fi
  fi

  fail_check "no C++ compiler/toolchain detected"
}

{
  echo "## Tool checks"
} | tee -a "$LOG"

check_repo_root
check_required_files
check_write_access
check_free_space

check_tool bash
check_tool git
check_tool tar
check_tool gzip
check_tool_min_version cmake "$MIN_CMAKE_VERSION"
check_tool_min_version ctest "$MIN_CTEST_VERSION"
check_cpp_toolchain

echo "" | tee -a "$LOG"

if [[ "$missing" -ne 0 ]]; then
  echo "[CI] Preflight failed." | tee -a "$LOG"
  exit 1
fi

echo "[CI] Preflight passed." | tee -a "$LOG"