# Hướng dẫn chi tiết từng phần — Code cụ thể

Tài liệu này đi sâu vào **code thật** cho từng phần, bổ sung cho `KE_HOACH_TRIEN_KHAI.md`. Kèm theo:
- `test_hardware.ino` — sketch Arduino test độc lập mic + DAC (làm TRƯỚC TIÊN)
- `oled_eyes_demo.ino` — demo biểu cảm mắt OLED cho 4 trạng thái (chờ/nghe/xử lý/nói)
- `dify_bridge.py` — script Python nối xiaozhi-esp32-server ↔ Dify ↔ Gemini ↔ Supabase
- `dashboard.html` — bản cập nhật: thêm thẻ thống kê, bảng xếp hạng ngành, danh mục chi tiết hơn (có "Quản trị Cơ sở dữ liệu")
- `supabase_schema.sql` — không đổi

**Thứ tự làm khuyến nghị:** 1 → 2 → 3 → 4 → 5 → 6 (đừng nhảy cóc, mỗi bước xác nhận xong mới qua bước sau — làm vậy dễ định vị lỗi hơn nhiều so với ráp hết rồi mới test).

---

## PHẦN 1 — Test phần cứng độc lập (làm đầu tiên, trước khi đụng Xiaozhi)

Trước khi build firmware Xiaozhi (phức tạp, nhiều lớp trừu tượng), hãy chắc chắn phần cứng hoạt động bằng một sketch Arduino đơn giản, tự viết, không phụ thuộc gì vào Xiaozhi.

### Cách chạy `test_hardware.ino`
1. Mở Arduino IDE → File → Open → chọn `test_hardware.ino`.
2. Boards Manager → cài gói **esp32 by Espressif Systems** (nếu chưa có).
3. Tools → Board → chọn **ESP32 Dev Module** (nếu dùng ESP32 thường) hoặc **ESP32S3 Dev Module** (nếu dùng ESP32-S3).
4. Sửa 6 dòng `#define` đầu file cho khớp với dây bạn đã đấu thật:
   ```cpp
   #define MIC_SCK_PIN   14
   #define MIC_WS_PIN    15
   #define MIC_SD_PIN    32
   #define SPK_BCK_PIN   26
   #define SPK_LCK_PIN   25
   #define SPK_DIN_PIN   22
   ```
5. Cắm board bằng **cáp Type-C loại truyền dữ liệu** (không phải cáp chỉ sạc — nếu Tools → Port không hiện cổng COM nào, 99% là do dùng nhầm cáp sạc).
6. Upload, sau đó mở **Serial Monitor** (Tools → Serial Monitor, chọn baud **115200**).

### Đọc kết quả
- **Test mic:** nói to vào mic — nếu thanh `[####...]` nhảy theo giọng nói → mic ổn. Nếu đứng im → kiểm tra lại dây SCK/WS/SD, và chiều cắm INMP441 (module này dễ hàn/cắm ngược VDD-GND từ shop kém uy tín).
- **Test loa:** sẽ phát tiếng bíp 440Hz 2 giây qua loa Bluetooth. Nếu **im lặng hoàn toàn**, 90% nguyên nhân là chân **XSMT của PCM5102A đang nối GND thay vì 3.3V** (lỗi phổ biến nhất, đã nói ở tài liệu trước) — kiểm tra lại chân này đầu tiên trước khi nghi ngờ gì khác.

Chỉ chuyển sang Phần 2 khi cả 2 test này đều PASS.

---

## PHẦN 2 — Firmware Xiaozhi (chỉnh sửa cấu hình, KHÔNG cần viết code C++)

**Lưu ý quan trọng để bạn khỏi lo lắng thừa:** Xiaozhi đã viết sẵn toàn bộ logic firmware (kết nối WiFi, xử lý nút nhấn giữ-để-nói, ghi âm I2S, gửi WebSocket, nhận phản hồi, phát âm thanh, vẽ trạng thái lên OLED). Việc của bạn ở bước này **chỉ là cấu hình**, không phải viết code nhúng từ đầu.

```bash
git clone https://github.com/78/xiaozhi-esp32.git
cd xiaozhi-esp32
. ./export.sh
idf.py set-target esp32s3        # hoặc "esp32" nếu dùng board thường (xem Phần 2b)
idf.py menuconfig
```

