/*
  test_hardware.ino
  ------------------------------------------------------------
  MỤC ĐÍCH: Test độc lập mic INMP441 + DAC PCM5102A TRƯỚC KHI
  build firmware Xiaozhi (vốn phức tạp hơn nhiều). Nếu sketch
  này chạy đúng (thấy mức âm thanh nhảy số khi bạn nói, nghe
  được tiếng "bíp" qua loa Bluetooth), nghĩa là phần cứng/dây
  nối ổn — mọi lỗi sau này chắc chắn nằm ở phần mềm, không phải
  do hàn/đấu dây sai.

  CÀI TRƯỚC: Arduino IDE + gói "esp32" by Espressif Systems
  (Boards Manager). Chọn đúng board: "ESP32 Dev Module" (ESP32
  thường) hoặc "ESP32S3 Dev Module" (ESP32-S3) tuỳ bạn dùng board nào.

  SỬA CÁC SỐ CHÂN BÊN DƯỚI cho khớp với dây bạn đã đấu thật.
  ------------------------------------------------------------
*/

#include "driver/i2s.h"
#include <math.h>

// ====== ĐÃ CẬP NHẬT CHO ESP32-S3 N16R8 (né vùng PSRAM/Flash GPIO 26-37) ======
// Mic INMP441 (I2S số 0, chiều RX - nhận)
#define MIC_SCK_PIN   4    // SCK/BCLK của mic
#define MIC_WS_PIN    5    // WS/LRC của mic
#define MIC_SD_PIN    6    // SD của mic

// DAC PCM5102 / PCM5102A (I2S số 1, chiều TX - phát) - 2 chip pin-to-pin giống nhau
#define SPK_BCK_PIN   7    // BCK của DAC
#define SPK_LCK_PIN   15   // LCK của DAC
#define SPK_DIN_PIN   16   // DIN của DAC
// Board: chọn "ESP32S3 Dev Module" trong Tools > Board
// ============================================================

#define SAMPLE_RATE   16000
#define I2S_MIC_PORT  I2S_NUM_0
#define I2S_SPK_PORT  I2S_NUM_1

void setupMic() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 xuất 32-bit khung, dữ liệu thật 24-bit
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // INMP441 thường nối kênh trái (L/R -> GND)
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pins = {
    .bck_io_num = MIC_SCK_PIN,
    .ws_io_num = MIC_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_SD_PIN
  };
  i2s_driver_install(I2S_MIC_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_MIC_PORT, &pins);
}

void setupSpeaker() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pins = {
    .bck_io_num = SPK_BCK_PIN,
    .ws_io_num = SPK_LCK_PIN,
    .data_out_num = SPK_DIN_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_SPK_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_SPK_PORT, &pins);
}

// Bước 1: đọc mic 5 giây, in ra mức âm thanh (giá trị càng lớn khi bạn nói càng to)
void testMic() {
  Serial.println("\n=== TEST 1: MIC INMP441 ===");
  Serial.println("Nói to vào mic trong 5 giây, quan sát mức âm thanh bên dưới...");
  int32_t buf[256];
  size_t bytesRead;
  unsigned long start = millis();
  while (millis() - start < 5000) {
    i2s_read(I2S_MIC_PORT, buf, sizeof(buf), &bytesRead, portMAX_DELAY);
    int samples = bytesRead / sizeof(int32_t);
    long peak = 0;
    for (int i = 0; i < samples; i++) {
      long v = abs(buf[i] >> 14); // dịch bit để ra biên độ dễ đọc
      if (v > peak) peak = v;
    }
    // Vẽ thanh mức đơn giản bằng ký tự '#'
    int bars = constrain(map(peak, 0, 50000, 0, 40), 0, 40);
    Serial.print("Mic level: [");
    for (int i = 0; i < 40; i++) Serial.print(i < bars ? '#' : ' ');
    Serial.print("] ");
    Serial.println(peak);
    delay(100);
  }
  Serial.println("Nếu thanh '#' gần như không nhúc nhích dù bạn nói to -> kiểm tra lại dây SCK/WS/SD hoặc chiều cắm INMP441 (dễ ngược VDD/GND).");
}

// Bước 2: phát một tiếng "bíp" 440Hz trong 2 giây qua PCM5102A -> loa Bluetooth
void testSpeaker() {
  Serial.println("\n=== TEST 2: DAC PCM5102A + LOA ===");
  Serial.println("Sẽ phát tiếng bíp 440Hz trong 2 giây. Nếu KHÔNG nghe gì, kiểm tra XSMT (phải nối 3.3V, không phải GND)!");
  const int freq = 440;
  const int durationMs = 2000;
  const int totalSamples = SAMPLE_RATE * durationMs / 1000;
  int16_t sampleBuf[2]; // stereo: [trái, phải]
  size_t bytesWritten;
  for (int i = 0; i < totalSamples; i++) {
    float t = (float)i / SAMPLE_RATE;
    int16_t s = (int16_t)(sin(2 * PI * freq * t) * 8000); // biên độ vừa phải, tránh rè
    sampleBuf[0] = s;
    sampleBuf[1] = s;
    i2s_write(I2S_SPK_PORT, sampleBuf, sizeof(sampleBuf), &bytesWritten, portMAX_DELAY);
  }
  Serial.println("Đã phát xong.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== BẮT ĐẦU TEST PHẦN CỨNG KIOSK ===");
  setupMic();
  setupSpeaker();
  testMic();
  testSpeaker();
  Serial.println("\n=== HOÀN TẤT. Mở Serial Monitor (115200 baud) để xem kết quả. ===");
}

void loop() {
  // để trống - chỉ chạy test 1 lần trong setup()
}
