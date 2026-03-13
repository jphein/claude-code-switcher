#!/bin/bash
# Install cc (Claude Code switcher) - adds project dir to PATH + shell wrapper
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASHRC="$HOME/.bashrc"
ZSHRC="$HOME/.zshrc"

# ── Clean up old ~/bin symlinks from previous install method ─────────────────
for old_link in "$HOME/bin/cc" "$HOME/bin/cc-test" "$HOME/bin/cc-api-test"; do
    if [[ -L "$old_link" ]]; then
        rm "$old_link"
        echo "Removed old symlink: $old_link"
    fi
done

# ── Shell integration ────────────────────────────────────────────────────────
PATH_LINE="export PATH=\"${SCRIPT_DIR}:\$PATH\""

# Wrapper function: auto-sources env.sh after provider/model switches
WRAPPER='cc() { command cc "$@" && [[ -f ~/.config/claude-code/env.sh ]] && source ~/.config/claude-code/env.sh; }'

add_to_rc() {
    local rc="$1"
    [[ -f "$rc" ]] || return 0

    local changed=false

    # Add PATH entry
    if ! grep -qF "claude-code-switcher" "$rc" 2>/dev/null; then
        echo "" >> "$rc"
        echo "# Claude Code switcher" >> "$rc"
        echo "$PATH_LINE" >> "$rc"
        echo "Added PATH to $rc"
        changed=true
    else
        echo "PATH already in $rc"
    fi

    # Add wrapper function (auto-sources env.sh)
    if ! grep -qF 'cc()' "$rc" 2>/dev/null; then
        echo "$WRAPPER" >> "$rc"
        echo "Added cc() wrapper to $rc"
        changed=true
    else
        echo "cc() wrapper already in $rc"
    fi
}

add_to_rc "$BASHRC"
add_to_rc "$ZSHRC"

echo ""
echo "Done! Next steps:"
echo "  source ~/.bashrc      (or open new terminal)"
echo "  cc status             Verify installation"
echo "  cc setup              See provider setup options"
echo "  cc help               See all commands"
