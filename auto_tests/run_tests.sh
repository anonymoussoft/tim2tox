#!/bin/bash
# Run all tim2tox auto tests
# Usage: ./run_tests.sh [test_file_pattern]

set -e

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
TIM2TOX_DIR="$PROJECT_ROOT/tim2tox"
cd "$SCRIPT_DIR"

# Build library if needed (smart build - only rebuilds if necessary)
if [[ -f "$TIM2TOX_DIR/build_ffi.sh" ]]; then
    echo -e "${YELLOW}Checking if library needs to be built...${NC}"
    "$TIM2TOX_DIR/build_ffi.sh"
else
    echo -e "${YELLOW}Warning: build_ffi.sh not found. Skipping automatic build.${NC}"
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Running tim2tox auto tests...${NC}"

# Check if Flutter is installed
if ! command -v flutter &> /dev/null; then
    echo -e "${RED}Error: Flutter is not installed or not in PATH${NC}"
    exit 1
fi

# Get Flutter version
FLUTTER_VERSION=$(flutter --version | head -n 1)
echo -e "${YELLOW}Flutter: $FLUTTER_VERSION${NC}"

# Get Dart version
DART_VERSION=$(dart --version 2>&1 | head -n 1)
echo -e "${YELLOW}Dart: $DART_VERSION${NC}"

# Install dependencies
echo -e "${GREEN}Installing dependencies...${NC}"
flutter pub get

# Function to parse and display test case timings from Flutter test output
parse_test_timings() {
    local output_file=$1
    if [ ! -f "$output_file" ]; then
        return
    fi
    
    # Extract test lines and parse timing
    # Format: "00:01 +4: Test Group test name"
    local temp_file="/tmp/test_timings_$$.txt"
    
    # Extract test lines, convert time to seconds
    grep -E "^\s*[0-9]+:[0-9]+\s+\+[0-9]+:" "$output_file" 2>/dev/null | \
    grep -v "All tests passed" | \
    grep -vE "\(setUpAll\)|\(tearDownAll\)|\(setUp\)|\(tearDown\)" | \
    while IFS= read -r line; do
        # Extract time (MM:SS)
        local time_part=$(echo "$line" | sed -E 's/^\s*([0-9]+):([0-9]+).*/\1:\2/')
        if [ "$time_part" != "$line" ]; then
            local minutes=$(echo "$time_part" | cut -d: -f1)
            local seconds=$(echo "$time_part" | cut -d: -f2)
            local time_sec=$((minutes * 60 + seconds))
            
            # Extract test name (after "+N: ")
            local test_name=$(echo "$line" | sed -E 's/.*\+[0-9]+:[[:space:]]+(.+)$/\1/' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
            
            if [ -n "$test_name" ]; then
                echo "${time_sec}|${test_name}"
            fi
        fi
    done > "$temp_file" 2>/dev/null
    
    # Process to find duplicates and calculate durations
    local prev_time=0
    local prev_name=""
    if [ -f "$temp_file" ] && [ -s "$temp_file" ]; then
        while IFS='|' read -r time_sec test_name; do
            if [ -n "$prev_name" ] && [ "$prev_name" = "$test_name" ]; then
                local duration=$((time_sec - prev_time))
                if [ $duration -gt 0 ]; then
                    echo -e "${YELLOW}  ✓ $test_name${NC} - ${GREEN}${duration}s${NC}"
                fi
            fi
            prev_time=$time_sec
            prev_name="$test_name"
        done < "$temp_file"
        
        # Handle last test using "All tests passed" line
        local final_line=$(grep "All tests passed" "$output_file" 2>/dev/null | head -1)
        if [ -n "$final_line" ] && [ -n "$prev_name" ]; then
            local final_time_part=$(echo "$final_line" | sed -E 's/^\s*([0-9]+):([0-9]+).*/\1:\2/')
            if [ "$final_time_part" != "$final_line" ]; then
                local final_minutes=$(echo "$final_time_part" | cut -d: -f1)
                local final_seconds=$(echo "$final_time_part" | cut -d: -f2)
                local final_time=$((final_minutes * 60 + final_seconds))
                local duration=$((final_time - prev_time))
                if [ $duration -gt 0 ]; then
                    echo -e "${YELLOW}  ✓ $prev_name${NC} - ${GREEN}${duration}s${NC}"
                fi
            fi
        fi
    fi
    
    rm -f "$temp_file"
}

# Run tests
if [ -z "$1" ]; then
    echo -e "${GREEN}Running all tests...${NC}"
    start_time=$(date +%s)
    output_file="/tmp/flutter_test_output_$(date +%s).log"
    flutter test 2>&1 | tee "$output_file"
    exit_code=${PIPESTATUS[0]}
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    echo ""
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}Test Case Timings:${NC}"
    parse_test_timings "$output_file"
    echo ""
    echo -e "${GREEN}Total duration: ${duration}s${NC}"
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    rm -f "$output_file"
    exit $exit_code
else
    echo -e "${GREEN}Running tests matching pattern: $1${NC}"
    start_time=$(date +%s)
    output_file="/tmp/flutter_test_output_$(date +%s).log"
    flutter test --name "$1" 2>&1 | tee "$output_file"
    exit_code=${PIPESTATUS[0]}
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    echo ""
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}Test Case Timings:${NC}"
    parse_test_timings "$output_file"
    echo ""
    echo -e "${GREEN}Total duration: ${duration}s${NC}"
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    rm -f "$output_file"
    exit $exit_code
fi
