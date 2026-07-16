# HƯỚNG DẪN ĐẤU NỐI TỪ ĐẦU — Freenove ESP32-S3 CAM

## Linh kiện cần có trước mặt

- [x] Freenove ESP32-S3 WROOM CAM (N16R8)
- [x] ILI9341 2.4" SPI module
- [x] INMP441 mic module
- [x] Nút nhấn (tạm dùng nút BOOT trên board, không cần nối thêm)
- [x] Breadboard 830 lỗ
- [x] Dây Dupont (đực-đực, đực-cái)
- [x] Cáp USB Type-C truyền dữ liệu
- [x] Loa Bluetooth (để bên cạnh, chưa dùng ở bước test)

---

## BƯỚC 1: Cắm board ESP32-S3 lên breadboard

```
  ┌─────────────────────────────────┐
  │  Cắm board ESP32-S3 CAM nằm    │
  │  giữa breadboard, USB-C hướng  │
  │  ra phía dưới (gần bạn)        │
  │                                 │
  │  Đảm bảo 2 hàng chân nằm ở    │
  │  2 bên rãnh giữa breadboard    │
  │  (không chập chân)             │
  └─────────────────────────────────┘
```

**CHƯA CẮM USB VÀO MÁY TÍNH — đấu dây xong hết mới cắm!**

---

## BƯỚC 2: Nối ILI9341 (màn hình) — 7 dây

Nhìn vào module ILI9341 2.4" SPI, thường có 9 chân theo thứ tự:
`VCC GND CS RST DC MOSI SCK LED MISO`
(có thể tên hơi khác tùy module, đọc trên mặt board)

### Bảng nối dây:

| Chân ILI9341 | Nối tới trên ESP32-S3 | Dây màu gợi ý |
|---|---|---|
| **VCC** | **3.3V** (chân 3V3 trên board) | Đỏ |
| **GND** | **GND** | Đen |
| **CS** | **GPIO 10** | Vàng |
| **RST** (hoặc RESET) | **GPIO 8** | Trắng |
| **DC** (hoặc RS) | **GPIO 9** | Xanh lá |
| **MOSI** (hoặc SDA/SDI) | **GPIO 11** | Xanh dương |
| **SCK** (hoặc CLK) | **GPIO 12** | Cam |
| **LED** (hoặc BL) | **3.3V** | Đỏ (chung với VCC được) |
| **MISO** | **KHÔNG NỐI** | — |

### Cách tìm đúng chân GPIO trên board Freenove:

Nhìn mặt trước board (có in tên chân), tìm theo số GPIO in trên silkscreen:

```
Board Freenove ESP32-S3 CAM — Nhìn từ trên xuống, USB ở dưới:

        MẶT TRÁI                    MẶT PHẢI
        --------                    ---------
    3V3 ●  1                   40 ● 5V
    3V3 ●  2                   39 ● GND
    RST ●  3                   38 ● GPIO 43 (TX)
 GPIO 4 ●  4                   37 ● GPIO 44 (RX)
 GPIO 5 ●  5                   36 ● GPIO 1
 GPIO 6 ●  6                   35 ● GPIO 2
 GPIO 7 ●  7                   34 ● GPIO 42
GPIO 15 ●  8                   33 ● GPIO 41
GPIO 16 ●  9  ← MIC_SCK       32 ● GPIO 40
GPIO 17 ● 10  ← MIC_WS        31 ● GPIO 39
GPIO 18 ● 11  ← MIC_SD        30 ● GPIO 38
 GPIO 8 ● 12  ← TFT_RST       29 ● GPIO 37 (cấm)
 GPIO 9 ● 13  ← TFT_DC        28 ● GPIO 36 (cấm)
GPIO 10 ● 14  ← TFT_CS        27 ● GPIO 35 (cấm)
GPIO 11 ● 15  ← TFT_MOSI      26 ● GPIO 0 (nút BOOT)
GPIO 12 ● 16  ← TFT_SCK       25 ● GPIO 45
GPIO 13 ● 17                   24 ● GPIO 46
GPIO 14 ● 18                   23 ● GPIO 47  ← BTN_PIN
     5V ● 19                   22 ● GPIO 21
    GND ● 20                   21 ● GPIO 48
        └────── micro-SD ──────┘
              USB-C ở đây
```

> **Mẹo:** Nếu board bạn in số chân khác, hãy nhìn kỹ trên silkscreen
> (chữ in trắng trên board). Quan trọng là đúng SỐ GPIO, không phải
> số thứ tự vật lý.

### Đấu dây ILI9341 — từng sợi một:

