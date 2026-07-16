# Kế hoạch triển khai: Kiosk Trợ lý AI Tư vấn Tuyển sinh (bản cập nhật)

Cập nhật theo danh sách linh kiện mới nhất: **bỏ MAX98357A + loa tích hợp**, dùng **PCM5102A (DAC line-out) + loa Bluetooth công suất lớn có sẵn** qua cáp AUX.

Kèm theo tài liệu này:
- `supabase_schema.sql` — không đổi, dùng như cũ
- `dashboard.html` — không đổi, dùng như cũ

---

## ✅ Đã chốt: ESP32-S3 N16R8

Dự án dùng **ESP32-S3-DevKitC-1 N16R8** (16MB Flash / 8MB PSRAM) — board này chọn được đúng cấu hình chính thức **"Bread Compact WiFi"** trong firmware Xiaozhi (thiết kế riêng cho tổ hợp ESP32-S3 + INMP441 + DAC I2S), có đủ PSRAM nên không còn rủi ro tràn bộ nhớ như bản ESP32 thường. Nhánh "Budget Version" và các lưu ý rủi ro ở bản trước **không còn áp dụng**.

**Lưu ý khi đấu dây:** board N16R8 dùng octal PSRAM nên dải **GPIO 26–37 bị chiếm dụng nội bộ**, không thể dùng cho mic/DAC/OLED như sơ đồ cũ vẽ cho ESP32 thường. Sơ đồ chân ở Giai đoạn 1 bên dưới đã được chọn lại để né hoàn toàn vùng này.

---

## GIAI ĐOẠN 0 — Danh sách mua sắm (cập nhật)

### Linh kiện điện tử

| # | Linh kiện | Quy cách | SL | Ghi chú |
|---|---|---|---|---|
| 1 | Vi điều khiển | **ESP32-S3-DevKitC-1 N16R8** (16MB Flash / 8MB PSRAM) | 1 | Đã chốt — dùng được board chính thức "Bread Compact WiFi" |
| 2 | Microphone | INMP441, chip đã hàn, có chân kim loại | 1 | Mua từ shop uy tín, đánh giá cao — test ngay khi nhận hàng (xem Giai đoạn 7) |
| 3 | DAC âm thanh | **PCM5102 hoặc PCM5102A**, đã hàn sẵn chân | 1 | 2 chip pin-to-pin giống nhau, dùng loại nào cũng được (PCM5102A hỗ trợ thêm logic 1.8V, không ảnh hưởng khi dùng với ESP32 3.3V). Chỉ xuất line-level, không tự khuếch đại được loa |
| 4 | Màn hình | OLED 0.96" I2C (SSD1306), 4 chân | 1 | |
| 5 | Nút nhấn | DS-316 (10mm) nhấn nhả, hoặc Arcade/LA38 | 1 | |
| 6 | Breadboard | MB-102, 830 lỗ | 1 | |
| 7 | Dây Dupont | 1 bó Đực-Đực, 1 bó Đực-Cái | | |
| 8 | Tụ điện / điện trở | Tụ 1000µF (lọc nguồn cho DAC), trở 220 Ohm | 1 bộ | Vẫn nên giữ — tụ lọc nguồn giúp giảm nhiễu tín hiệu DAC |

### Thiết bị hỗ trợ (không cần mua chung)
- **Cáp USB Type-C loại truyền dữ liệu thật** (không dùng loại chỉ sạc) — để nạp firmware.
- **Loa Bluetooth/loa active công suất lớn** sẵn có — thay thế hoàn toàn cho loa 3-5W + MAX98357A cũ. Đây là nâng cấp thực sự về độ lớn tiếng so với bản đầu.
- **Cáp AUX 3.5mm 2 đầu đực** — nối từ jack tai nghe trên PCM5102A sang cổng AUX IN của loa Bluetooth.
- Laptop/PC có trình duyệt — thiết lập Dify + công cụ nạp firmware.
- (Tuỳ chọn) Vỏ hộp mica/in 3D — giờ chỉ cần đục lỗ cho OLED, mic và nút bấm (**không cần lỗ cho loa** vì loa đặt ngoài vỏ).

> **Lợi ích phụ đáng chú ý:** vì loa giờ nằm ngoài kiosk (loa Bluetooth đặt riêng), rủi ro dội âm/nhiễu giữa loa và mic mà mình từng lưu ý ở bản thiết kế cũ giảm đi đáng kể — không cần vách ngăn cách ly loa-mic trong vỏ hộp nữa. Chỉ cần lưu ý không đặt loa Bluetooth chĩa thẳng và quá gần mic.