Trong menuconfig, vào **Xiaozhi Assistant**:
- **Board Type** → `Bread Compact WiFi` (chỉ hiện khi target là esp32s3)
- **OLED Type** → SSD1306 128x64
- **Server address** → sửa thành `ws://<IP-laptop-cua-ban>:8000/xiaozhi/v1/` (IP laptop chạy xiaozhi-esp32-server ở Phần 3, cùng mạng WiFi với kiosk)

Sau đó:
```bash
idf.py build
idf.py -p <COM_PORT> flash monitor
```

### Xác nhận đúng chân đang dùng
Sau khi flash, theo dõi log khởi động (`idf.py monitor`) — dòng log sẽ hiện các thông tin GPIO thực tế board đang cấu hình, ví dụ:
```
I (267) Board: UUID=... SKU=bread-compact-wifi
I (277) gpio: GPIO[0]| InputEn: 1| ...   <- đây thường là nút boot/nút giữ-để-nói
I (287) gpio: GPIO[47]| ...
```
Nếu muốn biết chính xác số GPIO của mic/DAC/OLED mà board "Bread Compact WiFi" đang dùng để đối chiếu với dây bạn đã đấu, mở trực tiếp file (sau khi `git clone` ở trên):
```
xiaozhi-esp32/main/boards/bread-compact-wifi/config.h
```
File này có các dòng dạng `#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_xx` — copy đúng các số đó để đấu dây, **hoặc** sửa các số này lại cho khớp với dây bạn đã đấu sẵn trên breadboard (cách nào cũng được, miễn phần cứng và file này khớp nhau).

### Phần 2b — Nếu dùng ESP32 thường (không phải S3)
```bash
idf.py set-target esp32
```
Board `Bread Compact WiFi` sẽ **không xuất hiện** trong menuconfig (do bị khoá `depends on IDF_TARGET_ESP32S3`). Tải bản **"ESP32 Budget Version"** riêng từ trang Release/Firmware của repo thay vì tự build board này. Sau khi flash, **bắt buộc** chạy thử liên tục 30-60 phút với đủ mic + OLED + DAC + nút nhấn cắm cùng lúc (đúng checklist Giai đoạn 7 của tài liệu trước) trước khi lắp cố định vào vỏ kiosk.

---

## PHẦN 3 — Backend xiaozhi-esp32-server

```bash
git clone https://github.com/xinnan-tech/xiaozhi-esp32-server.git
cd xiaozhi-esp32-server
docker compose up -d
```

Mở file cấu hình (thường `data/.config.yaml` hoặc `config.yaml` tuỳ phiên bản — xem README của repo để biết đường dẫn chính xác trong bản bạn tải). Cấu trúc mẫu cần điền (⚠️ **tên khoá có thể khác đôi chút tuỳ phiên bản — đối chiếu file mẫu `config.yaml` gốc trong repo bạn vừa tải, đừng chỉ copy nguyên khối dưới đây mà không kiểm tra**):

```yaml
selected_module:
  ASR: WhisperASR        # hoặc provider tiếng Việt khác server hỗ trợ
  LLM: OpenAIBridge       # trỏ về dify_bridge.py, không gọi thẳng Gemini
  TTS: EdgeTTS            # hoặc FPT.AI / Vbee — chọn giọng tiếng Việt

ASR:
  WhisperASR:
    api_key: "sk-..."          # OpenAI API key cho Whisper
    model: "whisper-1"

LLM:
  OpenAIBridge:
    base_url: "http://<IP-laptop>:8080/v1"   # trỏ vào dify_bridge.py đang chạy local
    api_key: "khong-can-dung-thuc"           # bridge không kiểm tra key, điền gì cũng được
    model: "gemini-2.5-flash-lite"

TTS:
  EdgeTTS:
    voice: "vi-VN-HoaiMyNeural"   # giọng nữ tiếng Việt của Edge TTS (miễn phí)
    # Nếu muốn giọng tự nhiên hơn, thay bằng provider FPT.AI/Vbee ở đây
```

