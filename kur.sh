#!/usr/bin/env bash
# ==============================================================================
# PLUS TV + OMARCHY AKILLI TV SİSTEMİ - OTOMATİK KURULUM PAKETİ (kur.sh)
# Bu script, temiz bir Omarchy kurulumunu veya çökmüş/sıfırlanmış bir sistemi
# tek komutla Plus TV Akıllı TV & Medya Merkezine dönüştürür.
# ==============================================================================

set -euo pipefail

KIT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
BACKUP_DIR="$HOME/.config/plustv-prev-backup-$TIMESTAMP"

echo -e "\033[1;36m==============================================================\033[0m"
echo -e "\033[1;32m  PLUS TV & OMARCHY AKILLI TV SİSTEMİ KURULUMU BAŞLIYOR...   \033[0m"
echo -e "\033[1;36m==============================================================\033[0m"

# 1. Gerekli sistem paketlerini kontrol et ve eksikleri tamamla
NEEDED_PKGS=()
for pkg in qt6-base qt6-declarative qt6-svg qt6-wayland qrencode wtype jq; do
  if ! pacman -Qi "$pkg" >/dev/null 2>&1; then
    NEEDED_PKGS+=("$pkg")
  fi
done

if (( ${#NEEDED_PKGS[@]} > 0 )); then
  echo -e "\033[1;33m-> Eksik sistem paketleri kuruluyor: ${NEEDED_PKGS[*]}\033[0m"
  if command -v omarchy-pkg-add >/dev/null 2>&1; then
    omarchy-pkg-add "${NEEDED_PKGS[@]}"
  else
    sudo pacman -S --needed --noconfirm "${NEEDED_PKGS[@]}"
  fi
else
  echo -e "\033[1;32m-> Tüm temel paketler (Qt6, qrencode, wtype, jq) zaten kurulu.\033[0m"
fi

# 2. Mevcut ayarların yedeğini al
echo -e "\033[1;34m-> Mevcut yapılandırma yedekleniyor: $BACKUP_DIR\033[0m"
mkdir -p "$BACKUP_DIR"
[[ -d "$HOME/.config/hypr" ]] && cp -a "$HOME/.config/hypr" "$BACKUP_DIR/" || true
[[ -d "$HOME/.config/omarchy" ]] && cp -a "$HOME/.config/omarchy" "$BACKUP_DIR/" || true

# 3. Plus TV uygulamasını ~/Work/plustv dizinine kopyala ve derle/kur
echo -e "\033[1;34m-> Plus TV kaynak kodu ve ikili dosyası kuruluyor...\033[0m"
mkdir -p "$HOME/Work" "$HOME/.local/bin"
if [[ "$KIT_DIR/plustv" != "$HOME/Work/plustv" ]]; then
  rm -rf "$HOME/Work/plustv"
  cp -a "$KIT_DIR/plustv" "$HOME/Work/plustv"
fi

BUILD_SUCCESS=0
if command -v qmake6 >/dev/null 2>&1; then
  echo -e "\033[1;34m-> Plus TV derleniyor...\033[0m"
  if (cd "$HOME/Work/plustv" && ./bin/build); then
    BUILD_SUCCESS=1
  fi
fi

if (( BUILD_SUCCESS == 0 )); then
  if [[ -f "$KIT_DIR/plustv/build/plustv" ]]; then
    echo -e "\033[1;33m-> Derleme atlandı, paketteki hazır derlenmiş ikili dosya kopyalanıyor...\033[0m"
    mkdir -p "$HOME/Work/plustv/build"
    cp -f "$KIT_DIR/plustv/build/plustv" "$HOME/Work/plustv/build/plustv"
  else
    echo -e "\033[1;31m-> Hata: Plus TV binary bulunamadı ve derlenemedi!\033[0m" >&2
    exit 1
  fi
fi

# Çalışan süreç varsa text file busy hatasını önlemek için durdur ve eski ikiliyi sil
pkill -x plustv >/dev/null 2>&1 || true
sleep 0.5
rm -f "$HOME/.local/bin/plustv"
install -Dm755 "$HOME/Work/plustv/build/plustv" "$HOME/.local/bin/plustv"

# Yardımcı sistem betiklerini (duvar kağıdı ve katman onarıcıları) yükle
if [[ -d "$KIT_DIR/local/bin" ]]; then
  cp -a "$KIT_DIR/local/bin/." "$HOME/.local/bin/"
  find "$HOME/.local/bin" -type f -exec sed -i "s|/home/enisozen|$HOME|g" {} + 2>/dev/null || true
  chmod +x "$HOME/.local/bin/"* || true
fi

# 4. Hyprland ve Omarchy yapılandırmalarını yükle (kullanıcı dizinini dinamik uyarla)
echo -e "\033[1;34m-> Hyprland ve Omarchy Akıllı TV ayarları uygulanıyor...\033[0m"
mkdir -p "$HOME/.config/hypr" "$HOME/.config/omarchy"
cp -a "$KIT_DIR/config/hypr/." "$HOME/.config/hypr/"
cp -a "$KIT_DIR/config/omarchy/." "$HOME/.config/omarchy/"

# Farklı kullanıcı adına sahip başka bir PC'ye kurulursa /home/enisozen yollarını $HOME ile değiştir
find "$HOME/.config/hypr" -type f -exec sed -i "s|/home/enisozen|$HOME|g" {} + 2>/dev/null || true
find "$HOME/.config/omarchy" -type f -name "*.sh" -exec sed -i "s|/home/enisozen|$HOME|g" {} + 2>/dev/null || true
find "$HOME/.config/omarchy/hooks" -type f -exec chmod +x {} + 2>/dev/null || true

# 5. Tarayıcı donanım/Wayland bayraklarını yükle
if [[ -d "$KIT_DIR/config/browser-flags" ]]; then
  cp -a "$KIT_DIR/config/browser-flags/." "$HOME/.config/"
fi

# 6. Medya platformu kısayollarını (.desktop) ve ikonları yükle
echo -e "\033[1;34m-> Medya kanalları ve uygulama ikonları yükleniyor...\033[0m"
mkdir -p "$HOME/.local/share/applications" "$HOME/.local/share/icons"
cp -a "$KIT_DIR/local/applications/." "$HOME/.local/share/applications/"
cp -a "$KIT_DIR/local/icons/." "$HOME/.local/share/icons/"

find "$HOME/.local/share/applications" -maxdepth 1 -name "*.desktop" -type f -exec sed -i "s|/home/enisozen|$HOME|g" {} + 2>/dev/null || true

# 7. Plus TV temasını, saydamlık, imleç ve sistem markalamasını etkinleştir
echo -e "\033[1;34m-> Plus TV teması, fare imleci ve sistem markalaması uygulanıyor...\033[0m"
if command -v omarchy >/dev/null 2>&1; then
  omarchy theme set plus-tv >/dev/null 2>&1 || true
  omarchy bar background glassy >/dev/null 2>&1 || true
  omarchy restart shell >/dev/null 2>&1 || true
  omarchy restart terminal >/dev/null 2>&1 || true
fi

# TV için büyütülmüş fare imlecini (boyut 40) sisteme hemen uygula
if command -v gsettings >/dev/null 2>&1; then
  gsettings set org.gnome.desktop.interface cursor-size 40 >/dev/null 2>&1 || true
fi
if command -v hyprctl >/dev/null 2>&1; then
  hyprctl setcursor default 40 >/dev/null 2>&1 || true
fi

# 8. Servisleri ve masaüstünü canlı olarak yenile
echo -e "\033[1;34m-> Hyprland ve Plus TV yeniden başlatılıyor...\033[0m"
if command -v hyprctl >/dev/null 2>&1; then
  hyprctl reload >/dev/null 2>&1 || true
  pkill -x plustv >/dev/null 2>&1 || true
  sleep 1
  if ! pgrep -x plustv >/dev/null 2>&1; then
    hyprctl dispatch exec "uwsm-app -- $HOME/.local/bin/plustv" >/dev/null 2>&1 || true
  fi
fi

echo -e "\033[1;36m==============================================================\033[0m"
echo -e "\033[1;32m  KURULUM TAMAMLANDI! Plus TV Akıllı TV Sistemi Hazır.       \033[0m"
echo -e "\033[1;36m==============================================================\033[0m"
echo -e "  * Plus TV içinde 'R' tuşuna basarak veya üstteki '📱 Kumanda'"
echo -e "    butonuna tıklayarak telefonunuzu uzaktan kumanda yapabilirsiniz."
echo -e "  * Sistem çökmesine karşı anlık görüntü (snapshot) almak için:"
echo -e "    \033[1;33momarchy snapshot create\033[0m"
