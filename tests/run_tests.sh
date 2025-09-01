#!/bin/bash

RUNTIME="./build/ragtime"
TESTS_DIR="tests"

RED='\033[0;31m'
GREEN='\033[0;32m'
BOLD='\033[1m'
DIMMED='\033[0;90m'
NO_COLOR='\033[0m'

PASSED_SUITES=0
FAILED_SUITES=()

START_TIME=$SECONDS

if [ ! -f "$RUNTIME" ]; then
  echo -e "${RED}Error: Runtime executable not found at $RUNTIME${NO_COLOR}"
  echo "Please build the runtime first by running: ./build.sh"
  exit 1
fi

run_test() {
  local test_script="$1"
  local expected_pattern="$2"

  "$RUNTIME" "$test_script" >/tmp/test_output.txt 2>&1

  if grep -q "$expected_pattern" /tmp/test_output.txt; then
    return 0
  else
    echo -e "    ${RED}✕ $(basename "$test_script")${NO_COLOR}"
    echo -e "      ${RED}Expected pattern not found: \"${expected_pattern}\"${NO_COLOR}"
    echo -e "      ${DIMMED}Received:${NO_COLOR}"
    cat /tmp/test_output.txt | head -10 | sed 's/^/      /'
    return 1
  fi
}

print_suite() {
  local suite_file="$1"
  local suite_status="$2"

  if [ "$suite_status" = "PASS" ]; then
    local dir=$(dirname "$suite_file")
    local file=$(basename "$suite_file")
    echo -e "${BOLD}${GREEN}PASS${NO_COLOR} ${DIMMED}${dir}/${NO_COLOR}${BOLD}${file}${NO_COLOR}"
  else
    local dir=$(dirname "$suite_file")
    local file=$(basename "$suite_file")
    echo -e "${BOLD}${RED}FAIL${NO_COLOR} ${DIMMED}${dir}/${NO_COLOR}${BOLD}${file}${NO_COLOR}"
    FAILED_SUITES+=("$suite_file")
  fi
}

run_suite() {
  local suite_file="$1"
  local expected_pattern="$2"

  local test_output=$(mktemp)
  run_test "$TESTS_DIR/$suite_file" "$expected_pattern" >"$test_output" 2>&1
  local test_result=$?

  if [ $test_result -eq 0 ]; then
    print_suite "$TESTS_DIR/$suite_file" "PASS"
    PASSED_SUITES=$((PASSED_SUITES + 1))
  else
    print_suite "$TESTS_DIR/$suite_file" "FAIL"
  fi

  cat "$test_output"
  rm -f "$test_output"
}

print_summary() {
  local total_suites=$((PASSED_SUITES + ${#FAILED_SUITES[@]}))

  if [ ${#FAILED_SUITES[@]} -gt 0 ]; then
    echo -e "${BOLD}${RED}Suites: ${#FAILED_SUITES[@]} failed${NO_COLOR}, ${PASSED_SUITES} passed, ${total_suites} total"
  else
    echo -e "${BOLD}Suites: ${GREEN}${total_suites} passed${NO_COLOR}, ${total_suites} total"
  fi

  echo -e "${BOLD}Time:   ${TOTAL_TIME}s${NO_COLOR}"

  if [ ${#FAILED_SUITES[@]} -eq 0 ]; then
    exit 0
  else
    echo
    echo -e "${BOLD}${RED}FAIL${NO_COLOR} Failed test suites:"
    for suite in "${FAILED_SUITES[@]}"; do
      echo -e "  ${RED}● ${suite}${NO_COLOR}"
    done
    exit 1
  fi
}

for test_file in "$TESTS_DIR"/*.test.js; do
  if [ -f "$test_file" ]; then
    test_name=$(basename "$test_file")
    run_suite "$test_name" "All tests passed"
  fi
done

TOTAL_TIME=$((SECONDS - START_TIME))

echo
print_summary
