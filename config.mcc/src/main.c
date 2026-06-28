#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "definitions.h"

/* =========================================================================
 *  İki kart CAN demosu
 *  - Bir kart TEK sayıları (1,3,5..), diğeri ÇİFT sayıları (2,4,6..) yollar.
 *  - Her kart ALDIĞI mesajı, kaç ms'de aldığını (zaman damgası) ile UART'a basar.
 *
 *  KART A için aşağıyı  1  bırak (TEK yayıncı).
 *  KART B'ye yüklemeden önce  0  yap (ÇİFT yayıncı), tekrar Build et.
 * ========================================================================= */
#define BOARD_SENDS_ODD   0

#if BOARD_SENDS_ODD
  #define MY_TX_ID    0x100U            /* tek sayı yayıncısının kimliği */
  #define ROLE_NAME   "TEK (1,3,5..)"
  static uint32_t txValue = 1U;
#else
  #define MY_TX_ID    0x200U            /* çift sayı yayıncısının kimliği */
  #define ROLE_NAME   "CIFT (2,4,6..)"
  static uint32_t txValue = 2U;
#endif

#define TX_PERIOD_MS  1000U             /* her 1 sn'de bir gönder */

/* CAN denetleyicisi mesajlarını bu RAM'de tutar (TX/RX öncesi tanıtılmalı).
 * 32-bayt hizalı olmalı. */
static uint8_t Can1MessageRAM[CAN1_MESSAGE_RAM_CONFIG_SIZE] __attribute__((aligned(32)));

/* ---- 1 ms sistem sayacı (SysTick) ---- */
static volatile uint32_t g_ms = 0U;
void SysTick_Handler(void)
{
    g_ms++;
}

/* ---- RX paylaşımlı değişkenler (callback ISR içinde yazılır) ---- */
static volatile bool     rxFlag = false;
static volatile uint32_t rxAtMs = 0U;   /* mesajın TAM geliş anı (ms) */
static uint32_t rxId   = 0U;
static uint8_t  rxLen  = 0U;
static uint8_t  rxData[64];
static uint16_t rxTs   = 0U;
static CAN_MSG_RX_FRAME_ATTRIBUTE rxFrameAttr = CAN_MSG_RX_DATA_FRAME;

/* ===== DONANIM AES-128 (ECB, tek blok) =====
 * SAM E54 dahili AES cevre birimini dogrudan register'dan surer.
 * Olculen is: gercek donanim AES-128 blok suresi (decrypt). */
static const uint8_t AES_KEY[16] = {
    0x2BU,0x7EU,0x15U,0x16U, 0x28U,0xAEU,0xD2U,0xA6U,
    0xABU,0xF7U,0x15U,0x88U, 0x09U,0xCFU,0x4FU,0x3CU
};

static uint32_t ld_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void st_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void aes_hw_init(void)
{
    MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_AES_Msk;   /* AES APB clock'unu ac */
}

/* Mod + anahtar ayarla: encrypt=true sifrele, false coz */
static void aes_cfg(const uint8_t key[16], bool encrypt)
{
    AES_REGS->AES_CTRLA = 0U;                            /* devre disi (yeniden ayar icin) */
    AES_REGS->AES_CTRLA = AES_CTRLA_AESMODE_ECB
                        | AES_CTRLA_KEYSIZE_128BIT
                        | (encrypt ? AES_CTRLA_CIPHER_ENC : AES_CTRLA_CIPHER_DEC)
                        | AES_CTRLA_STARTMODE_MANUAL;
    AES_REGS->AES_CTRLA |= AES_CTRLA_ENABLE_Msk;
    for (int i = 0; i < 4; i++) { AES_REGS->AES_KEYWORD[i] = ld_le32(key + (4 * i)); }
}

/* Tek 16-baytlik blogu isle (moda gore sifrele/coz). OLCULEN cekirdek is budur. */
static void aes_block(const uint8_t in[16], uint8_t out[16])
{
    AES_REGS->AES_INTFLAG    = AES_INTFLAG_ENCCMP_Msk;   /* eski bayragi temizle */
    AES_REGS->AES_CTRLB      = AES_CTRLB_NEWMSG_Msk;     /* yeni mesaj */
    AES_REGS->AES_DATABUFPTR = 0U;
    for (int i = 0; i < 4; i++) { AES_REGS->AES_INDATA = ld_le32(in + (4 * i)); }
    AES_REGS->AES_CTRLB     |= AES_CTRLB_START_Msk;      /* basla */
    while ((AES_REGS->AES_INTFLAG & AES_INTFLAG_ENCCMP_Msk) == 0U) { }
    AES_REGS->AES_DATABUFPTR = 0U;
    for (int i = 0; i < 4; i++) { st_le32(out + (4 * i), AES_REGS->AES_INDATA); }
}

