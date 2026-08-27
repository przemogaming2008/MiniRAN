#!/usr/bin/env bash
set -Eeuo pipefail

: "${CI_LOG_DIR:=ci_out/logs}"
: "${MIN_CMAKE_VERSION:=3.21.0}"
: "${MIN_CTEST_VERSION:=3.21.0}"
: "${MIN_FREE_SPACE_KB:=524288}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
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

  if [[ ! -d "$ROOT_DIR/include" ]]; then
    fail_check "current directory is not repository root: missing include/"
  fi

  if [[ ! -d "$ROOT_DIR/tests" ]]; then
    fail_check "current directory is not repository root: missing tests/"
  fi

  if [[ ! -d "$ROOT_DIR/ci/scripts" ]]; then
    fail_check "current directory is not repository root: missing ci/scripts/"
  fi

  if [[ -f "$ROOT_DIR/CMakeLists.txt" &&
        -d "$ROOT_DIR/src" &&
        -d "$ROOT_DIR/include" &&
        -d "$ROOT_DIR/tests" &&
        -d "$ROOT_DIR/ci/scripts" ]]; then
    pass_check "current directory looks like repository root"
  fi
}

check_file_exists() {
  local file="$1"

  if [[ ! -f "$ROOT_DIR/$file" ]]; then
    fail_check "required file missing: $file"
  else
    pass_check "required file exists: $file"
  fi
}

check_required_files() {
  echo "" | tee -a "$LOG"
  echo "[required-files]" | tee -a "$LOG"

  local files=(
    "CMakeLists.txt"
    "Jenkinsfile"
    "README.md"
    "docs/CI_RUNBOOK.md"
    "ci/README_CI.md"
    "ci/jenkins/README_LOG_RECORDER.md"

    "ci/scripts/ci_env_report.sh"
    "ci/scripts/ci_build.sh"
    "ci/scripts/ci_test_unit.sh"
    "ci/scripts/ci_test_component.sh"
    "ci/scripts/ci_run_cli_scenarios.sh"
    "ci/scripts/ci_mega_gate.sh"
    "ci/scripts/ci_test_sanitizers.sh"
    "ci/scripts/ci_static_analysis.sh"
    "ci/scripts/ci_collect_logs.sh"

    "src/main.cpp"
    "src/simulation/scenario_runner.cpp"
    "src/simulation/simulation_result.cpp"
    "src/traffic/traffic_generator.cpp"
    "src/nodes/ue.cpp"

    "include/miniran/simulation/scenario_config.h"
    "include/miniran/simulation/simulation_result.h"
    "include/miniran/traffic/traffic_generator.h"
    "include/miniran/nodes/ue.h"

    "tests/test_main.cpp"
    "tests/component/test_ci_mega_gate.cpp"
    "tests/component/test_udp_low_bandwidth_limits_throughput.cpp"
    "tests/component/test_tcp_traffic.cpp"
    "tests/unit/test_traffic_generator.cpp"

    "scenarios/tcp_basic.cfg"
    "scenarios/udp_lossy.cfg"
    "scenarios/ci_tcp_heavy.cfg"
    "scenarios/ci_udp_loss_15.cfg"
    "scenarios/ci_low_bandwidth.cfg"
  )

  for file in "${files[@]}"; do
    check_file_exists "$file"
  done

  pass_check "required file check completed"
}

check_script_executable_bits() {
  echo "" | tee -a "$LOG"
  echo "[script-executable-bits]" | tee -a "$LOG"

  local scripts=(
    "ci/scripts/ci_env_report.sh"
    "ci/scripts/ci_build.sh"
    "ci/scripts/ci_test_unit.sh"
    "ci/scripts/ci_test_component.sh"
    "ci/scripts/ci_run_cli_scenarios.sh"
    "ci/scripts/ci_mega_gate.sh"
    "ci/scripts/ci_test_sanitizers.sh"
    "ci/scripts/ci_static_analysis.sh"
    "ci/scripts/ci_collect_logs.sh"
  )

  for script in "${scripts[@]}"; do
    if [[ ! -f "$ROOT_DIR/$script" ]]; then
      fail_check "script missing, cannot check executable bit: $script"
      continue
    fi

    if [[ -x "$ROOT_DIR/$script" ]]; then
      pass_check "script is executable in filesystem: $script"
    else
      echo "[CI] WARNING: script is not executable in filesystem: $script" | tee -a "$LOG"
      echo "[CI] Jenkins can still run it through bash, but direct ./script usage may fail." | tee -a "$LOG"
    fi

    if command -v git >/dev/null 2>&1 && [[ -d "$ROOT_DIR/.git" ]]; then
      local git_mode
      git_mode="$(git -C "$ROOT_DIR" ls-files --stage -- "$script" | awk '{print $1}' || true)"

      if [[ "$git_mode" == "100755" ]]; then
        pass_check "script executable bit is tracked by Git: $script"
      else
        fail_check "script executable bit is not tracked by Git: $script"
      fi
    fi
  done
}

