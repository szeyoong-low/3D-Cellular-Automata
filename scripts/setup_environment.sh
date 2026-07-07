#!/bin/bash

GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color (Reset)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/dependencies.sh"

echo "Checking for Homebrew..."
if ! command -v brew &> /dev/null; then
    echo -e "${YELLOW}Homebrew not found. Installing Homebrew first...${NC}"
    NONINTERACTIVE=1 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# Configure Homebrew in PATH for the current script
if [[ -x /home/linuxbrew/.linuxbrew/bin/brew ]]; then
    eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"
elif [[ -x /opt/homebrew/bin/brew ]]; then
    eval "$(/opt/homebrew/bin/brew shellenv)"
elif [[ -x /usr/local/bin/brew ]]; then
    eval "$(/usr/local/bin/brew shellenv)"
fi

# Add it to .bashrc for future interactive shells on a new Linux machine
if ! grep -q "brew shellenv" ~/.bashrc 2>/dev/null; then
    echo 'eval "$('$(command -v brew)' shellenv)"' >> ~/.bashrc
fi

# If running in GitHub Actions, add it to GITHUB_PATH for subsequent steps
if [[ -n "${GITHUB_PATH}" ]]; then
    dirname "$(command -v brew)" >> "$GITHUB_PATH"
fi

echo "Updating Homebrew..."
brew update

echo "Installing Core Libraries & Build Tools..."
# Note: llvm installs both clang-format and clang-tidy
brew install "${DEPENDENCIES[@]}"

echo "--------------------------------------------------------"
echo -e "${GREEN}Installation complete!${NC}"