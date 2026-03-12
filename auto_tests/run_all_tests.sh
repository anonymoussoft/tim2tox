#!/bin/bash
# Compatibility wrapper: run all tests via run_tests_ordered.sh
# Usage:
#   ./run_all_tests.sh
#   ./run_all_tests.sh 5,6
#   ./run_all_tests.sh 7-9 --merge-output

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -x "./run_tests_ordered.sh" ]; then
  echo "Error: ./run_tests_ordered.sh not found or not executable"
  exit 1
fi

exec ./run_tests_ordered.sh "$@"
