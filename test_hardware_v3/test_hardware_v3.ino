/*
  test_hardware_v3.ino
  ------------------------------------------------------------
  TEST PHẦN CỨNG v3 — Thêm PCM5102A (DAC âm thanh I2S)
  Board: Freenove ESP32-S3 WROOM CAM (N16R8)
  Màn hình: ILI9341 2.4" SPI
  Mic: INMP441
  Nút nhấn: GPIO 47
  Loa: PCM5102A (I2S DAC) → cáp AUX → loa Bluetooth

  CÀI THƯ VIỆN TRƯỚC:
  - Arduino IDE → Library Manager → tìm "LovyanGFX" → Install
  - Boards Manager → "esp32 by Espressif Systems" >= 3.x
  - Tools → Board → "ESP32S3 Dev Module"
  - Tools → PSRAM → "OPI PSRAM"
  - Tools → USB CDC On Boot → "Enabled"
  - Tools → Flash Size → "16MB"
  ------------------------------------------------------------
*/

#include <LovyanGFX.hpp>
#include "driver/i2s.h"
#include <math.h>

// ==================== CẤU HÌNH CHÂN ====================
// ILI9341 SPI
#define TFT_MOSI   11
#define TFT_SCK    12
#define TFT_CS     10
#define TFT_DC     9
#define TFT_RST    8

// Mic INMP441 (I2S RX)
#define MIC_SCK    16
#define MIC_WS     17
#define MIC_SD     18

// PCM5102A (I2S TX)
#define SPK_BCK    7    // Bit Clock
#define SPK_WS     15   // Word Select (LRC)
#define SPK_DOUT   14   // Data Out

// Nút nhấn
#define BTN_PIN    47

// I2S ports
#define I2S_MIC_PORT  I2S_NUM_0
#define I2S_SPK_PORT  I2S_NUM_1

#define SAMPLE_RATE   16000

// ==================== CẤU HÌNH MÀN HÌNH ====================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.pin_sclk = TFT_SCK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = -1;
      cfg.pin_dc   = TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs  = TFT_CS;
      cfg.pin_rst = TFT_RST;
      cfg.panel_width  = 240;
      cfg.panel_height = 320;
      cfg.invert = true;
      cfg.readable = false;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

LGFX tft;

// ==================== SETUP MIC ====================
void setupMic() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pins = {
    .bck_io_num = MIC_SCK,
    .ws_io_num = MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_SD
  };
  i2s_driver_install(I2S_MIC_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_MIC_PORT, &pins);
}

// ==================== SETUP LOA PCM5102A ====================
void setupSpeaker() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pins = {
    .bck_io_num = SPK_BCK,
    .ws_io_num = SPK_WS,
    .data_out_num = SPK_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_SPK_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_SPK_PORT, &pins);
}

// ==================== PHÁT ÂM THANH SINE WAVE ====================
// Phát tone tần số freq_hz trong duration_ms mili giây
void playTone(int freq_hz, int duration_ms, int volume = 20000) {
  int samples = SAMPLE_RATE * duration_ms / 1000;
  int16_t buf[256];
  int written = 0;
  size_t bytesWritten;

  for (int i = 0; i < samples; i++) {
    float angle = 2.0f * M_PI * freq_hz * i / SAMPLE_RATE;
    int16_t sample = (int16_t)(volume * sinf(angle));
    buf[written++] = sample;
    buf[written++] = sample;  // stereo: L + R

    if (written >= 256) {
      i2s_write(I2S_SPK_PORT, buf, sizeof(buf), &bytesWritten, portMAX_DELAY);
      written = 0;
    }
  }
  if (written > 0) {
    i2s_write(I2S_SPK_PORT, buf, written * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
  }
}

// ==================== TEST MÀN HÌNH ====================
void testDisplay() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("=== TEST MAN HINH ===");

  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.println("Neu ban doc duoc dong nay");
  tft.println("=> Man hinh OK!");

  tft.fillRect(10, 90, 60, 40, TFT_RED);
  tft.fillRect(80, 90, 60, 40, TFT_GREEN);
  tft.fillRect(150, 90, 60, 40, TFT_BLUE);

  tft.setCursor(10, 145);
  tft.setTextColor(TFT_YELLOW);
  tft.println("3 o mau: DO - XANH LA - XANH DUONG");
  tft.println("Neu thay du 3 mau => SPI OK!");

  delay(3000);
}

// ==================== TEST MIC ====================
void testMic() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("=== TEST MIC ===");

  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.setTextColor(TFT_CYAN);
  tft.println("Noi to vao mic trong 8 giay...");
  tft.println("Thanh xanh se nhay theo giong noi");

  int32_t buf[256];
  size_t bytesRead;
  unsigned long start = millis();
  int y = 80;
  int barMaxW = 200;

  while (millis() - start < 8000) {
    i2s_read(I2S_MIC_PORT, buf, sizeof(buf), &bytesRead, portMAX_DELAY);
    int samples = bytesRead / sizeof(int32_t);
    long peak = 0;
    for (int i = 0; i < samples; i++) {
      long v = abs(buf[i] >> 14);
      if (v > peak) peak = v;
    }

    int barW = constrain(map(peak, 0, 50000, 0, barMaxW), 0, barMaxW);
    tft.fillRect(20, y, barMaxW, 20, TFT_BLACK);
    tft.fillRect(20, y, barW, 20, TFT_GREEN);
    tft.drawRect(20, y, barMaxW, 20, TFT_WHITE);

    tft.fillRect(20, y + 30, 200, 16, TFT_BLACK);
    tft.setCursor(20, y + 30);
    tft.setTextColor(TFT_WHITE);
    tft.printf("Peak: %ld", peak);

    Serial.printf("Mic level: %ld\n", peak);
    delay(80);
  }

  tft.setCursor(10, y + 60);
  tft.setTextColor(TFT_GREEN);
  tft.println("Neu thanh nhay khi noi => MIC OK!");
  tft.setTextColor(TFT_RED);
  tft.setCursor(10, y + 80);
  tft.println("Neu dung im => kiem tra day SCK/WS/SD");

  delay(3000);
}