---

## GIAI ĐOẠN 1 — Lắp ráp phần cứng

**Bộ chân đã chọn cho ESP32-S3 N16R8** — né hoàn toàn GPIO 26-37 (PSRAM/Flash nội bộ), GPIO 19-20 (USB), GPIO 0/3/45/46 (strapping), GPIO 43-44 (UART console):

### Mic INMP441
| Chân INMP441 | Nối tới |
|---|---|
| SCK | GPIO 4 |
| WS | GPIO 5 |
| SD | GPIO 6 |
| VDD | 3.3V |
| GND, L/R | GND |

### DAC PCM5102 / PCM5102A

| Chân DAC | Nối tới | Ghi chú |
|---|---|---|
| VIN | 3.3V hoặc 5V | Xem ghi trên module cụ thể bạn mua (đa số hỗ trợ cả hai nhờ LDO tích hợp) |
| GND | GND | |
| BCK | GPIO 7 | |
| LCK | GPIO 15 | |
| DIN | GPIO 16 | |
| SCK | **GND** | Không dùng master clock ngoài — bắt buộc nối GND |
| FLT | GND | Chọn bộ lọc chuẩn (normal latency) |
| DEMP | GND | Tắt de-emphasis |
| FMT | GND | Định dạng I2S chuẩn |
| **XSMT** | **3.3V** | ⚠️ **Quan trọng nhất — lỗi phổ biến số 1 gây "không có tiếng"!** Chân này điều khiển mute. Nhiều module để mặc định nối GND (im tiếng). Bắt buộc kéo lên 3.3V mới nghe được. |
| OUT (jack 3.5mm) | Cáp AUX → loa Bluetooth | |

> Nếu module bạn mua không đưa 4 chân FLT/DEMP/XSMT/FMT ra ngoài header mà chỉ có các miếng hàn jumper ở mặt sau (ký hiệu H1L–H4L), thì 3 jumper đầu để mặc định là được, riêng jumper XSMT (thường ký hiệu H3L) cần hàn lại sang phía "H" để không bị mute. Nếu ngại đụng mỏ hàn, ưu tiên chọn loại module có đưa chân XSMT ra ngoài để đấu dây trực tiếp — đa số module "đã hàn sẵn chân" trên Shopee đều thuộc loại này.

### OLED & nút nhấn
| Module | Chân | Nối tới |
|---|---|---|
| OLED SSD1306 | SCL | GPIO 17 |
| OLED SSD1306 | SDA | GPIO 18 |
| OLED SSD1306 | VCC / GND | 3.3V / GND |
| Nút nhấn (bàn phím lớn DS-316/Arcade) | 1 chân | GPIO 8 |
| Nút nhấn | chân còn lại | GND |

> **Mẹo:** board ESP32-S3 DevKitC-1 đã có sẵn 1 nút BOOT vật lý nối cứng vào GPIO 0 — đây chính là chân mặc định firmware Xiaozhi dùng làm nút giữ-để-nói. Bạn có thể dùng tạm nút BOOT có sẵn để test nhanh trước, sau đó đấu nút DS-316 rời vào GPIO 8 và sửa `BOOT_BUTTON_GPIO` trong `config.h` (Giai đoạn 2) sang GPIO 8 cho bản lắp ráp cuối — tránh dùng chung GPIO 0 với nút rời vì đây là chân strapping, giữ nó ở mức thấp lúc cắm điện có thể vô tình đưa board vào chế độ nạp firmware.

*(Nếu muốn đối chiếu với số chân mặc định thật của board "Bread Compact WiFi", mở file `main/boards/bread-compact-wifi/config.h` sau khi tải mã nguồn ở Giai đoạn 2 và sửa lại các macro `AUDIO_I2S_MIC_GPIO_*`, `AUDIO_I2S_SPK_GPIO_*`, `DISPLAY_SCL_PIN`, `DISPLAY_SDA_PIN`, `BOOT_BUTTON_GPIO` cho khớp với bảng trên.)*

### Kiểm tra trước khi cấp điện
Dùng đồng hồ vạn năng đo thông mạch GND–GND, đảm bảo 3.3V không chạm GND, trước khi cắm USB lần đầu.

---