check_scenarios() {
  echo "" | tee -a "$LOG"
  echo "[scenarios]" | tee -a "$LOG"

  if [[ ! -d "$ROOT_DIR/scenarios" ]]; then
    fail_check "scenarios directory missing"
    return
  fi

  local scenario_count
  scenario_count="$(find "$ROOT_DIR/scenarios" -maxdepth 1 -type f -name "*.cfg" | wc -l | tr -d ' ')"

  echo "Scenario count: $scenario_count" | tee -a "$LOG"

  if [[ "$scenario_count" -lt 5 ]]; then
    fail_check "expected at least 5 scenario cfg files"
  else
    pass_check "scenario cfg files found"
  fi

  local scenario
  while IFS= read -r scenario; do
    echo "Scenario: ${scenario#$ROOT_DIR/}" | tee -a "$LOG"

    if ! grep -q '^scenario_name=' "$scenario"; then
      fail_check "scenario missing scenario_name: ${scenario#$ROOT_DIR/}"
    fi

    if ! grep -q '^transport_mode=' "$scenario"; then
      fail_check "scenario missing transport_mode: ${scenario#$ROOT_DIR/}"
    fi

    if ! grep -q '^traffic_pattern=' "$scenario"; then
      fail_check "scenario missing traffic_pattern: ${scenario#$ROOT_DIR/}"
    fi
  done < <(find "$ROOT_DIR/scenarios" -maxdepth 1 -type f -name "*.cfg" | sort)
}

check_cmake_contract() {
  echo "" | tee -a "$LOG"
  echo "[cmake-contract]" | tee -a "$LOG"

  local cmake_file="$ROOT_DIR/CMakeLists.txt"

  if [[ ! -f "$cmake_file" ]]; then
    fail_check "cannot check CMake contract: CMakeLists.txt missing"
    return
  fi

  local required_patterns=(
    "MINIRAN_BUILD_TESTS"
    "MINIRAN_ENABLE_SANITIZERS"
    "miniran_unit_tests"
    "miniran_component_tests"
    "miniran_mega_tests"
    "miniran_tests"
    "unit_tests"
    "component_tests"
    "mega_gate_tests"
    "all_tests"
    "LABELS \"unit\""
    "LABELS \"component\""
    "LABELS \"mega\""
    "LABELS \"all\""
  )

  local pattern
  for pattern in "${required_patterns[@]}"; do
    if grep -q "$pattern" "$cmake_file"; then
      pass_check "CMake contract contains: $pattern"
    else
      fail_check "CMake contract missing: $pattern"
    fi
  done
}

check_jenkins_contract() {
  echo "" | tee -a "$LOG"
  echo "[jenkins-contract]" | tee -a "$LOG"

  local jenkinsfile="$ROOT_DIR/Jenkinsfile"

  if [[ ! -f "$jenkinsfile" ]]; then
    fail_check "cannot check Jenkins contract: Jenkinsfile missing"
    return
  fi

  local required_patterns=(
    "agent any"
    "ci_env_report.sh"
    "ci_build.sh"
    "ci_test_unit.sh"
    "ci_test_component.sh"
    "ci_run_cli_scenarios.sh"
    "ci_mega_gate.sh"
    "ci_test_sanitizers.sh"
    "ci_static_analysis.sh"
    "ci_collect_logs.sh"
    "Static analysis"
    "archiveArtifacts"
    "junit"
  )

  local pattern
  for pattern in "${required_patterns[@]}"; do
    if grep -q "$pattern" "$jenkinsfile"; then
      pass_check "Jenkinsfile contains: $pattern"
    else
      fail_check "Jenkinsfile missing: $pattern"
    fi
  done
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

check_msvc_asan_runtime() {
  echo "" | tee -a "$LOG"
  echo "[msvc-asan-runtime]" | tee -a "$LOG"

  case "${OSTYPE:-}" in
    msys*|cygwin*|win32*) ;;
    *)
      pass_check "not a Windows shell, skipping MSVC ASan runtime check"
      return
      ;;
  esac

  local dll_name="clang_rt.asan_dynamic-x86_64.dll"
  local dll_path=""

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

  if [[ -n "$dll_path" ]]; then
    echo "MSVC ASan runtime: $dll_path" | tee -a "$LOG"
    pass_check "MSVC ASan runtime found"
  else
    echo "[CI] WARNING: MSVC ASan runtime not found." | tee -a "$LOG"
    echo "[CI] Sanitizer stage may still work with GCC/Clang, but MSVC ASan will fail without this runtime." | tee -a "$LOG"
  fi
}

{
  echo "## Tool checks"
} | tee -a "$LOG"

check_repo_root
check_required_files
check_script_executable_bits
check_scenarios
check_cmake_contract
check_jenkins_contract
check_write_access
check_free_space

check_tool bash
check_tool git
check_tool tar
check_tool gzip
check_tool_min_version cmake "$MIN_CMAKE_VERSION"
check_tool_min_version ctest "$MIN_CTEST_VERSION"
check_cpp_toolchain
check_msvc_asan_runtime

echo "" | tee -a "$LOG"

if [[ "$missing" -ne 0 ]]; then
  echo "[CI] Preflight failed." | tee -a "$LOG"
  exit 1
fi

echo "[CI] Preflight passed." | tee -a "$LOG"