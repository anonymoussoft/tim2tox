#!/bin/bash
# Merge phase 1–12 test results into merged_results.log and merged_summary.txt
# Usage: ./merge_phase_results.sh
# Run after all phase agents have completed (phase1-12_results.log and phaseN_summary.txt exist)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MERGED_DIR="$SCRIPT_DIR/merged_results"
MERGED_LOG="$MERGED_DIR/merged_results.log"
MERGED_SUMMARY="$MERGED_DIR/merged_summary.txt"

mkdir -p "$MERGED_DIR"

# Phase names for display
PHASE_NAMES=(
  "Phase 1: Basic Tests"
  "Phase 2: Friendship Tests"
  "Phase 3: Message Tests"
  "Phase 4: Group Tests"
  "Phase 5: ToxAV Tests"
  "Phase 6: Profile Tests"
  "Phase 7: Conversation Tests"
  "Phase 8: File Tests"
  "Phase 9: Conference Tests"
  "Phase 10: Group Extended Tests"
  "Phase 11: Network Tests"
  "Phase 12: Other Tests"
)

# Initialize totals
TOTAL_TESTS=0
TOTAL_PASSED=0
TOTAL_FAILED=0
TOTAL_SKIPPED=0
declare -a FAILED_ENTRIES  # "phase_num|test_name" for grouping by phase

# 1. Concatenate phase results into merged_results.log
: > "$MERGED_LOG"
for n in {1..12}; do
  phase_log="$MERGED_DIR/phase${n}_results.log"
  if [ -f "$phase_log" ]; then
    echo "" >> "$MERGED_LOG"
    echo "========== ${PHASE_NAMES[$((n-1))]} ==========" >> "$MERGED_LOG"
    cat "$phase_log" >> "$MERGED_LOG"
  fi
done

# 2. Parse each phase summary and aggregate
for n in {1..12}; do
  phase_summary="$MERGED_DIR/phase${n}_summary.txt"
  if [ ! -f "$phase_summary" ]; then
    continue
  fi

  # Extract Total Tests, Passed, Failed, Skipped
  phase_total=$(grep -E "^Total Tests:" "$phase_summary" | sed 's/Total Tests:[[:space:]]*//' || echo "0")
  phase_passed=$(grep -E "^Passed:" "$phase_summary" | sed 's/Passed:[[:space:]]*//' || echo "0")
  phase_failed=$(grep -E "^Failed:" "$phase_summary" | sed 's/Failed:[[:space:]]*//' || echo "0")
  phase_skipped=$(grep -E "^Skipped:" "$phase_summary" | sed 's/Skipped:[[:space:]]*//' || echo "0")

  TOTAL_TESTS=$((TOTAL_TESTS + phase_total))
  TOTAL_PASSED=$((TOTAL_PASSED + phase_passed))
  TOTAL_FAILED=$((TOTAL_FAILED + phase_failed))
  TOTAL_SKIPPED=$((TOTAL_SKIPPED + phase_skipped))

  # Extract failed test names (lines after "Failed Tests:" starting with "  - ")
  in_failed=0
  while IFS= read -r line; do
    if [[ "$line" =~ ^Failed[[:space:]]+Tests: ]]; then
      in_failed=1
      continue
    fi
    if [ "$in_failed" -eq 1 ]; then
      # Stop at empty line or next section (e.g. "Skipped Tests:")
      if [[ "$line" =~ ^[[:space:]]*$ ]] || [[ "$line" =~ ^Skipped[[:space:]]+Tests: ]]; then
        break
      fi
      if [[ "$line" =~ ^[[:space:]]*-[[:space:]]+(.+)$ ]]; then
        test_name="${BASH_REMATCH[1]}"
        FAILED_ENTRIES+=("$n|$test_name")
      fi
    fi
  done < "$phase_summary"
done

# 3. Write merged_summary.txt
{
  echo "═══════════════════════════════════════════════════════════"
  echo "  Test Execution Summary (Merged from Phase 1–12)"
  echo "═══════════════════════════════════════════════════════════"
  echo ""
  echo "Execution Date: $(date) (phases run in parallel)"
  echo "Total Tests: $TOTAL_TESTS"
  echo "Passed: $TOTAL_PASSED"
  echo "Failed: $TOTAL_FAILED"
  echo "Skipped: $TOTAL_SKIPPED"
  echo ""

  if [ ${#FAILED_ENTRIES[@]} -gt 0 ]; then
    echo "Failed Tests (with Phase):"
    for n in 1 2 3 4 5 6 7 8 9 10 11 12; do
      phase_tests=""
      for entry in "${FAILED_ENTRIES[@]}"; do
        phase_num="${entry%%|*}"
        test_name="${entry#*|}"
        if [ "$phase_num" = "$n" ]; then
          if [ -n "$phase_tests" ]; then
            phase_tests="$phase_tests, $test_name"
          else
            phase_tests="$test_name"
          fi
        fi
      done
      if [ -n "$phase_tests" ]; then
        echo "  Phase $n: $phase_tests"
      fi
    done
    echo ""
  fi

  echo "═══════════════════════════════════════════════════════════"
} > "$MERGED_SUMMARY"

echo "Merged results written to: $MERGED_LOG"
echo "Merged summary written to: $MERGED_SUMMARY"
echo ""
cat "$MERGED_SUMMARY"

# Exit with failure if any tests failed
if [ "$TOTAL_FAILED" -gt 0 ]; then
  exit 1
fi
exit 0
