#!/bin/bash
# Guardrail checks for test assertion anti-patterns.
# Usage: ./check_test_assertions.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

TARGETS=(
  "test/scenarios"
  "test/scenarios_binary"
  "test/unit_tests"
)

FAILED=0

run_check() {
  local name="$1"
  local pattern="$2"

  local hits
  hits=$(rg -n --pcre2 "$pattern" "${TARGETS[@]}" || true)
  if [ -n "$hits" ]; then
    echo "[FAIL] $name"
    echo "$hits"
    echo ""
    FAILED=1
  else
    echo "[PASS] $name"
  fi
}

echo "Running assertion guard checks..."

# Tautology: expect(... || true, ...)
run_check "No tautological '|| true' in expect" \
  "expect\\([^;\\n]*\\|\\|\\s*true"

# Tautology: expect(x == 0 || x != 0, isTrue)
run_check "No tautological '==0 || !=0' checks in expect" \
  "expect\\([^;\\n]*==\\s*0\\s*\\|\\|\\s*[^;\\n]*!=\\s*0"

# Tautology: expect(x != 0 || x == 0, isTrue)
run_check "No tautological '!=0 || ==0' checks in expect" \
  "expect\\([^;\\n]*!=\\s*0\\s*\\|\\|\\s*[^;\\n]*==\\s*0"

# Empty catch blocks swallow failures.
run_check "No empty catch blocks" \
  "catch\\s*\\([^\\)]*\\)\\s*\\{\\s*\\}"

# Empty catchError handlers swallow failures.
run_check "No empty catchError handlers" \
  "catchError\\(\\s*\\([^\\)]*\\)\\s*\\{\\s*\\}\\s*\\)"

if [ "$FAILED" -ne 0 ]; then
  echo ""
  echo "Assertion guard checks failed."
  exit 1
fi

echo ""
echo "All assertion guard checks passed."