/* Bir blok islemesini 3 parcaya ayirarak olcer:
 *  cin   = GIRIS I/O   (4 kelime veriyi AES'e yazma)
 *  ccore = CEKIRDEK    (START -> ENCCMP: AES turlarinin gercek hesabi)
 *  cout  = CIKIS I/O   (4 kelime sonucu okuma)
 * DWT calismis olmali (dwt_init). Kesmeler cagiran tarafindan kapatilmali. */
typedef struct { uint32_t cin; uint32_t ccore; uint32_t cout; } aes_timing_t;

static void aes_block_timed(const uint8_t in[16], uint8_t out[16], aes_timing_t *t)
{
    AES_REGS->AES_INTFLAG    = AES_INTFLAG_ENCCMP_Msk;
    AES_REGS->AES_CTRLB      = AES_CTRLB_NEWMSG_Msk;
    AES_REGS->AES_DATABUFPTR = 0U;

    uint32_t a = DWT->CYCCNT;
    for (int i = 0; i < 4; i++) { AES_REGS->AES_INDATA = ld_le32(in + (4 * i)); }   /* GIRIS I/O */
    uint32_t b = DWT->CYCCNT;

    AES_REGS->AES_CTRLB |= AES_CTRLB_START_Msk;                                     /* tetik */
    while ((AES_REGS->AES_INTFLAG & AES_INTFLAG_ENCCMP_Msk) == 0U) { }              /* CEKIRDEK */
    uint32_t c = DWT->CYCCNT;

    AES_REGS->AES_DATABUFPTR = 0U;
    for (int i = 0; i < 4; i++) { st_le32(out + (4 * i), AES_REGS->AES_INDATA); }   /* CIKIS I/O */
    uint32_t d = DWT->CYCCNT;

    t->cin   = b - a;
    t->ccore = c - b;
    t->cout  = d - c;
}

/* FIPS-197 test vektoruyle donanim AES dogrulamasi (acilista bir kez).
 * Eslesirse donanim AES'in dogru calistigi kanitlanir. */
static bool aes_selftest(void)
{
    static const uint8_t k[16]  = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static const uint8_t pt[16] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    static const uint8_t ct[16] = {0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a};
    uint8_t tmp[16], back[16];
    aes_cfg(k, true);  aes_block(pt, tmp);     /* encrypt PT -> tmp, ct olmali */
    aes_cfg(k, false); aes_block(ct, back);    /* decrypt CT -> back, pt olmali */
    return (memcmp(tmp, ct, 16) == 0) && (memcmp(back, pt, 16) == 0);
}

/* ===== ADIM 1: DWT cevrim sayaci (ns cozunurluklu sure olcumu) ===== */
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* izlemeyi ac */
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;             /* cevrim sayacini baslat */
}
static uint32_t cycles_to_ns(uint32_t cyc)
{
    return (uint32_t)(((uint64_t)cyc * 1000000000ULL) / (uint64_t)CPU_CLOCK_FREQUENCY);
}

static void uart_print(const char *s)
{
    while (*s != '\0')
    {
        /* Veri register'i (DRE) boşalana kadar bekle */
        while ((SERCOM2_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) == 0U)
        {
        }
        SERCOM2_REGS->USART_INT.SERCOM_DATA = (uint8_t)(*s);
        s++;
    }
    /* Son bayt tamamen gönderilene kadar bekle (TXC) */
    while ((SERCOM2_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk) == 0U)
    {
    }
}

/* Mesaj gelince CAN kesmesinden (ISR) çağrılır.
 * Burada sadece geliş anını yakala + bayrak set et; ağır iş (UART) ana döngüde. */
static void canRxCallback(uintptr_t context)
{
    (void)context;
    rxAtMs = g_ms;       /* mesajın geldiği anı kaydet */
    rxFlag = true;
}

/* Tek seferlik RX'i bir sonraki mesaj için yeniden silahlandır.
 * (Sürücü her alımdan sonra RX kesmesini kapatıyor.) */
