#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/dependencies.sh"

echo "Checking for Homebrew..."
if ! command -v brew &> /dev/null; then
    echo "Homebrew not found. Installing Homebrew first..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

echo "Updating Homebrew..."
brew update

echo "Installing Core Libraries & Build Tools..."
# Note: llvm installs both clang-format and clang-tidy
brew install "${DEPENDENCIES[@]}"

echo "--------------------------------------------------------"
echo "Installation complete!"