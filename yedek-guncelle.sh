#!/usr/bin/env bash
# ==============================================================================
# PLUS TV + OMARCHY TAŞINABİLİR YEDEK PAKETİ GÜNCELLEYİCİ (yedek-guncelle.sh)
# Sistemde veya Plus TV'de yaptığınız yeni değişiklikleri bu pakete aktarır ve
# başka PC'lere taşınmaya hazır .tar.gz arşivini yeniden üretir.
# ==============================================================================

set -euo pipefail

KIT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "\033[1;34m-> Plus TV ve sistem ayarları taşınabilir pakete kopyalanıyor...\033[0m"

rm -rf "$KIT_DIR/plustv" "$KIT_DIR/config" "$KIT_DIR/local"
mkdir -p "$KIT_DIR/plustv" \
         "$KIT_DIR/config/hypr" \
         "$KIT_DIR/config/omarchy" \
         "$KIT_DIR/config/browser-flags" \
         "$KIT_DIR/local/applications" \
         "$KIT_DIR/local/icons" \
         "$KIT_DIR/local/bin"

# 1. Plus TV kaynak kodu ve derlenmiş binary (pkgbuild geçici dosyaları hariç)
cp -a "$HOME/Work/plustv/src" "$KIT_DIR/plustv/"
cp -a "$HOME/Work/plustv/bin" "$KIT_DIR/plustv/"
cp -a "$HOME/Work/plustv/tests" "$KIT_DIR/plustv/"
cp -a "$HOME/Work/plustv/plustv.pro" "$HOME/Work/plustv/README.md" "$HOME/Work/plustv/LICENSE" "$HOME/Work/plustv/.gitignore" "$KIT_DIR/plustv/"
mkdir -p "$KIT_DIR/plustv/build" "$KIT_DIR/plustv/pkgbuild"
[[ -f "$HOME/Work/plustv/build/plustv" ]] && cp -a "$HOME/Work/plustv/build/plustv" "$KIT_DIR/plustv/build/"
cp -a "$HOME/Work/plustv/pkgbuild/PKGBUILD" "$HOME/Work/plustv/pkgbuild/plustv.desktop" "$HOME/Work/plustv/pkgbuild/plustv.svg" "$KIT_DIR/plustv/pkgbuild/"

# 2. Hyprland ayarları (.bak yedekleri hariç temiz dosyalar)
for f in autostart.lua bindings.lua hyprland.lua input.lua looknfeel.lua monitors.lua hyprsunset.conf xdph.conf .luarc.json; do
  [[ -f "$HOME/.config/hypr/$f" ]] && cp -a "$HOME/.config/hypr/$f" "$KIT_DIR/config/hypr/"
done

# 3. Omarchy ayarları (branding, digiturk teması, shell.json, dil, eklentiler)
for item in branding defaults extensions hooks language plugins shell.json themed themes; do
  [[ -e "$HOME/.config/omarchy/$item" ]] && cp -a "$HOME/.config/omarchy/$item" "$KIT_DIR/config/omarchy/"
done

# 4. Tarayıcı bayrakları (chrome-flags.conf & chromium-flags.conf)
for f in chrome-flags.conf chromium-flags.conf; do
  [[ -f "$HOME/.config/$f" ]] && cp -a "$HOME/.config/$f" "$KIT_DIR/config/browser-flags/"
done

# 5. Uygulama kısayolları (.desktop) ve ikonlar
cp -a "$HOME/.local/share/applications/." "$KIT_DIR/local/applications/"
[[ -d "$HOME/.local/share/icons" ]] && cp -a "$HOME/.local/share/icons/." "$KIT_DIR/local/icons/"

# 6. Yardımcı araçlar ve betikler
[[ -f "$HOME/.local/bin/omarchy-ensure-wallpaper" ]] && cp -a "$HOME/.local/bin/omarchy-ensure-wallpaper" "$KIT_DIR/local/bin/"

chmod +x "$KIT_DIR/kur.sh" "$KIT_DIR/install.sh" "$KIT_DIR/yedek-guncelle.sh"
[[ -d "$KIT_DIR/local/bin" ]] && chmod +x "$KIT_DIR/local/bin/"* || true

# Normalize user home paths in exported config/local files for portability
find "$KIT_DIR/config/hypr" -type f -exec sed -E -i "s|/home/[^/\"'[:space:]]+|/home/enisozen|g" {} + 2>/dev/null || true
find "$KIT_DIR/config/omarchy" -type f -name "*.sh" -exec sed -E -i "s|/home/[^/\"'[:space:]]+|/home/enisozen|g" {} + 2>/dev/null || true
find "$KIT_DIR/local/bin" -type f -exec sed -E -i "s|/home/[^/\"'[:space:]]+|/home/enisozen|g" {} + 2>/dev/null || true
find "$KIT_DIR/local/applications" -maxdepth 1 -name "*.desktop" -type f -exec sed -E -i "s|/home/[^/\"'[:space:]]+|/home/enisozen|g" {} + 2>/dev/null || true

# 7. Taşınabilir .tar.gz arşivini oluştur
ARCHIVE_WORK="$HOME/Work/plustv-omarchy-kit.tar.gz"
ARCHIVE_DL="$HOME/Downloads/plustv-omarchy-kit.tar.gz"

tar -czf "$ARCHIVE_WORK" -C "$(dirname "$KIT_DIR")" "$(basename "$KIT_DIR")"
mkdir -p "$HOME/Downloads"
cp -f "$ARCHIVE_WORK" "$ARCHIVE_DL"

echo -e "\033[1;32m-> Taşınabilir kurulum paketi güncellendi!\033[0m"
echo -e "   Klasör : $KIT_DIR"
echo -e "   Arşiv 1: $ARCHIVE_WORK"
echo -e "   Arşiv 2: $ARCHIVE_DL"