// ==================== TEST NÚT NHẤN ====================
void testButton() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("=== TEST NUT ===");

  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.setTextColor(TFT_CYAN);
  tft.println("Nhan nut trong 10 giay...");
  tft.println("Man hinh se doi mau khi nhan");

  unsigned long start = millis();
  int pressCount = 0;
  bool lastState = HIGH;

  while (millis() - start < 10000) {
    bool state = digitalRead(BTN_PIN);
    if (state == LOW && lastState == HIGH) {
      pressCount++;
      tft.fillScreen(TFT_BLUE);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.setCursor(40, 140);
      tft.printf("NHAN! x%d", pressCount);
      Serial.printf("Button pressed! count=%d\n", pressCount);
      delay(200);
    } else if (state == HIGH && lastState == LOW) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(1);
      tft.setCursor(10, 10);
      tft.println("Tha nut... nhan tiep di!");
    }
    lastState = state;
    delay(20);
  }

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(pressCount > 0 ? TFT_GREEN : TFT_RED);
  tft.printf("Nut da nhan %d lan\n", pressCount);
  tft.println(pressCount > 0 ? "=> NUT OK!" : "=> Kiem tra lai day nut!");

  delay(3000);
}

// ==================== TEST LOA PCM5102A ====================
void testSpeaker() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("=== TEST LOA ===");

  tft.setTextSize(1);
  tft.setCursor(10, 45);
  tft.setTextColor(TFT_CYAN);
  tft.println("Phat 3 tieng beep...");
  tft.println("Ban nen nghe thay am thanh qua loa.");
  tft.println("");
  tft.setTextColor(TFT_YELLOW);
  tft.println("Neu im tieng:");
  tft.println(" - Kiem tra XSMT co noi 3.3V chua?");
  tft.println(" - Kiem tra cap AUX cam vao AUX IN?");
  tft.println(" - Bat loa Bluetooth len?");
  tft.println(" - Kiem tra BCK=7 WS=15 DIN=14?");

  Serial.println("Phat tone 1000Hz...");
  tft.setCursor(10, 155);
  tft.setTextColor(TFT_WHITE);
  tft.println(">> Dang phat BEEP 1 (1000Hz)...");
  playTone(1000, 800);
  delay(300);

  tft.fillRect(0, 155, 320, 10, TFT_BLACK);
  tft.setCursor(10, 155);
  tft.println(">> Dang phat BEEP 2 (1500Hz)...");
  Serial.println("Phat tone 1500Hz...");
  playTone(1500, 800);
  delay(300);

  tft.fillRect(0, 155, 320, 10, TFT_BLACK);
  tft.setCursor(10, 155);
  tft.println(">> Dang phat BEEP 3 (800Hz)...");
  Serial.println("Phat tone 800Hz...");
  playTone(800, 800);
  delay(300);

  tft.fillRect(0, 155, 320, 10, TFT_BLACK);
  tft.setCursor(10, 155);
  tft.setTextColor(TFT_GREEN);
  tft.println(">> Xong! Nghe thay am thanh chua?");
  Serial.println("Da phat xong 3 tieng beep.");

  delay(4000);
}

// ==================== KẾT QUẢ TỔNG HỢP ====================
void showSummary() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("=== KET QUA ===");

  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.setTextColor(TFT_GREEN);
  tft.println("[OK] Man hinh ILI9341 SPI");
  tft.println("");
  tft.setTextColor(TFT_WHITE);
  tft.println("[??] Mic INMP441");
  tft.println("     (thanh co nhay khong?)");
  tft.println("");
  tft.println("[??] Nut nhan GPIO 47");
  tft.println("     (co nhan duoc khong?)");
  tft.println("");
  tft.println("[??] Loa PCM5102A");
  tft.println("     (nghe thay 3 beep khong?)");
  tft.println("");
  tft.setTextColor(TFT_CYAN);
  tft.println("-----------------------------");
  tft.println("Neu tat ca OK => san sang");
  tft.println("flash firmware Xiaozhi!");
}

// ==================== SETUP & LOOP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TEST PHAN CUNG KIOSK v3 ===");
  Serial.println("Board: Freenove ESP32-S3 CAM N16R8");
  Serial.println("Display: ILI9341 | Mic: INMP441 | Loa: PCM5102A");
  Serial.println("");

  pinMode(BTN_PIN, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.println("KIOSK AI TUYEN SINH");
  tft.setTextSize(1);
  tft.setCursor(20, 130);
  tft.println("Dang khoi dong test v3...");
  delay(2000);

  setupMic();
  setupSpeaker();

  Serial.println(">>> Test 1: Man hinh");
  testDisplay();

  Serial.println(">>> Test 2: Mic");
  testMic();

  Serial.println(">>> Test 3: Nut nhan");
  testButton();

  Serial.println(">>> Test 4: Loa PCM5102A");
  testSpeaker();

  Serial.println(">>> Tong ket");
  showSummary();

  Serial.println("\n=== HOAN TAT TEST PHAN CUNG v3 ===");
}

void loop() {
  // de trong - chi chay test 1 lan
}