1. **Dây đỏ #1:** ILI9341 `VCC` → breadboard hàng `3V3` (chân 1 hoặc 2 bên trái board)
2. **Dây đen #1:** ILI9341 `GND` → breadboard hàng `GND` (chân 20 bên trái hoặc chân 39 bên phải)
3. **Dây vàng:** ILI9341 `CS` → `GPIO 10` (chân vật lý 14 bên trái)
4. **Dây trắng:** ILI9341 `RST` → `GPIO 8` (chân vật lý 12 bên trái)
5. **Dây xanh lá:** ILI9341 `DC` → `GPIO 9` (chân vật lý 13 bên trái)
6. **Dây xanh dương:** ILI9341 `MOSI` → `GPIO 11` (chân vật lý 15 bên trái)
7. **Dây cam:** ILI9341 `SCK` → `GPIO 12` (chân vật lý 16 bên trái)
8. **Dây đỏ #2:** ILI9341 `LED` → breadboard hàng `3V3` (cùng hàng với VCC)

**Kiểm tra:** Đếm lại — bạn phải có **8 dây** nối từ ILI9341 (MISO để trống).

---

## BƯỚC 3: Nối INMP441 (mic) — 5 dây

Module INMP441 thường có 6 chân: `VDD GND SD SCK WS L/R`

| Chân INMP441 | Nối tới | Ghi chú |
|---|---|---|
| **VDD** | **3.3V** | Dùng chung hàng 3.3V với ILI9341 |
| **GND** | **GND** | Dùng chung hàng GND |
| **SD** | **GPIO 18** (chân vật lý 11 bên trái) | Dữ liệu âm thanh |
| **SCK** | **GPIO 16** (chân vật lý 9 bên trái) | Clock |
| **WS** | **GPIO 17** (chân vật lý 10 bên trái) | Word Select |
| **L/R** | **GND** | Chọn kênh trái — nối chung GND |

### Đấu dây INMP441 — từng sợi:

1. **Dây đỏ:** INMP441 `VDD` → hàng `3V3` trên breadboard
2. **Dây đen #1:** INMP441 `GND` → hàng `GND` trên breadboard
3. **Dây tím:** INMP441 `SD` → `GPIO 18`
4. **Dây xám:** INMP441 `SCK` → `GPIO 16`
5. **Dây nâu:** INMP441 `WS` → `GPIO 17`
6. **Dây đen #2:** INMP441 `L/R` → hàng `GND` (cùng GND)

**Kiểm tra:** 6 dây, tất cả chân INMP441 đều được nối.

---

## BƯỚC 4: Nối nút nhấn 4 chân (tactile switch) — 2 dây

Nút 4 chân có cấu tạo như sau (nhìn từ trên xuống):

```
     ┌─────┐
  A ─┤     ├─ B
     │     │
  C ─┤     ├─ D
     └─────┘
```

Chân A-C thông nhau, chân B-D thông nhau. Khi bấm: A-B thông (C-D thông).
→ Bạn chỉ cần nối **2 chân chéo nhau** (A và D, hoặc B và C).

### Cắm nút lên breadboard:

Cắm nút nhấn **ngang qua rãnh giữa** breadboard (2 chân mỗi bên rãnh).

### Nối dây:

| Chân nút | Nối tới |
|---|---|
| **1 chân bất kỳ** | **GPIO 47** (chân vật lý 23 bên phải board) |
| **Chân chéo đối diện** | **GND** |

1. **Dây xanh:** Từ 1 chân nút → `GPIO 47`
2. **Dây đen:** Từ chân chéo đối diện → hàng `GND` trên breadboard

> **Cách dùng:** Đè giữ nút → nói vào mic → thả nút = gửi câu hỏi.
> Code đã bật INPUT_PULLUP nên không cần thêm điện trở.

---

## BƯỚC 5: Kiểm tra lại TRƯỚC KHI cắm điện

### Checklist bắt buộc:

- [ ] **3.3V không chạm GND** — nhìn kỹ dây đỏ và dây đen, đảm bảo không chập
- [ ] **ILI9341:** 8 dây (VCC, GND, CS, RST, DC, MOSI, SCK, LED)
- [ ] **INMP441:** 6 dây (VDD, GND, SD, SCK, WS, L/R)
- [ ] **Nút nhấn:** 2 dây (GPIO 47 + GND)
- [ ] **Không có dây nào nối vào GPIO 26-37** (cấm tuyệt đối, PSRAM)
- [ ] **MISO của ILI9341 để trống** (không nối gì)
- [ ] Tất cả dây cắm chặt, không lỏng

### Nếu có đồng hồ vạn năng:
Chuyển sang chế độ đo thông mạch (beep), đo giữa 3.3V và GND → **KHÔNG được kêu beep**.
Nếu kêu beep = chập mạch → tìm và sửa dây trước khi cắm USB.

---

## BƯỚC 6: Cài đặt Arduino IDE

