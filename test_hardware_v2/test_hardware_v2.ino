/*
  test_hardware_v2.ino
  ------------------------------------------------------------
  TEST PHẦN CỨNG PROTOTYPE
  Board: Freenove ESP32-S3 WROOM CAM (N16R8)
  Màn hình: ILI9341 2.4" SPI
  Mic: INMP441
  Nút nhấn: dùng nút BOOT có sẵn trên board (GPIO 0)

  KHÔNG cắm camera OV2640 → các chân camera (4-18) đều trống,
  ta tận dụng cho ILI9341 và INMP441.

  CÀI THƯ VIỆN TRƯỚC:
  - Arduino IDE → Library Manager → tìm "LovyanGFX" → Install
  - Boards Manager → "esp32 by Espressif Systems" ≥ 3.x
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
// ILI9341 SPI (dùng chân camera freed)
#define TFT_MOSI   11   // SPI MOSI
#define TFT_SCK    12   // SPI CLK
#define TFT_CS     10   // Chip Select
#define TFT_DC     9    // Data/Command
#define TFT_RST    8    // Reset
// BL (backlight) → nối thẳng 3.3V (luôn sáng)
// MISO → không nối

// Mic INMP441 (I2S RX, dùng chân camera freed)
#define MIC_SCK    16   // BCLK
#define MIC_WS     17   // LRCLK
#define MIC_SD     18   // DATA OUT từ mic

// Nút nhấn 4 chân (tactile switch) — nối riêng
#define BTN_PIN    47   // 1 chân nút → GPIO 47, chân còn lại → GND

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
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.readable = false;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

LGFX tft;

#define SAMPLE_RATE   16000
#define I2S_MIC_PORT  I2S_NUM_0

// ==================== HÀM SETUP MIC ====================
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

  bool micOK = true;
  tft.setCursor(10, y + 60);
  if (micOK) {
    tft.setTextColor(TFT_GREEN);
    tft.println("Neu thanh nhay khi noi => MIC OK!");
  }
  tft.setTextColor(TFT_RED);
  tft.setCursor(10, y + 80);
  tft.println("Neu dung im => kiem tra day");
  tft.println("SCK/WS/SD va chieu cam INMP441");

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
  tft.println("[??] Mic INMP441");
  tft.println("     (thanh co nhay khong?)");
  tft.println("");
  tft.println("[??] Nut nhan");
  tft.println("     (co nhan duoc khong?)");
  tft.println("");
  tft.setTextColor(TFT_CYAN);
  tft.println("-----------------------------");
  tft.println("Neu tat ca OK => san sang");
  tft.println("chuyen sang firmware chinh!");
  tft.println("");
  tft.setTextColor(TFT_WHITE);
  tft.println("LUU Y: Prototype nay KHONG");
  tft.println("dung DAC (loa qua WiFi/BT)");
  tft.println("nen khong can test loa o day.");
}

// ==================== SETUP & LOOP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TEST PHAN CUNG KIOSK v2 ===");
  Serial.println("Board: Freenove ESP32-S3 CAM N16R8");
  Serial.println("Display: ILI9341 2.4\" SPI");
  Serial.println("Mic: INMP441");
  Serial.println("Button: GPIO 47 (nut nhan 4 chan)");
  Serial.println("");

  pinMode(BTN_PIN, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);  // ngang, 320x240
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.println("KIOSK AI TUYEN SINH");
  tft.setTextSize(1);
  tft.setCursor(20, 130);
  tft.println("Dang khoi dong test...");
  delay(2000);

  setupMic();

  Serial.println(">>> Test 1: Man hinh");
  testDisplay();

  Serial.println(">>> Test 2: Mic");
  testMic();

  Serial.println(">>> Test 3: Nut nhan");
  testButton();

  Serial.println(">>> Tong ket");
  showSummary();

  Serial.println("\n=== HOAN TAT TEST PHAN CUNG ===");
}

void loop() {
  // de trong - chi chay test 1 lan
}
