#!/bin/bash
# Run tim2tox auto tests with verbose output
# Usage: ./run_tests_verbose.sh [test_file]

set -e

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Running tim2tox auto tests with verbose output...${NC}"

# Check if Flutter is installed
if ! command -v flutter &> /dev/null; then
    echo -e "${RED}Error: Flutter is not installed or not in PATH${NC}"
    exit 1
fi

# Install dependencies
echo -e "${GREEN}Installing dependencies...${NC}"
flutter pub get

# Run tests with verbose output
if [ -z "$1" ]; then
    echo -e "${GREEN}Running all tests with verbose output...${NC}"
    flutter test --reporter expanded
else
    echo -e "${GREEN}Running test file: $1${NC}"
    flutter test "$1" --reporter expanded
fi

echo -e "${GREEN}Tests completed!${NC}"
