#!/usr/bin/env bash
# ==============================================================================
# PLUS TV + OMARCHY SMART TV SYSTEM - AUTOMATED INSTALLATION SCRIPT (install.sh)
# Transforms a clean Arch Linux / Omarchy installation or restores a reset system
# into a fully-featured Plus TV Smart TV & Media Center with a single command.
# ==============================================================================

set -euo pipefail

KIT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
BACKUP_DIR="$HOME/.config/plustv-prev-backup-$TIMESTAMP"

echo -e "\033[1;36m==============================================================\033[0m"
echo -e "\033[1;32m  PLUS TV & OMARCHY SMART TV SYSTEM INSTALLATION STARTING... \033[0m"
echo -e "\033[1;36m==============================================================\033[0m"

# 1. Check and install required system dependencies
NEEDED_PKGS=()
for pkg in qt6-base qt6-declarative qt6-svg qt6-wayland qrencode wtype jq gcc make; do
  if ! pacman -Qi "$pkg" >/dev/null 2>&1; then
    NEEDED_PKGS+=("$pkg")
  fi
done

if (( ${#NEEDED_PKGS[@]} > 0 )); then
  echo -e "\033[1;33m-> Installing missing dependencies: ${NEEDED_PKGS[*]}\033[0m"
  if command -v omarchy-pkg-add >/dev/null 2>&1; then
    omarchy-pkg-add "${NEEDED_PKGS[@]}"
  else
    sudo pacman -S --needed --noconfirm "${NEEDED_PKGS[@]}"
  fi
else
  echo -e "\033[1;32m-> All core packages (Qt6, qrencode, wtype, jq) are installed.\033[0m"
fi

# 2. Backup existing configuration
echo -e "\033[1;34m-> Backing up existing configuration to: $BACKUP_DIR\033[0m"
mkdir -p "$BACKUP_DIR"
[[ -d "$HOME/.config/hypr" ]] && cp -a "$HOME/.config/hypr" "$BACKUP_DIR/" || true
[[ -d "$HOME/.config/omarchy" ]] && cp -a "$HOME/.config/omarchy" "$BACKUP_DIR/" || true

# 3. Deploy Plus TV application source & compile / install binary
echo -e "\033[1;34m-> Deploying Plus TV source code and binaries...\033[0m"
mkdir -p "$HOME/Work" "$HOME/.local/bin"
if [[ "$KIT_DIR/plustv" != "$HOME/Work/plustv" ]]; then
  rm -rf "$HOME/Work/plustv"
  cp -a "$KIT_DIR/plustv" "$HOME/Work/plustv"
fi

BUILD_SUCCESS=0
if command -v qmake6 >/dev/null 2>&1; then
  echo -e "\033[1;34m-> Compiling Plus TV from source...\033[0m"
  if (cd "$HOME/Work/plustv" && ./bin/build); then
    BUILD_SUCCESS=1
  fi
fi

if (( BUILD_SUCCESS == 0 )); then
  if [[ -f "$KIT_DIR/plustv/build/plustv" ]]; then
    echo -e "\033[1;33m-> Compilation skipped or failed, using bundled pre-compiled binary...\033[0m"
    mkdir -p "$HOME/Work/plustv/build"
    cp -f "$KIT_DIR/plustv/build/plustv" "$HOME/Work/plustv/build/plustv"
  else
    echo -e "\033[1;31m-> Error: Plus TV binary not found and could not be built!\033[0m" >&2
    exit 1
  fi
fi

# Stop running instance to prevent 'text file busy' errors and install binary
pkill -x plustv >/dev/null 2>&1 || true
sleep 0.5
rm -f "$HOME/.local/bin/plustv"
install -Dm755 "$HOME/Work/plustv/build/plustv" "$HOME/.local/bin/plustv"

# Install helper scripts (e.g. wallpaper & layer guardians)
if [[ -d "$KIT_DIR/local/bin" ]]; then
  cp -a "$KIT_DIR/local/bin/." "$HOME/.local/bin/"
  find "$HOME/.local/bin" -type f -exec sed -E -i "s|/home/[^/\"'[:space:]]+|$HOME|g" {} + 2>/dev/null || true
  chmod +x "$HOME/.local/bin/"* || true
fi

# 4. Deploy Hyprland and Omarchy TV configurations (dynamically adapting user paths)
echo -e "\033[1;34m-> Applying Hyprland and Omarchy Smart TV configurations...\033[0m"
mkdir -p "$HOME/.config/hypr" "$HOME/.config/omarchy"
cp -a "$KIT_DIR/config/hypr/." "$HOME/.config/hypr/"
cp -a "$KIT_DIR/config/omarchy/." "$HOME/.config/omarchy/"

# Dynamically replace hardcoded paths with current user's $HOME
find "$HOME/.config/hypr" -type f -exec sed -E -i "s|/home/[^/\"'[:space:]]+|$HOME|g" {} + 2>/dev/null || true
find "$HOME/.config/omarchy" -type f -name "*.sh" -exec sed -E -i "s|/home/[^/\"'[:space:]]+|$HOME|g" {} + 2>/dev/null || true
find "$HOME/.config/omarchy/hooks" -type f -exec chmod +x {} + 2>/dev/null || true

# 5. Install browser Wayland and hardware acceleration flags
if [[ -d "$KIT_DIR/config/browser-flags" ]]; then
  cp -a "$KIT_DIR/config/browser-flags/." "$HOME/.config/"
fi

# 6. Deploy media platform desktop entries and icons
echo -e "\033[1;34m-> Deploying media channels and application icons...\033[0m"
mkdir -p "$HOME/.local/share/applications" "$HOME/.local/share/icons"
cp -a "$KIT_DIR/local/applications/." "$HOME/.local/share/applications/"
cp -a "$KIT_DIR/local/icons/." "$HOME/.local/share/icons/"

find "$HOME/.local/share/applications" -maxdepth 1 -name "*.desktop" -type f -exec sed -E -i "s|/home/[^/\"'[:space:]]+|$HOME|g" {} + 2>/dev/null || true

# 7. Apply Plus TV theme, transparency, cursor, and branding
echo -e "\033[1;34m-> Applying Plus TV theme, enlarged TV cursor, and branding...\033[0m"
if command -v omarchy >/dev/null 2>&1; then
  omarchy theme set plus-tv >/dev/null 2>&1 || true
  omarchy bar background glassy >/dev/null 2>&1 || true
  omarchy restart shell >/dev/null 2>&1 || true
  omarchy restart terminal >/dev/null 2>&1 || true
fi

# Apply 40px enlarged TV mouse cursor immediately
if command -v gsettings >/dev/null 2>&1; then
  gsettings set org.gnome.desktop.interface cursor-size 40 >/dev/null 2>&1 || true
fi
if command -v hyprctl >/dev/null 2>&1; then
  hyprctl setcursor default 40 >/dev/null 2>&1 || true
fi

# 8. Reload Hyprland and start Plus TV cleanly on Workspace 1
echo -e "\033[1;34m-> Reloading Hyprland and launching Plus TV...\033[0m"
if command -v hyprctl >/dev/null 2>&1; then
  hyprctl reload >/dev/null 2>&1 || true
  pkill -x plustv >/dev/null 2>&1 || true
  sleep 1
  if ! pgrep -x plustv >/dev/null 2>&1; then
    hyprctl dispatch exec "uwsm-app -- $HOME/.local/bin/plustv" >/dev/null 2>&1 || true
  fi
fi

echo -e "\033[1;36m==============================================================\033[0m"
echo -e "\033[1;32m  INSTALLATION COMPLETE! Plus TV Smart TV System is Ready.   \033[0m"
echo -e "\033[1;36m==============================================================\033[0m"
echo -e "  * Press 'R' inside Plus TV or click '📱 Remote' on the top bar"
echo -e "    to open the mobile web remote control via QR code."
echo -e "  * To capture a system snapshot against future crashes:"
echo -e "    \033[1;33momarchy snapshot create\033[0m"
