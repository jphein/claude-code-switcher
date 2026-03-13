#!/bin/bash
# Install cc (Claude Code switcher) to ~/bin

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${HOME}/bin"

# Create ~/bin if needed
mkdir -p "$BIN_DIR"

# Symlink executables
for cmd in cc cc-test cc-api-test; do
    target="$BIN_DIR/$cmd"
    if [ -L "$target" ] || [ -e "$target" ]; then
        echo "Removing existing $target"
        rm -f "$target"
    fi
    ln -s "$SCRIPT_DIR/$cmd" "$target"
    echo "Linked $cmd -> $target"
done

# Check if ~/bin is in PATH
if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo ""
    echo "Add ~/bin to your PATH by adding this to ~/.bashrc or ~/.zshrc:"
    echo ""
    echo '  export PATH="$HOME/bin:$PATH"'
    echo ""
fi

echo ""
echo "Done! Next steps:"
echo "  cc status         Verify installation"
echo "  cc setup-bedrock  Configure AWS (if using Bedrock)"
echo "  cc setup-vertex   Configure GCP (if using Vertex AI)"
echo "  cc help           See all commands"