### Chạy `dify_bridge.py` (script trung gian)
```bash
pip install flask requests python-dotenv
```
Tạo file `.env` cùng thư mục với `dify_bridge.py`:
```
DIFY_BASE_URL=https://your-dify-host/v1
DIFY_API_KEY=app-xxxxxxxxxxxxxxxx
GEMINI_API_KEY=AIzaSy...
GEMINI_MODEL=gemini-2.5-flash-lite
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_SERVICE_ROLE_KEY=eyJ...
DEVICE_ID=kiosk-01
```
Chạy:
```bash
python dify_bridge.py
```
Sẽ thấy log `Dify bridge đang chạy tại http://0.0.0.0:8080`. Script này phải **chạy song song, cùng lúc** với `docker compose up -d` của xiaozhi-esp32-server (2 tiến trình riêng biệt trên cùng laptop).

### Test bridge độc lập (không cần đụng tới ESP32/Xiaozhi)
```bash
curl http://localhost:8080/health

curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"messages":[{"role":"user","content":"Học phí ngành CNTT bao nhiêu?"}]}'
```
Nếu nhận được JSON có `choices[0].message.content` là một câu trả lời hợp lý → bridge hoạt động đúng, có thể yên tâm nó sẽ chạy tốt khi ESP32 gọi vào. Đồng thời kiểm tra Supabase Table Editor xem dòng câu hỏi test này có xuất hiện không.

---

## PHẦN 4 — Thiết lập Dify (thao tác trên giao diện web, không cần code)

1. Vào Dify (tự host hoặc Cloud) → **Create App** → chọn loại **Chat App** (không cần Chatflow phức tạp vì phần phân loại đã do `dify_bridge.py` đảm nhiệm).
2. Vào tab **Context** → **Add Knowledge Base** → upload tài liệu trường (PDF/Word/txt: đề án tuyển sinh, học phí, ngành đào tạo, FAQ). Dify tự động chia nhỏ (chunk) và tạo embedding.
3. Vào tab **Orchestrate** (hoặc **Prompt Eng.** tuỳ phiên bản) → dán System Prompt "đanh đá vừa phải" đã có ở tài liệu trước vào ô **Instructions**.
4. Đảm bảo Knowledge Base bạn vừa thêm ở bước 2 được **bật (enabled)** trong phần Context của app này.
5. Vào **Publish** → **Access API** → copy **API Key** (dạng `app-xxxxxxxxxxxx`) và **API Base URL** — đây chính là `DIFY_API_KEY` và `DIFY_BASE_URL` trong file `.env` ở Phần 3.
6. Test nhanh ngay trong giao diện Dify (nút "Debug and Preview" hoặc "Chat" ở góc phải) bằng vài câu hỏi mẫu trước khi nối qua bridge — dễ debug hơn nhiều so với test qua toàn bộ chuỗi ESP32.

---

## PHẦN 5 — Supabase

Đã có `supabase_schema.sql` — chạy 1 lần trong SQL Editor. Test nhanh bằng curl (thay `SERVICE_ROLE_KEY` và `PROJECT_URL` thật):

```bash
# Test ghi (phải dùng service_role key)
curl -X POST "https://PROJECT_URL.supabase.co/rest/v1/questions" \
  -H "apikey: SERVICE_ROLE_KEY" \
  -H "Authorization: Bearer SERVICE_ROLE_KEY" \
  -H "Content-Type: application/json" \
  -d '{"question":"test","answer":"test trả lời","category":"Khác"}'

# Test đọc (dùng anon key - đúng như dashboard.html sẽ dùng)
curl "https://PROJECT_URL.supabase.co/rest/v1/questions?select=*" \
  -H "apikey: ANON_KEY"
```
Nếu lệnh ghi trả về lỗi 401/403 khi cố tình dùng **anon key** thay vì service_role key — đó là **đúng như thiết kế** (RLS đang chặn ghi công khai, xác nhận bảo mật hoạt động).

---

## PHẦN 6 — Chạy thử toàn chuỗi (end-to-end)

Thứ tự bật:
1. `docker compose up -d` (xiaozhi-esp32-server) trên laptop
2. `python dify_bridge.py` trên laptop (cổng 8080)
3. Cấp điện cho ESP32 kiosk, đợi kết nối WiFi + WebSocket (xem log OLED hoặc `idf.py monitor`)
4. Bấm giữ nút, hỏi thử: "Học phí ngành Công nghệ thông tin bao nhiêu?"
5. Theo dõi luồng: OLED hiện "Đang nghe..." → "Đang xử lý..." → loa Bluetooth phát câu trả lời → mở `dashboard.html` xem có xuất hiện dòng mới trong vài giây không.

