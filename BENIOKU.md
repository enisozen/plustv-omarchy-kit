# Plus TV & Omarchy Akıllı TV Taşınabilir Kurulum Paketi

Bu paket, Omarchy üzerinde oluşturduğumuz **Plus TV Akıllı TV & Medya Merkezi** sistemini, **Telefon Uzaktan Kumanda** modülünü, tüm **Hyprland / Omarchy özel ayarlarını**, **Plus TV & Digiturk temalarını**, **Özel Sinematik Duvar Kağıtlarını**, **Hakkında ve Ekran Koruyucu Markalamalarını**, **Kilidi Aç (Plymouth) Kilit Ekranını** ve **12 medya platformu kısayolunu** içerir.

---

## 1. Telefonu Uzaktan Kumanda Olarak Kullanma 📱
1. Plus TV açıkken klavyeden **`R`** tuşuna basın veya sağ üstteki **`📱 Kumanda`** butonuna tıklayın.
2. Ekranda açılan **QR Kodu**, bilgisayar ile aynı Wi-Fi ağında olan telefonunuzun kamerasıyla okutun (veya ekranda yazan `http://192.168.x.x:53317` adresini telefon tarayıcısında açın).
3. Telefon ekranınızda kurulum gerektirmeyen **Plus TV Akıllı Kumanda** açılır:
   - **⏻ Uzaktan Güç Yönetimi (Sağ Üst Kırmızı Güç Butonu):**
     - **Bilgisayarı Kapat (Shutdown):** TV ve bilgisayarı güvenli onay penceresiyle tamamen kapatma.
     - **Yeniden Başlat (Reboot):** Bilgisayarı uzaktan baştan başlatma.
     - **Uyku Modu (Suspend):** Sistemi anında uyku moduna alma.
     - **Ekranı Aç / Kapat (DPMS):** TV ekranını karartma veya uyandırma.
     - **Ekranı Kilitle:** Kilit ekranına alma.
   - **🖥️ Masaüstleri Sekmesi & Hızlı Masaüstü Geçişi:**
     - Açık olan tüm masaüstlerini (Workspaces) ve içlerindeki aktif uygulamaları canlı olarak listeleme.
     - Tek dokunuşla istenen masaüstüne geçme.
     - `◀ Önceki Masaüstü` ve `Sonraki Masaüstü ▶` butonlarıyla tek parmakla hızlı gezinme.
     - Kumanda sekmesinde D-Pad'in hemen altında entegre hızlı masaüstü geçiş çubuğu.
   - **🎛️ Kumanda Sekmesi:** Yön tuşları (D-Pad), OK, Geri, Kategori değiştirme, Ses Açma/Kısma/Sessiz, 10 sn İleri/Geri Sarma, Oynat/Durdur.
   - **📺 Kanallar Sekmesi:** beIN CONNECT, Digiturk, TV+, Apple TV+, Prime Video, Netflix, YouTube, Chrome, Apple Music, Spotify, YouTube Music ve GeForce NOW kanallarını tek dokunuşla doğrudan açma.
   - **🎮 Oyun Kontrolcüsü Sekmesi (Gamepad):**
     - **Uzaktan Oyun Deneyimi:** Koltuktan TV'de oyun oynayabilmeniz için entegre mobil oyun kontrolcüsü (GeForce NOW, emülatörler, bulut ve masaüstü oyunları).
     - **İki Farklı Mod:** 🕹️ WASD (PC/FPS) ve 🏹 Ok Tuşları (Retro/TV) modları arasında tek dokunuşla geçiş.
     - **Ergonomik D-Pad & Basılı Tutma:** Yön kontrollerinde parmağınızı basılı tuttuğunuz sürece kesintisiz yürüme, koşma ve hareket.
     - **Renkli ABXY Butonları:** Klasik konsol düzeninde A (Space / Zıpla), B (Shift / Koş), X (Ctrl / Eğil / Etkileşim), Y (R / Şarjör).
     - **Tetikler & Omuz Tuşları:** LT (Nişan Al / Sağ Fare Tık), RT (Ateş Et / Sol Fare Tık), LB (Q / Sol El), RB (E / Sağ El).
     - **Sistem Tuşları:** SELECT (Tab - Envanter/Skor), START (Esc - Duraklat/Menü), 1-4 hızlı silah/eşya seçimi.
   - **🖱️ Fare & Klavye Sekmesi:**
     - **Air Touchpad:** Dokunmatik imleç kontrolü, Sol Tık, Sağ Tık, Orta Tık ve çift yönlü sayfa kaydırma.
     - **Tam Donanımlı Windows PC Klavyesi:** Esc, F1-F12, Del, Türkçe Q klavye tuşları, Caps Lock, Shift, Tab, Ctrl, Alt, AltGr, Boşluk ve Ok tuşları.
     - **Windows Kısayol Tuşları:** ⊞ Win, Win+D (Masaüstü Göster), Alt+Tab (Pencere Değiştir), Alt+F4 (Pencere Kapat), Ctrl+C (Kopyala), Ctrl+V (Yapıştır), Ctrl+Z (Geri Al), Ctrl+A (Tümünü Seç).
     - **Arama Çubuğu:** Film/dizi veya şarkı adını doğrudan TV'ye yazıp aratma.
   - **🔍 Büyütülmüş TV Fare İmleci:** TV'ye uzaktan bakıldığında fare imlecinin rahatça görülmesi için imleç boyutu net ve belirgin (boyut 40) olarak ayarlandı.
   - **Üst Bar:** Hangi kanalda olursanız olun **`🏠 Plus TV`** tuşuyla ana ekrana dönme, **`🖥️ Masaüstü`** sekmesini açma, **`⛶ Tam Ekran`** yapma veya **`❌ Kapat`** ile açık uygulamayı kapatıp Plus TV'ye dönme.

---

## 2. Başka Bir Bilgisayara (veya Sıfırlanmış Sisteme) Kurma 🚀
1. `Downloads/plustv-omarchy-kit.tar.gz` dosyasını bir USB belleğe kopyalayın.
2. Yeni bilgisayara standart **Omarchy** kurun ve USB bellekteki arşivi çıkartıp tek komutu çalıştırın:
   ```bash
   tar -xzf plustv-omarchy-kit.tar.gz
   cd plustv-omarchy-kit
   ./kur.sh
   ```
3. `kur.sh` tüm gerekli paketleri kontrol eder, Plus TV'yi derleyip kurar, tüm Hyprland/Omarchy ayarlarını ve kanal ikonlarını yeni bilgisayarın kullanıcı adına uyarlayarak 1 dakikada sistemi birebir ayağa kaldırır.

---

## 3. İleride Yaptığınız Değişiklikleri Yedeğe Kaydetme 💾
Sistemde yeni bir ayar yaptığınızda veya Plus TV'yi güncellediğinizde yedeği yenilemek için:
```bash
~/Work/plustv-omarchy-kit/yedek-guncelle.sh
```
komutunu çalıştırmanız yeterlidir. Hem klasör hem de `~/Downloads/plustv-omarchy-kit.tar.gz` dosyası otomatik güncellenir.

---

## 4. Sistem Çökmelerine Karşı Anlık Geri Yükleme (Btrfs Snapshot) 🛡️
Mevcut bilgisayarınızda güncelleme veya deneme yapmadan önce 1 saniyede sistem görüntüsü almak için:
```bash
omarchy snapshot create
```
Sistemde bir sorun olursa geri dönmek için:
```bash
omarchy snapshot restore
```
veya bilgisayar açılırken önyükleme (boot) menüsünden önceki sağlam görüntüyü seçebilirsiniz.