## GIAI ĐOẠN 2 — Firmware Xiaozhi trên ESP32-S3

```bash
git clone https://github.com/78/xiaozhi-esp32.git
cd xiaozhi-esp32
. ./export.sh
idf.py set-target esp32s3
idf.py menuconfig   # Xiaozhi Assistant → Board Type → Bread Compact WiFi
```

Trong menuconfig, ngoài Board Type, sửa thêm:
- **OLED Type** → SSD1306 128x64
- **Server address** → sửa thành WebSocket server bạn tự host ở Giai đoạn 3 (ví dụ `ws://<IP-laptop>:8000/xiaozhi/v1/`) thay vì server mặc định của hãng — bắt buộc để dùng được tiếng Việt và dữ liệu riêng của trường.

Nếu muốn wiring khớp đúng bảng chân ở Giai đoạn 1, mở `main/boards/bread-compact-wifi/config.h` và sửa các macro `AUDIO_I2S_MIC_GPIO_*`, `AUDIO_I2S_SPK_GPIO_*`, `DISPLAY_SCL_PIN`, `DISPLAY_SDA_PIN`, `BOOT_BUTTON_GPIO` cho khớp GPIO 4/5/6 (mic), 7/15/16 (DAC), 17/18 (OLED), 8 (nút nhấn).

```bash
idf.py build
idf.py -p <COM_PORT> flash monitor
```

---

## GIAI ĐOẠN 3 — Backend tự host (xiaozhi-esp32-server)

```bash
git clone https://github.com/xinnan-tech/xiaozhi-esp32-server.git
cd xiaozhi-esp32-server
docker compose up -d
```

Trong `config.yaml`, khai báo 3 nhóm provider:
- **ASR:** provider hỗ trợ tiếng Việt tốt (OpenAI Whisper API, hoặc Google Cloud Speech-to-Text nếu hỗ trợ endpoint tuỳ chỉnh).
- **LLM:** Gemini 2.5 Flash/Flash-Lite trở lên qua endpoint tương thích OpenAI của Google, hoặc trỏ về Dify (xem Giai đoạn 4).
- **TTS:** giọng tiếng Việt (FPT.AI, Vbee, Google Cloud TTS…).

> ⚠️ Tên khoá cấu hình có thể khác nhau tuỳ phiên bản — luôn đối chiếu `config.yaml` mẫu mới nhất trong repo.

---

## GIAI ĐOẠN 4 — Dify: kiến thức trường + tính cách AI

1. Tạo Dify app kiểu Chatflow (tự host hoặc Dify Cloud free tier).
2. **Knowledge Base:** tải tài liệu chính thức của trường (đề án tuyển sinh, học phí, ngành đào tạo, FAQ).
3. **System Prompt** (giữ nguyên bản mẫu "đanh đá vừa phải"):
   ```
   Bạn là [Tên trợ lý], trợ lý tuyển sinh của Trường [Tên trường], đang
   đứng tại một kiosk tương tác trong ngày hội tuyển sinh. Người nói
   chuyện với bạn là học sinh THPT và phụ huynh.

   Phong cách: nhanh nhảu, dí dỏm, hơi "đanh đá" một cách duyên dáng —
   thẳng thắn, thỉnh thoảng chọc nhẹ — nhưng luôn tôn trọng người hỏi,
   không mỉa mai quá đà, không xúc phạm, không nói tục.

   Nguyên tắc bắt buộc:
   1. CHỈ trả lời dựa trên tài liệu trong Knowledge Base được cung cấp.
      Nếu không có thông tin, nói thẳng: "Cái này mình chưa có thông tin
      chính xác, bạn ghé quầy tư vấn hỏi thầy cô trực tiếp nhé" — TUYỆT
      ĐỐI không tự bịa số liệu (điểm chuẩn, học phí, chỉ tiêu...).
   2. Trả lời ngắn gọn, tối đa 3-4 câu mỗi lượt vì đây là hội thoại
      bằng giọng nói.
   3. Nếu câu hỏi không liên quan tuyển sinh/nhà trường, trả lời dí
      dỏm một câu rồi hướng học sinh quay lại chủ đề tuyển sinh.
   4. Không bình luận tiêu cực về trường khác, không tiết lộ thông
      tin cá nhân của giáo viên/nhân viên.
   ```
