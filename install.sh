#!/bin/bash
# Install cc (Claude Code switcher) - adds project dir to PATH
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASHRC="$HOME/.bashrc"
ZSHRC="$HOME/.zshrc"

PATH_LINE="export PATH=\"${SCRIPT_DIR}:\$PATH\""

add_to_rc() {
    local rc="$1"
    if [[ -f "$rc" ]] && ! grep -qF "claude-code-switcher" "$rc" 2>/dev/null; then
        echo "" >> "$rc"
        echo "# Claude Code switcher" >> "$rc"
        echo "$PATH_LINE" >> "$rc"
        echo "Added to $rc"
    elif [[ -f "$rc" ]]; then
        echo "Already in $rc"
    fi
}

add_to_rc "$BASHRC"
add_to_rc "$ZSHRC"

echo ""
echo "Done! Next steps:"
echo "  source ~/.bashrc      (or open new terminal)"
echo "  cc status             Verify installation"
echo "  cc setup-bedrock      Configure AWS (if using Bedrock)"
echo "  cc help               See all commands"
