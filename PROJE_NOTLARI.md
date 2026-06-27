# SAM E54 Xplained Pro — Proje Notları ve Devir Belgesi

Bu belge, eski projede (Windows'tan gelen "Deneme") çözdüğümüz her şeyi içerir.
Yeni (Mac'te temiz) Harmony projesinde bu değerleri kullanıp üstüne CAN FD ekleyeceğiz.

---

## 1. Donanım

| Öğe | Değer |
|---|---|
| Kart | SAM E54 Xplained Pro |
| MCU | ATSAME54P20A |
| Debugger/Programlayıcı | Onboard **EDBG** (SN: ATML2748071800006539) |
| Host | macOS |
| Seri port (Mac) | `/dev/tty.usbmodem102` veya `/dev/cu.usbmodem102` |

---

## 2. UART testi (ÇALIŞAN ayarlar)

- **Peripheral:** SERCOM2 → USART modu
- **Pinler:** **PB25 = TX** (PAD0), **PB24 = RX** (PAD1)
  - Bu pinler kartta **EDBG'nin sanal COM portuna** bağlı → ekstra kablo gerekmez, debug USB'si yeterli.
- **Baud:** 115200, 8N1 (TXPO=0, RXPO=1)
- **Önemli:** stdio yönlendirilmemiş (`xc32_monitor.c` boş) → **`printf` çalışmaz.** Doğrudan SERCOM2'ye yazılır.
- **Kritik bulgu:** Kütüphanenin kesme tabanlı `SERCOM2_USART_Write` fonksiyonu **takılıyordu** (WriteIsBusy hiç temizlenmedi). Çözüm: **polling (doğrudan register'a yazan)** `uart_print`.

### Çalışan `main.c` (polling UART):
```c
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "definitions.h"

static void uart_print(const char *s)
{
    while (*s != '\0')
    {
        while ((SERCOM2_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) == 0U) { }
        SERCOM2_REGS->USART_INT.SERCOM_DATA = (uint8_t)(*s);
        s++;
    }
    while ((SERCOM2_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk) == 0U) { }
}

int main(void)
{
    SYS_Initialize(NULL);
    uart_print("\r\n=== SAM E54 calisiyor! Merhaba Furkan ===\r\n");

    uint32_t counter = 0;
    char buf[48];
    while (true)
    {
        SYS_Tasks();
        sprintf(buf, "Sayac: %lu\r\n", (unsigned long)counter++);
        uart_print(buf);
        for (volatile uint32_t d = 0; d < 3000000U; d++) { }
    }
    return EXIT_FAILURE;
}
```

---

## 3. CLOCK — en kritik kısım (arkadaşın config'inde 2 ayrı hata vardı)

### Orijinal (bozuk) kurulum:
XOSC0 (harici 12 MHz kristal) → GCLK1 → DPLL0 (×10) → 120 MHz CPU; GCLK1 ayrıca → SERCOM2 (12 MHz).

### Hata 1 — Kristal başlamıyordu
- `XOSCRDY` hiç set olmuyordu → clock init'te sonsuz döngü.
- Gerekenler (yeni projede MCC Clock Configurator'da):
  - **Automatic Loop Control (ALC) AÇIK** (bu kartın kristali ALC ile çalışacak şekilde üretilmiş).
  - **Oscillator Startup Time ~1–2 ms** (31 µs çok kısaydı; 1 sn ise gereksiz uzun).
  - **Hangi XOSC?** Belirsiz kaldı: XOSC0 bir kez başladı (kararsız), XOSC1 hiç başlamadı. Forumlar bu kartta 12 MHz'in genelde **XOSC1**'de olduğunu söylüyor. **Yeni projede önce XOSC0 (ALC + uzun startup) dene; tutmazsa XOSC1.**

### Hata 2 — DPLL kilitlenmiyordu
- DPLL'e **doğrudan 12 MHz referans** veriliyordu.
- Ama SAM E54 DPLL referans girişi **≤ ~3.2 MHz** olmalı → 12 MHz aralık dışı → hiç kilitlenmez.
- **Doğrusu:** DPLL referansını böl (ör. 12 MHz ÷ 12 = **1 MHz**), sonra **×120 → 120 MHz**.

### Bizim geçici çözümümüz (UART testi için — CAN FD'ye UYGUN DEĞİL):
Kristali ve DPLL'i tamamen bıraktık, her şeyi **dahili DFLL**'den besledik:
- GCLK0 (CPU) = DFLL = 48 MHz
- GCLK1 (SERCOM2) = DFLL ÷4 = 12 MHz (baud aynı kaldı)
- GCLK2 = DFLL ÷2 = 24 MHz
- DPLL hiç kullanılmadı.
- **Not:** DFLL açık-döngü ±birkaç % kayar → **CAN FD için yetersiz.** CAN FD'de kristal+DPLL şart.

---

## 4. CAN FD (yeni projede kurulacak)

### Pinler (eski projede zaten atanmıştı):
- **CAN1_TX = PB12**
- **CAN1_RX = PB13**
- **CAN_STBY = PC13** (GPIO çıkış, transceiver standby kontrolü)
- Not: SAM E54 sadece CAN denetleyicisini içerir; **harici CAN transceiver** gerekir (STBY pini bir transceiver olduğunu gösteriyor).

### Yapılacaklar:
- CAN1 bileşeninde **CAN FD modu** aç.
- **Nominal (arbitrasyon) bit hızı:** ___ (BELİRLENECEK)
- **Data (veri fazı) bit hızı:** ___ (BELİRLENECEK)
- CAN'a temiz bir **GCLK** ata (ör. 40/48/80 MHz, kristal+DPLL'den türetilmiş).
- Sample point / bit timing → MCC, CAN clock'undan hesaplar.

---

## 4b. MCC'de kurulu bileşenler (Project Graph) — yeni projede aynısını ekle

Eski projede şu Harmony bileşenleri vardı:

| Bileşen | Kullanım | Pin / Not |
|---|---|---|
| **SERCOM2** | USART (UART) | PB25 TX / PB24 RX — EDBG sanal COM (ÇALIŞAN UART) |
| **SERCOM7** | I2C Master | PD08 (PAD0) / PD09 (PAD1) |
| **SERCOM0** | USART (yedek?) | Pin atanmamış — boşta/yedek görünüyor |
| **CAN1** | CAN | PB12 TX / PB13 RX / PC13 STBY |
| **TC0** | Timer | TMR |
| **TRNG** | Rastgele sayı | — |
| **EVSYS** | Event System | — |
| **NVMCTRL** | Flash denetleyici (MEMORY) | — |
| **CMCC** | Cache | — |
| Core / System / CMSIS / DFP | Çekirdek | SAME54_DFP 3.11.261, CMSIS 6.3.0 |

> Yeni projede en azından **SERCOM2 (UART)** + **CAN1 (CAN FD)** şart. Diğerlerini (SERCOM7 I2C, TC0, TRNG, EVSYS) gerçekten kullanıyorsan ekle; kullanmıyorsan sade tut.

### SERCOM2 — USART (UART) ayarları (eski projedeki değerler):
| Ayar | Değer |
|---|---|
| Select SERCOM Operation Mode | **USART with internal clock** |
| Baud Rate | **115200** |
| Character Size / Data | **8 bit** |
| Parity | **None** |
| Stop Bits | **1** |
| Data Order | **LSB First** |
| Sample Rate | 16x arithmetic |
| Receive Enable (RXEN) | Açık |
| Transmit Enable (TXEN) | Açık |
| Run in Standby | Kapalı |
| Redirect STDIO/printf | **None** (bu yüzden printf çalışmıyor → polling ile yazdık. İstersen yeni projede bunu UART'a yönlendirip printf de kullanabilirsin.) |
| RXPO (RX pad) | **PAD1 = PB24** |
| TXPO (TX pad) | **PAD0 = PB25** |
| Pinler | **PB25 = SERCOM2/PAD0 (TX)**, **PB24 = SERCOM2/PAD1 (RX)** |

**Yeni projede SERCOM2'yi USART olarak eklemek için:**
1. Project Graph'ta sol taraftaki **Device Resources** listesinden **SERCOM → SERCOM2**'yi bul, çift tıkla/ekle (Project Graph'a kutu olarak gelir).
2. Gelen kutuda mod sorulursa **USART** seç (ya da kutuya tıklayıp sağdaki Configuration panelinden Operation Mode = USART).
3. Sağdaki **Configuration Options** panelinde yukarıdaki tabloyu uygula (Baud 115200, 8-None-1, LSB First).
4. **Pin Configuration** (Plugins → Pin Configuration) ekranında: **PB25 → SERCOM2 PAD0 (TX)**, **PB24 → SERCOM2 PAD1 (RX)**.
5. Generate.

### SERCOM7 — I2C Master ayarları (eski projedeki değerler):
| Ayar | Değer |
|---|---|
| Select SERCOM Operation Mode | **I2C Master** |
| Transfer Speed Mode | **STANDARD_AND_FAST_MODE** |
| Enable operation in Standby mode | Kapalı |
| SDA Hold Time | **50-100ns hold time** |
| I2C Speed in KHz | **400** |
| I2C Trise in nano seconds | **50** |
| Enable 10-bit Addressing | Kapalı |
| Pinler | PD08 (PAD0) / PD09 (PAD1) |

---

## 5. Tool / Flash iş akışı (önemli tuzaklar)

- **Proje Tool'u EDBG olmalı** (Simulator DEĞİL!).
  - "Simulate" ve **"MPLAB: Program device" → SİMÜLATÖRE gider.**
  - Gerçek kart için: **"Debug: Start Debugging" (F5).**
- **Gerçek kartta olduğunu doğrula:** DEBUG CONSOLE'da `Target device ATSAME54P20A found` / `Device Id = 0x61840000` / `Programming complete`.
  - Simülatör belirtisi: `W0106-SIM`, `beta mode`, `We don't simulate the clock`.
- **Flash ECC hatası** (`NVMCtrl ... ECCDE / ECCSE`) → **PROJECTS → Device actions → Chip erase**, sonra tekrar programla.
- launch.json'da `stopOnEntry: true` → F5'ten sonra **Continue (▶)** basmak gerekir.
- Seri monitör: **Serial Monitor (Microsoft)** eklentisi; port `usbmodem102`, 115200.

---

## 6. Neden yeni proje?

Bu proje **Windows'ta** kurulmuş (`.vscode/settings.json`'da `c:\Program Files\...` ve `\` yolları vardı). Mac'te MCC **Generate** çalışmıyor:
`ENOENT: scandir 'U:s/.../Deneme/src'` — MCC, `/Users` yolunu `U:s` diye bozuyor (platform bug'ı, dosyada saklı değil, düzeltilemiyor).
→ Bu yüzden **macOS'te sıfırdan temiz Harmony projesi** açıp yukarıdaki değerleri uygulayacağız.

---

## 7. Yeni proje kurulum sırası (özet yol haritası)

1. Mac'te yeni MPLAB/Harmony projesi → cihaz: **ATSAME54P20A**.
2. **Tool = EDBG** seç.
3. **Clock:** kristal (ALC + ~1-2 ms startup) → DPLL referansı ≤3.2 MHz'e böl → 120 MHz. (Kristal XOSC0 tutmazsa XOSC1.)
4. **SERCOM2 USART:** PB25/PB24, 115200 → UART testi (yukarıdaki polling main.c) ile clock'u doğrula.
5. **CAN1:** FD modu, bit hızları, PB12/PB13/PC13, transceiver.
6. **Generate → Build → Debug: Start Debugging → Continue → Serial Monitor.**
