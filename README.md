# DLL Injector

Advanced DLL injection tool with multiple injection methods and stealth features.

## Features

- **LoadLibrary:** Standard injection method using Windows API.
- **Manual Map:** Custom PE loader that maps DLL into memory without using standard Windows loader.
- **NtMapViewOfSection:** Stealthy mapping using kernel-level syscalls to bypass common memory allocation detections.
- **Advanced Bypass:** Patches target resource entry points to temporarily disable initialization and detection routines.
- **Module Hiding:** Unlinks the injected module from the Process Environment Block (PEB) list to hide it from process scanners.
- **PE Header Erasing:** Overwrites DLL headers in memory after injection to prevent identification by memory scanners.
- **Thread Hijacking:** Intercepts an existing thread in the target process to execute code instead of creating a new one.

## Usage

1. Run the application as Administrator.
2. Select the target process from the list or use the search bar.
3. Browse and select the DLL file you wish to inject.
4. Choose your preferred injection method.
5. Select additional stealth options if required.
6. Click "PERFORM INJECTION".

---

# DLL Injector (Turkish)

Çeşitli enjeksiyon yöntemleri ve gizlilik özellikleri içeren gelişmiş DLL enjeksiyon aracı.

## Özellikler

- **LoadLibrary:** Windows API kullanan standart enjeksiyon yöntemi.
- **Manual Map:** Standart Windows yükleyicisini kullanmadan DLL'i hafızaya haritalayan özel PE yükleyici.
- **NtMapViewOfSection:** Yaygın hafıza ayırma tespitlerini atlamak için çekirdek seviyesindeki syscall'ları kullanan gizli haritalama.
- **Advanced Bypass:** Başlatma ve tespit rutinlerini geçici olarak devre dışı bırakmak için hedef kaynak giriş noktalarını yamalar.
- **Modül Gizleme:** İşlem tarayıcılarından gizlemek için enjekte edilen modülü Process Environment Block (PEB) listesinden siler.
- **PE Başlığı Silme:** Hafıza tarayıcıları tarafından tanımlanmasını önlemek için enjeksiyondan sonra hafızadaki DLL başlıklarının üzerine yazar.
- **Thread Hijacking:** Yeni bir thread oluşturmak yerine hedef işlemdeki mevcut bir thread'i durdurarak kodu çalıştırır.

## Kullanım

1. Uygulamayı Yönetici olarak çalıştırın.
2. Listeden hedef işlemi seçin veya arama çubuğunu kullanın.
3. Enjekte etmek istediğiniz DLL dosyasını seçin.
4. Tercih ettiğiniz enjeksiyon yöntemini belirleyin.
5. Gerekiyorsa ek gizlilik seçeneklerini işaretleyin.
6. "PERFORM INJECTION" butonuna tıklayın.

<img width="1771" height="1012" alt="image" src="https://github.com/user-attachments/assets/37cd783b-757d-4f50-881d-4c73a34e375b" />
<img width="1832" height="1039" alt="image" src="https://github.com/user-attachments/assets/244cbc24-297d-44e8-9ce4-dce34f28422e" />
<img width="486" height="683" alt="image" src="https://github.com/user-attachments/assets/f6d5d3af-a511-4d52-bd1d-e73d79777dd2" />



