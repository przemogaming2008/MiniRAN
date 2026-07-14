ROOT_DIR="$(pwd)"
mkdir -p "$ROOT_DIR/$CI_LOG_DIR" "$ROOT_DIR/$CI_REPORT_DIR"

ctest --test-dir "$ROOT_DIR/$BUILD_DIR" \
  -C "${CTEST_CONFIG:-Debug}" \
  -L component \
  --output-on-failure \
  --output-log "$ROOT_DIR/$CI_LOG_DIR/component-ctest.log" \
  --output-junit "$ROOT_DIR/$CI_REPORT_DIR/component-ctest.xml"
