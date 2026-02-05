#!/bin/bash

# LoRa Build Test Script
# Tests all LoRa project environments and reports only failures

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track results
PASSED=0
FAILED=0
FAILED_BUILDS=()

echo "============================================"
echo "  LoRa Projects Build Test"
echo "============================================"
echo ""

# Function to build and check result
build_env() {
    local project_dir="$1"
    local env_name="$2"
    local project_name=$(basename "$project_dir")

    printf "Building %-20s %-25s ... " "$project_name" "[$env_name]"

    # Capture output and exit code
    local output
    local exit_code=0
    output=$(cd "$project_dir" && pio run -e "$env_name" 2>&1) || exit_code=$?

    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
        FAILED_BUILDS+=("$project_name:$env_name")
        # Store output for later display
        echo "$output" > "/tmp/build_fail_${project_name}_${env_name}.log"
    fi
}

echo "--- sender-lora ---"
build_env "$SCRIPT_DIR/sender-lora" "esp32dev_ra02"
build_env "$SCRIPT_DIR/sender-lora" "esp32dev_sx1262"
build_env "$SCRIPT_DIR/sender-lora" "uno_ra02"

echo ""
echo "--- receiver-lora ---"
build_env "$SCRIPT_DIR/receiver-lora" "esp32dev_ra02"
build_env "$SCRIPT_DIR/receiver-lora" "esp32dev_sx1262"
build_env "$SCRIPT_DIR/receiver-lora" "uno_ra02"

echo ""
echo "--- multisender-lora ---"
build_env "$SCRIPT_DIR/multisender-lora" "esp32dev_dual_ra02"
build_env "$SCRIPT_DIR/multisender-lora" "esp32dev_dual_sx1262"

echo ""
echo "============================================"
echo "  Results"
echo "============================================"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "============================================"
    echo -e "  ${RED}Failed Builds${NC}"
    echo "============================================"
    for build in "${FAILED_BUILDS[@]}"; do
        project=$(echo "$build" | cut -d: -f1)
        env=$(echo "$build" | cut -d: -f2)
        echo ""
        echo -e "${YELLOW}>>> $project [$env]${NC}"
        echo "-------------------------------------------"
        cat "/tmp/build_fail_${project}_${env}.log" | tail -50
        rm -f "/tmp/build_fail_${project}_${env}.log"
    done
    echo ""
    exit 1
else
    echo ""
    echo -e "${GREEN}All builds passed!${NC}"
    exit 0
fi
