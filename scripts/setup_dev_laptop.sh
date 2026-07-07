#!/bin/bash

GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color (Reset)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "========================================"
echo "Starting Developer Laptop Setup..."
echo "========================================"

# Install dependencies using the shared script
echo -e "\n${YELLOW}Step 1: Installing dependencies...${NC}"
bash "$SCRIPT_DIR/setup_environment.sh"

# Configure Git to use the shared hooks directory
echo -e "\n${YELLOW}Step 2: Configuring Git hooks...${NC}"
cd "$PROJECT_ROOT" || exit 1
git config core.hooksPath .githooks
echo -e "${GREEN}Git hooks configured to use .githooks directory.${NC}"

# Ensure the hooks are executable
chmod +x .githooks/*

echo "========================================"
echo -e "${GREEN}Developer setup complete! You are ready to contribute.${NC}"
echo "========================================"
