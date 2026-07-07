#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/dependencies.sh"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color (Reset)

# Counter for missing packages
MISSING_COUNT=0

echo "Checking project dependencies..."
echo "--------------------------------------------------------"

for pkg in "${DEPENDENCIES[@]}"; do
    # brew list > /dev/null hides the output, -q keeps it quiet
    # We check the exit code ($?) of the brew command
    if brew list "$pkg" &> /dev/null; then
        echo -e "${GREEN}$pkg is installed.${NC}"
    else
        echo -e "${RED}$pkg is NOT installed.${NC}"
        ((MISSING_COUNT++))
    fi
done

echo "--------------------------------------------------------"

# Final status and exit codes
if [ "$MISSING_COUNT" -eq 0 ]; then
    echo -e "${GREEN}All dependencies are satisfied!${NC}"
    exit 0
else
    echo -e "${YELLOW}$MISSING_COUNT dependency/dependencies are missing.${NC}"
    echo -e "${YELLOW}Run the environment setup script to fix this.${NC}"
    echo -e "${YELLOW}To check if brew is installed, run 'brew --version'.${NC}"
    exit 1
fi