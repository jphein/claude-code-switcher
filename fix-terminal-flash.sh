#!/bin/bash
# fix-terminal-flash.sh — Disable terminal flashing/blinking for Claude Code
#
# Claude Code's spinner and progress updates can trigger terminal bells
# and cursor blinks, causing annoying screen flashes.

echo "Fixing terminal flash/blink settings..."

# 1. GNOME Terminal - all profiles
echo "[1/4] GNOME Terminal profiles..."
for profile in $(dconf list /org/gnome/terminal/legacy/profiles:/ 2>/dev/null | tr -d '/:'); do
  dconf write /org/gnome/terminal/legacy/profiles:/:$profile/cursor-blink-mode "'off'"
  dconf write /org/gnome/terminal/legacy/profiles:/:$profile/text-blink-mode "'never'"
  dconf write /org/gnome/terminal/legacy/profiles:/:$profile/audible-bell false
  echo "  Fixed profile: $profile"
done

# 2. GTK/GNOME system settings
echo "[2/4] System settings..."
gsettings set org.gnome.desktop.interface cursor-blink false 2>/dev/null
gsettings set org.gnome.desktop.wm.preferences visual-bell false 2>/dev/null
gsettings set org.gnome.desktop.wm.preferences audible-bell false 2>/dev/null
gsettings set org.gnome.desktop.sound event-sounds false 2>/dev/null
echo "  Disabled cursor-blink, visual-bell, audible-bell, event-sounds"

# 3. Shell bell
echo "[3/4] Shell inputrc..."
grep -q "set bell-style none" ~/.inputrc 2>/dev/null || echo "set bell-style none" >> ~/.inputrc
echo "  Set bell-style none"

# 4. tmux (if used)
echo "[4/4] tmux..."
if [[ -f ~/.tmux.conf ]]; then
  grep -q "set -g visual-bell off" ~/.tmux.conf 2>/dev/null || {
    echo -e "\n# Disable bells\nset -g visual-bell off\nset -g bell-action none" >> ~/.tmux.conf
    echo "  Added tmux bell settings"
  }
else
  echo "  No tmux.conf (skipped)"
fi

echo ""
echo "Done! Terminal should no longer flash."
echo "Changes apply immediately to new terminals."