### 6a. Cài board ESP32-S3:
1. Mở Arduino IDE
2. **File → Preferences → Additional Board Manager URLs** → thêm:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. **Tools → Board → Boards Manager** → tìm `esp32` → cài **"esp32 by Espressif Systems"** (phiên bản ≥ 3.0)

### 6b. Cài thư viện LovyanGFX:
1. **Tools → Manage Libraries** (hoặc Sketch → Include Library → Manage Libraries)
2. Tìm **"LovyanGFX"** → Install

### 6c. Cấu hình board (QUAN TRỌNG):
Vào **Tools** và chỉnh từng mục:

| Mục | Chọn |
|---|---|
| Board | **ESP32S3 Dev Module** |
| USB CDC On Boot | **Enabled** |
| PSRAM | **OPI PSRAM** |
| Flash Size | **16MB (128Mb)** |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** hoặc mặc định |
| Upload Speed | **921600** |

---

## BƯỚC 7: Upload code test

1. Cắm cáp USB Type-C từ board vào máy tính
2. **Tools → Port** → chọn cổng COM mới xuất hiện (ví dụ COM3, COM5...)
   - Nếu KHÔNG thấy cổng nào → **99% do cáp USB chỉ sạc, không truyền dữ liệu** → đổi cáp khác
3. Mở file **`test_hardware_v2.ino`**
4. Bấm nút **Upload** (mũi tên →) ở góc trên trái
5. Đợi compile + upload xong (lần đầu hơi lâu ~2-3 phút)
6. Sau khi upload xong, **Tools → Serial Monitor** → chọn baud **115200**

### Kết quả mong đợi:

**Trên màn hình ILI9341:**
1. Hiện chữ "KIOSK AI TUYEN SINH" → đợi 2 giây
2. Hiện 3 ô màu ĐỎ - XANH LÁ - XANH DƯƠNG → nếu thấy = SPI OK
3. Hiện thanh xanh lá nhảy → **nói to vào mic** trong 8 giây
4. Hiện test nút → **bấm nút BOOT** trên board trong 10 giây
5. Hiện tổng kết

**Trên Serial Monitor (máy tính):**
- Hiện log `Mic level: xxxx` khi nói
- Hiện `Button pressed!` khi bấm nút

---

## XỬ LÝ SỰ CỐ THƯỜNG GẶP

| Hiện tượng | Nguyên nhân | Cách sửa |
|---|---|---|
| Màn hình trắng / không hiện gì | Sai dây MOSI/SCK/DC/CS | Kiểm tra lại 4 dây này |
| Màn hình có sọc loạn | Sai dây RST hoặc DC | Đổi thử DC ↔ RST |
| Màn hình đen, không sáng | Chưa nối LED/BL vào 3.3V | Nối thêm dây |
| Mic thanh không nhảy | Sai dây SCK/WS/SD | Kiểm tra 3 dây + VDD + L/R→GND |
| Mic thanh không nhảy | Module cắm ngược | INMP441 có 1 lỗ nhỏ ở mặt trước — mặt đó phải hướng ra ngoài (gần miệng nói), KHÔNG úp xuống |
| Nút bấm không phản hồi | Bấm nhầm nút RST | Nút BOOT thường nhỏ hơn, ở vị trí khác với nút RST |
| Upload lỗi | Cáp USB chỉ sạc | Đổi cáp USB Type-C khác |
| Port không hiện | Chưa cài driver | Cài driver CH343 cho board Freenove |

---

## BẢNG TÓM TẮT SƠ ĐỒ CHÂN

```
┌─────────────────────────────────────────────────┐
│           FREENOVE ESP32-S3 CAM                 │
│                                                 │
│  ILI9341 Display (SPI)     INMP441 Mic (I2S)   │
│  ┌─────────────────┐       ┌──────────────┐    │
│  │ MOSI → GPIO 11  │       │ SCK → GPIO 16│    │
│  │ SCK  → GPIO 12  │       │ WS  → GPIO 17│    │
│  │ CS   → GPIO 10  │       │ SD  → GPIO 18│    │
│  │ DC   → GPIO  9  │       │ VDD → 3.3V   │    │
│  │ RST  → GPIO  8  │       │ GND → GND    │    │
│  │ VCC  → 3.3V     │       │ L/R → GND    │    │
│  │ GND  → GND      │       └──────────────┘    │
│  │ LED  → 3.3V     │       Nút nhấn 4 chân:      │
│  │ MISO → (trống)  │       1 chân  → GPIO 47      │
│  └─────────────────┘       1 chân  → GND           │
│                                                 │
│  ⛔ GPIO 26-37: CẤM DÙNG (PSRAM)               │
│  ⛔ GPIO 19-20: CẤM DÙNG (USB)                 │
└─────────────────────────────────────────────────┘
```