static void armReceive(void)
{
    /* Filtre kurulu olmadığından gelen tüm standart çerçeveler RX FIFO1'e düşer
     * (CAN GFC ayarı: ANFS_RXF1). Bu yüzden FIFO1'den okuyoruz. */
    (void)CAN1_MessageReceive(&rxId, &rxLen, rxData, &rxTs,
                              CAN_MSG_ATTR_RX_FIFO1, &rxFrameAttr);
}

/* ===== AES benchmark: N tekrar, min/ortalama/max (toplam + cekirdek) =====
 * encrypt=true sifrele, false coz. Sonucu UART'a basar. */
#define BENCH_N   1000U
static void aes_bench_dir(const char *label, bool encrypt)
{
    aes_cfg(AES_KEY, encrypt);

    uint8_t in[16]  = {0};
    uint8_t out[16];
    aes_timing_t tt;

    for (int w = 0; w < 8; w++) { aes_block(in, out); }   /* isinma */

    uint32_t tmin = 0xFFFFFFFFU, tmax = 0U;
    uint64_t tsum = 0U, csum = 0U;

    for (uint32_t i = 0U; i < BENCH_N; i++)
    {
        __disable_irq();
        aes_block_timed(in, out, &tt);
        __enable_irq();

        uint32_t tot = tt.cin + tt.ccore + tt.cout;
        if (tot < tmin) { tmin = tot; }
        if (tot > tmax) { tmax = tot; }
        tsum += tot;
        csum += tt.ccore;

        (void)memcpy(in, out, 16);   /* sonucu girdiye besle: optimize edilmesin + veri degissin */
    }

    uint32_t tmean = (uint32_t)(tsum / BENCH_N);
    uint32_t cmean = (uint32_t)(csum / BENCH_N);

    char b[200];
    (void)sprintf(b, "%s (N=%lu): toplam min/ort/max = %lu/%lu/%lu ns | cekirdek ort = %lu ns\r\n",
                  label, (unsigned long)BENCH_N,
                  (unsigned long)cycles_to_ns(tmin), (unsigned long)cycles_to_ns(tmean),
                  (unsigned long)cycles_to_ns(tmax), (unsigned long)cycles_to_ns(cmean));
    uart_print(b);
}

