# Kế hoạch Prototype — Kiosk AI Tư vấn Tuyển sinh

## Linh kiện THỰC TẾ đang có

| # | Linh kiện | Ghi chú |
|---|---|---|
| 1 | **ESP32-S3-WROOM N16R8 CAM** (breadboard) | 16MB Flash / 8MB PSRAM, có khe camera (không dùng camera trong prototype) |
| 2 | **ILI9341 2.4" SPI** | Thay thế OLED SSD1306. Màn hình màu 320x240, hiển thị được câu hỏi/trả lời |
| 3 | **INMP441** | Mic I2S |
| 4 | **Nút nhấn** | 1 nút |
| 5 | **Loa Bluetooth** | Kết nối qua WiFi/server, KHÔNG dùng DAC PCM5102A |

---

## SƠ ĐỒ ĐẤU DÂY (ESP32-S3 CAM N16R8)

> **QUAN TRỌNG:** Board CAM có camera chiếm ~20 GPIO. Prototype này KHÔNG dùng camera,
> nên ta tận dụng các chân đó. GPIO 26-37 vẫn bị PSRAM chiếm, TUYỆT ĐỐI không dùng.

### ILI9341 2.4" SPI
| Chân ILI9341 | Nối tới | Ghi chú |
|---|---|---|
| VCC | 3.3V | |
| GND | GND | |
| CS | GPIO 42 | |
| RST (RESET) | GPIO 46 | Strapping pin, an toàn làm output sau boot |
| DC (RS) | GPIO 41 | |
| MOSI (SDA/SDI) | GPIO 1 | SPI data |
| SCK (CLK) | GPIO 2 | SPI clock |
| LED (BL) | 3.3V | Backlight luôn sáng |
| MISO | Không nối | Không cần đọc từ màn hình |

### Mic INMP441
| Chân INMP441 | Nối tới | Ghi chú |
|---|---|---|
| SCK | GPIO 43 | ⚠️ Đây là UART0 TX — Serial sẽ dùng USB CDC thay thế |
| WS | GPIO 44 | ⚠️ Đây là UART0 RX |
| SD | GPIO 21 | |
| VDD | 3.3V | |
| GND | GND | |
| L/R | GND | Kênh trái |

### Nút nhấn
| Chân | Nối tới | Ghi chú |
|---|---|---|
| 1 chân | GPIO 45 | Strapping pin, an toàn làm input sau boot |
| Chân còn lại | GND | Dùng INPUT_PULLUP trong code |

### ⚠️ Lưu ý quan trọng khi cài đặt Arduino IDE
Vì GPIO 43/44 (UART0) được dùng cho mic, bạn PHẢI:
- Tools → **USB CDC On Boot** → **Enabled**
- Điều này cho phép Serial Monitor hoạt động qua USB thay vì UART0

---

## THỨ TỰ TRIỂN KHAI PROTOTYPE

### Bước 1: Test phần cứng (`test_hardware_v2.ino`)
1. Mở Arduino IDE
2. Cài thư viện: **LovyanGFX** (Library Manager)
3. Cài board: **esp32 by Espressif Systems** (Boards Manager)
4. Tools → Board → **ESP32S3 Dev Module**
5. Tools → PSRAM → **OPI PSRAM**
6. Tools → USB CDC On Boot → **Enabled**
7. Upload `test_hardware_v2.ino`, xem kết quả trên màn hình ILI9341
8. Chỉ sang bước 2 khi MÀN HÌNH + MIC + NÚT đều OK

### Bước 2: Thiết lập Supabase
1. Tạo project free tier tại supabase.com
2. Chạy `supabase_schema.sql` trong SQL Editor
3. Lấy Project URL, anon key, service_role key

### Bước 3: Thiết lập Dify
1. Tạo Dify app (Cloud free hoặc self-host)
2. Upload `knowledge_base_mau.md` (đã sửa thông tin trường thật) vào Knowledge Base
3. Dán System Prompt "đanh đá"
4. Lấy API Key + Base URL

### Bước 4: Chạy `dify_bridge.py`
1. Tạo file `.env` với đủ key
2. `pip install flask requests python-dotenv`
3. `python dify_bridge.py`
4. Test: `curl http://localhost:8080/health`

### Bước 5: Mở Dashboard
1. Sửa `SUPABASE_URL` và `SUPABASE_ANON_KEY` trong `dashboard.html`
2. Mở file trên trình duyệt → thấy câu hỏi + câu trả lời realtime

### Bước 6: Firmware Kiosk chính (kết nối toàn chuỗi)
- Build firmware Xiaozhi hoặc viết firmware custom kết nối WiFi → gửi audio lên server → nhận text → hiển thị lên ILI9341 + phát TTS qua loa BT

---

## ARCHITECTURE PROTOTYPE

```
[Học sinh nói] → [INMP441 mic]
                      ↓
              [ESP32-S3 CAM]  ← [Nút nhấn giữ-để-nói]
                      ↓
              Gửi audio qua WiFi
                      ↓
          [xiaozhi-esp32-server]
           (STT: Whisper API)
                      ↓
            [dify_bridge.py]
             ├─ Gọi Dify (KB + prompt)
             ├─ Gọi Gemini (phân loại)
             └─ Ghi Supabase
                      ↓
          Trả câu trả lời về ESP32
                      ↓
        ┌─────────────────────────────┐
        │ ILI9341: hiện câu hỏi/đáp  │
        │ Loa BT: phát giọng TTS     │
        └─────────────────────────────┘
                      ↓
          [dashboard.html trên TV/laptop]
           Supabase Realtime → hiện Q&A
```

---

## VỀ KNOWLEDGE BASE

File `knowledge_base_mau.md` đã được tạo sẵn. Bạn cần:

1. **Thay thế tất cả `[...]`** bằng thông tin thật của trường
2. Các mục QUAN TRỌNG NHẤT cần điền chính xác:
   - Tên trường, địa chỉ, hotline
   - Danh sách ngành + mã ngành
   - Học phí cụ thể từng ngành
   - Điểm chuẩn năm gần nhất
   - Học bổng + điều kiện
   - Thời gian nhận hồ sơ
3. **TUYỆT ĐỐI không bịa số liệu** — nếu chưa có, để trống hoặc ghi "đang cập nhật"
4. Upload file đã sửa vào Dify → Knowledge Base → Dify tự chunk + embedding