Nếu một khâu nào đó không chạy, cô lập lỗi theo đúng thứ tự Phần 1 → 5 ở trên (mỗi phần đã có cách test độc lập riêng) thay vì đoán mò trên toàn hệ thống.

---

## PHẦN 7 — Biểu cảm mắt trên OLED (tạo sự thích thú cho học sinh)

### Tin đáng mừng trước tiên: thử bản mặc định của Xiaozhi trước
Firmware Xiaozhi đã có sẵn một hệ thống hiển thị biểu cảm ("emotion"/"theme") gắn liền với trạng thái thiết bị (đang chờ/đang nghe/đang nói) — mã nguồn có hẳn 1 thư mục riêng cho việc này (`main/display/`). Trên các board dùng màn LCD màu lớn, hệ này hiện icon mặt cười/mặt suy nghĩ dạng ảnh màu; trên OLED đơn sắc nhỏ như của bạn, khả năng cao nó vẫn hiện icon/trạng thái đơn giản hơn. **Hãy chạy thử bản build mặc định trước (Phần 2) và quan sát màn OLED khi bấm nút hỏi** — có thể đã đủ sinh động mà bạn không cần viết thêm gì.

### Nếu muốn "mắt" biểu cảm rõ hơn (như demo)
Dùng `oled_eyes_demo.ino` để chốt hình vẽ ưng ý trước khi đụng vào firmware Xiaozhi:
1. Mở file, sửa `OLED_SDA_PIN`/`OLED_SCL_PIN` cho khớp dây đã đấu.
2. Cài thư viện **Adafruit SSD1306** và **Adafruit GFX Library** qua Library Manager của Arduino IDE.
3. Upload, mở Serial Monitor (115200 baud), gõ `1`/`2`/`3`/`4` để xem thử từng biểu cảm: chờ (chớp mắt, đảo mắt nhẹ) → đang nghe (mắt to hơn + vòng sóng lan toả) → đang xử lý (mắt híp + 3 chấm loading) → đang nói (mắt thường + cột sóng âm nhảy như miệng đang nói).
4. Chỉnh sửa số đo (`h`, bán kính vòng sóng, tốc độ chớp...) ngay trong code cho tới khi ưng ý — vì đây là sketch độc lập, sửa xong upload lại là thấy ngay, không phải build lại toàn bộ Xiaozhi mỗi lần thử.

### Ghép vào firmware Xiaozhi thật
Đây là bước cần bạn tự mở mã nguồn ra xem (mình không có quyền tải thẳng file cụ thể để đưa số dòng chính xác):
1. Sau khi `git clone` (Phần 2), tìm file định nghĩa trạng thái thiết bị (dạng liệt kê idle/listening/speaking...) và file `main/boards/bread-compact-wifi/compact_wifi_board.cc` — đây là nơi khởi tạo đối tượng điều khiển màn OLED.
2. Tìm hàm được gọi mỗi khi trạng thái đổi (thường tên có chữ như `SetDeviceState`, `OnStateChanged`, hoặc tương tự — tìm theo từ khoá "listening"/"speaking"/"idle" trong toàn bộ thư mục `main/` bằng chức năng Find in Files của IDE bạn dùng).
3. Tại đúng những chỗ đó, dịch lại 4 hàm `drawWaiting/drawListening/drawThinking/drawTalking` từ file demo sang đúng API màn hình mà class hiển thị của Xiaozhi đang cung cấp (nếu class đó cũng dựa trên Adafruit GFX hoặc u8g2, phần lớn logic vẽ hình có thể giữ nguyên gần như không đổi — chỉ đổi cách lấy con trỏ `display`).

Nếu bước 2-3 quá mất thời gian mò mẫm, phương án lùi an toàn: dùng nguyên bản biểu cảm mặc định của Xiaozhi (đã đủ tạo cảm giác "có sự sống" cho kiosk) và dành công sức cho các phần còn lại của dự án.