int main(void)
{
    SYS_Initialize(NULL);

    /* CAN mesaj RAM'ini tanıt (gönderme/almadan ÖNCE şart) */
    CAN1_MessageRAMConfigSet(Can1MessageRAM);

    /* 1 ms tick (CPU = 48 MHz) */
    (void)SysTick_Config(CPU_CLOCK_FREQUENCY / 1000U);

    /* ns cozunurluklu sure olcumu icin DWT cevrim sayacini ac */
    dwt_init();

    /* Donanim AES'i baslat + FIPS-197 ile dogrula, sonra RX olcumu icin decrypt moduna al */
    aes_hw_init();
    bool aesOk = aes_selftest();
    aes_cfg(AES_KEY, false);     /* decrypt modu (RX'te blok cozumu olculecek) */

    /* Gelen mesaj için callback + ilk silahlandırma */
    CAN1_RxCallbackRegister(canRxCallback, 0U, CAN_MSG_ATTR_RX_FIFO1);
    armReceive();

    char buf[200];
    (void)sprintf(buf, "\r\n=== SAM E54 CAN demo | rol: %s | AES HW self-test: %s ===\r\n",
                  ROLE_NAME, aesOk ? "OK" : "FAIL");
    uart_print(buf);

    /* Acilista AES-128 benchmark: encrypt + decrypt, N tekrar ortalama */
    uart_print("--- AES-128 benchmark (donanim) ---\r\n");
    aes_bench_dir("ENCRYPT", true);
    aes_bench_dir("DECRYPT", false);
    aes_cfg(AES_KEY, false);     /* RX olcumu icin decrypt moduna geri don */

    uint32_t lastTx = 0U;

    while (true)
    {
        SYS_Tasks();

        /* ---------------- Periyodik gönderim ---------------- */
        if ((g_ms - lastTx) >= TX_PERIOD_MS)
        {
            lastTx = g_ms;

            /* 64 baytlik plaintext kur: ilk 4 bayt sayac, gerisi desen (4 AES blogu) */
            uint8_t payload[64];
            for (int j = 0; j < 64; j++) { payload[j] = (uint8_t)j; }
            payload[0] = (uint8_t)(txValue & 0xFFU);
            payload[1] = (uint8_t)((txValue >> 8)  & 0xFFU);
            payload[2] = (uint8_t)((txValue >> 16) & 0xFFU);
            payload[3] = (uint8_t)((txValue >> 24) & 0xFFU);

            /* 4 blogu AES ile YERINDE sifrele, sonra 64 bayti gonder */
            aes_cfg(AES_KEY, true);                 /* encrypt modu */
            for (int b = 0; b < 4; b++) { aes_block(payload + (16 * b), payload + (16 * b)); }

            /* CAN FD, BRS YOK -> 64 bayt, dusuk (50k) hizda; termination gerekmez */
            bool txOk = CAN1_MessageTransmit(MY_TX_ID, 64U, payload,
                                             CAN_MODE_FD_WITHOUT_BRS,
                                             CAN_MSG_ATTR_TX_FIFO_DATA_FRAME);

            /* --- CAN hata teşhisi --- */
            uint8_t tec = 0U, rec = 0U;
            CAN1_ErrorCountGet(&tec, &rec);
            CAN_ERROR err = CAN1_ErrorGet();   /* ayrıca bus-off ise INIT'i temizler (auto-recover) */
            uint32_t lec = (uint32_t)err & 0x7U;

            const char *durum = "";
            if ((err & CAN_ERROR_BUS_OFF) != 0U)        { durum = " >> BUS-OFF (hat/ACK yok)"; }
            else if (lec == CAN_ERROR_LEC_ACK)          { durum = " >> ACK-YOK (karsi dugum yok?)"; }
            else if ((err & CAN_ERROR_PASSIVE) != 0U)   { durum = " >> ERROR-PASSIVE"; }
            else if ((err & CAN_ERROR_WARNING_STATUS) != 0U) { durum = " >> WARNING"; }

            if (txOk)
            {
                (void)sprintf(buf, "[t=%lu ms] TX id=0x%lX deger=%lu | TEC=%u REC=%u%s\r\n",
                              (unsigned long)g_ms, (unsigned long)MY_TX_ID,
                              (unsigned long)txValue, (unsigned)tec, (unsigned)rec, durum);
                txValue += 2U;            /* tek->tek, çift->çift kalır */
            }
            else
            {
                (void)sprintf(buf, "[t=%lu ms] TX-FIFO DOLU (gonderilemiyor) | TEC=%u REC=%u%s\r\n",
                              (unsigned long)g_ms, (unsigned)tec, (unsigned)rec, durum);
            }
            uart_print(buf);
        }

        /* ---------------- Gelen mesaj ---------------- */
        if (rxFlag)
        {
            rxFlag = false;

            /* Gelen 64 bayt = 4 AES blogu (gercek ciphertext). Hepsini COZ + sureyi olc. */
            uint8_t plain[64];
            uint8_t nblk = (uint8_t)(rxLen / 16U);          /* normalde 4 (64 bayt) */
            if (nblk == 0U) { nblk = 1U; }
            if (nblk > 4U)  { nblk = 4U; }

            aes_cfg(AES_KEY, false);                        /* decrypt modu (olcum disinda) */

            /* === DECRYPT SURESINI OLC (ns) — nblk blogu, cekirdek + I/O ayri ===
             * Kesmeleri kapat ki araya CAN/SysTick girip olcumu sismesin. */
            aes_timing_t tt;
            uint32_t cin = 0U, ccore = 0U, cout = 0U;
            __disable_irq();
            for (uint8_t b = 0U; b < nblk; b++)
            {
                aes_block_timed(rxData + (16U * b), plain + (16U * b), &tt);
                cin += tt.cin; ccore += tt.ccore; cout += tt.cout;
            }
            __enable_irq();

            uint32_t ns_core = cycles_to_ns(ccore);
            uint32_t ns_io   = cycles_to_ns(cin + cout);
            uint32_t ns_tot  = cycles_to_ns(cin + ccore + cout);

            /* cozulmus sayac (plaintext'in ilk 4 bayti) — gercek uctan uca dogrulama */
            uint32_t value = (uint32_t)plain[0] | ((uint32_t)plain[1] << 8)
                           | ((uint32_t)plain[2] << 16) | ((uint32_t)plain[3] << 24);

            (void)sprintf(buf, "[t=%lu ms] RX deger=%lu | %u blok (%u B) COZ: cekirdek=%lu io=%lu toplam=%lu ns\r\n",
                          (unsigned long)rxAtMs, (unsigned long)value,
                          (unsigned)nblk, (unsigned)rxLen,
                          (unsigned long)ns_core, (unsigned long)ns_io, (unsigned long)ns_tot);
            uart_print(buf);

            armReceive();                 /* sıradaki mesaj için yeniden silahlandır */
        }
    }

    return EXIT_FAILURE;
}