4. **Node phân loại:** xuất thêm trường `category` theo danh sách cố định (Công nghệ thông tin / Kinh tế - Quản trị / Ngôn ngữ - Xã hội / Học phí - Học bổng / Khác).
5. **Node ghi log:** HTTP Request cuối workflow, POST vào Supabase:
   ```
   POST https://YOUR-PROJECT.supabase.co/rest/v1/questions
   Headers:
     apikey: <SERVICE_ROLE_KEY>
     Authorization: Bearer <SERVICE_ROLE_KEY>
     Content-Type: application/json
   Body: { "question": "...", "answer": "...", "category": "..." }
   ```
6. Trỏ LLM provider trong `xiaozhi-esp32-server` về API endpoint của app Dify này; nếu chưa có plugin trực tiếp, dùng một script trung gian nhỏ (Flask/FastAPI) forward request.

---

## GIAI ĐOẠN 5 — Supabase (không đổi)

1. Tạo project free tier tại supabase.com, chạy `supabase_schema.sql` trong SQL Editor.
2. Lấy **Project URL**, **anon public key** (cho dashboard), **service_role key** (giữ bí mật, dùng trong node HTTP Request của Dify).
3. Nhớ "đánh thức" project trước sự kiện 1-2 ngày nếu để im hơn 7 ngày.

---

## GIAI ĐOẠN 6 — Dashboard thời gian thực (không đổi)

Sửa 2 dòng cấu hình đầu file `dashboard.html`, đẩy lên GitHub repo, bật GitHub Pages, mở trên TV cạnh kiosk.

---

## GIAI ĐOẠN 7 — Kiểm thử

- [ ] **Test linh kiện ngay khi nhận hàng:** cắm thử riêng lẻ mic, DAC, OLED, nút nhấn trước khi lắp chung — phát hiện sớm hàng lỗi/hàng kém chất lượng từ Shopee để còn kịp đổi trả.
- [ ] **Âm thanh:** nếu không nghe được gì đầu tiên, kiểm tra ngay chân **XSMT của PCM5102A** trước (lỗi phổ biến nhất).
- [ ] **Tải hệ thống (đặc biệt nếu chọn Nhánh B — ESP32 thường):** chạy liên tục 30-60 phút với đủ mic+OLED+DAC+nút nhấn, theo dõi log xem có tự reboot không.
- [ ] Pipeline AI: hỏi thử bằng giọng nói → transcript đúng tiếng Việt → câu trả lời đúng theo Knowledge Base → giọng TTS phát rõ qua loa Bluetooth.
- [ ] Supabase + Dashboard cập nhật realtime.
- [ ] Thử trong môi trường ồn giả lập.
- [ ] Hỏi liên tục nhiều lượt để kiểm tra hạn mức free tier Gemini.
- [ ] Kiểm tra khoảng cách/hướng đặt loa Bluetooth so với mic để tránh vọng âm nếu để gần nhau.

---

## GIAI ĐOẠN 8 — Checklist ngày sự kiện (không đổi)

- [ ] Đánh thức Supabase project.
- [ ] Bật billing dự phòng cho Gemini API.
- [ ] Mang bộ phát WiFi/4G di động riêng.
- [ ] Sạc đầy pin dự phòng / adapter dự phòng cho ESP32 lẫn laptop backend, **và cả loa Bluetooth**.
- [ ] Chuẩn bị bản demo quay sẵn phòng sự cố.
- [ ] Nút Reset của board dễ tiếp cận.

---

## Phụ lục — Tổng hợp các lần điều chỉnh

| Vấn đề | Đã sửa thành |
|---|---|
| Gemini 1.5 Flash (ngừng hoạt động) | Gemini 2.5 Flash/Flash-Lite hoặc mới hơn |
| Server mặc định xiaozhi.me (tối ưu tiếng Trung) | Tự host xiaozhi-esp32-server |
| Mic đơn, dễ nhiễu ồn | Đặt gần miệng học sinh |
| Chưa tách quyền ghi/đọc dữ liệu | RLS: đọc công khai qua anon key, ghi riêng qua service_role key |
| **Loa 3-5W tích hợp qua MAX98357A** | **DAC PCM5102A xuất line-out qua AUX → loa Bluetooth ngoài công suất lớn** |
| ESP32 thường vs ESP32-S3 | **Chưa chốt — xem hộp quyết định đầu tài liệu; ảnh hưởng trực tiếp đến việc chọn được board "Bread Compact WiFi" hay phải dùng bản Budget Version rủi ro hơn** |
