# TIẾN ĐỘ DỰ ÁN: Kiosk AI Tư vấn Tuyển sinh
> File này dùng để AI phiên sau đọc và tiếp tục công việc.
> Cập nhật lần cuối: 17/07/2026 (đã flash Xiaozhi, sửa board LCD/nút, ghi log Supabase)

---

## TỔNG QUAN DỰ ÁN

Kiosk AI tư vấn tuyển sinh cho Trường Trung cấp Kinh tế - Kỹ thuật TP. Cần Thơ.
Học sinh nhấn giữ nút, hỏi bằng giọng nói, AI trả lời qua loa, dữ liệu hiển thị realtime trên dashboard.

**Kiến trúc:** Mic INMP441 → ESP32-S3 → Xiaozhi (STT) → dify_bridge.py (Flask) → Dify (RAG) + Gemini (phân loại) → Supabase → Dashboard realtime

---

## PHẦN CỨNG HIỆN CÓ

| Thiết bị | Trạng thái |
|---|---|
| Freenove ESP32-S3 WROOM CAM (N16R8) | DA CO - DA TEST OK |
| ILI9341 2.4" SPI (man hinh) | DA CO - DA TEST OK |
| INMP441 (mic) | DA CO - DA TEST OK |
| Nut nhan (tactile switch) | DA CO - DA TEST OK |
| Breadboard 830 lo | DA CO |
| Day Dupont | DA CO |
| Cap USB Type-C | DA CO |
| Loa Bluetooth | DA CO (chua ket noi) |
| **PCM5102 / PCM5102A (DAC am thanh)** | **DA CO - SAN SANG LAP** |
| Cap AUX 3.5mm | **DA CO - SAN SANG LAP** |

## SO DO DAU NOI HIEN TAI (DA TEST OK)

```
ILI9341 (man hinh)     ESP32-S3
VCC                    3.3V
GND                    GND
CS                     GPIO 10
RST                    GPIO 8
DC                     GPIO 9
MOSI                   GPIO 11
SCK                    GPIO 12
LED                    3.3V

INMP441 (mic)          ESP32-S3
VDD                    3.3V
GND                    GND
SCK                    GPIO 16
WS                     GPIO 17
SD                     GPIO 18
L/R                    GND

Nut nhan               ESP32-S3
Dau 1                  GPIO 47
Dau 2                  GND
```

## SO DO DAU NOI PCM5102 (CHUA LAP - CHO MUA LINH KIEN)

```
PCM5102 / PCM5102A     ESP32-S3         Ghi chu
VIN                    3.3V             (hoac 5V tuy module)
GND                    GND
BCK                    GPIO 7           I2S bit clock
LCK                    GPIO 15          I2S word select (LRC)
DIN                    GPIO 14          I2S data out
SCK                    GND              Khong dung master clock → bat buoc noi GND
FLT                    GND              Bo loc chuan
DEMP                   GND              Tat de-emphasis
FMT                    GND              Dinh dang I2S chuan
XSMT                   3.3V             ⚠️ QUAN TRONG: phai keo len 3.3V, neu de GND se bi im tieng!
OUT (jack 3.5mm)       Cap AUX → loa    Noi vao cong AUX IN cua loa Bluetooth
```

> **Luu y:** GPIO 7, 15, 14 khong trung voi bat ky thiet bi nao da noi.
> GPIO 26-37: CAM DUNG (PSRAM). GPIO 19-20: CAM DUNG (USB).

---

## NHUNG GI DA THUC HIEN

### 1. Phan cung (DA XONG)
- [x] Dau noi ILI9341 (man hinh SPI) → ESP32-S3 — da test OK
- [x] Dau noi INMP441 (mic) → ESP32-S3 — da test OK
- [x] Dau noi nut nhan → GPIO 47 — da test OK
- [x] Dau noi PCM5102A (DAC) → ESP32-S3 — da test OK
- [x] Loa nhan duoc am thanh tu Xiaozhi; truoc do loi do day PCM5102A bi long
- [x] Chay file `test_hardware_v3/test_hardware_v3.ino` — man hinh, mic, nut, loa deu hoat dong

### 2. Phan mem - Cloud (DA XONG / MOT PHAN BO QUA)
- [x] Tao Supabase project, chay `supabase_schema.sql` (bang `questions` + RLS + Realtime)
- [x] Upload `knowledge_base_mau.md` len Dify lam knowledge base
- [x] Tao Dify app (chatbot voi knowledge base truong)
- [ ] Khong su dung Dify lam LLM truc tiep cho ESP32: da bo qua do Xiaozhi cloud dung MQTT/WebSocket rieng, khong nhan OpenAI REST truc tiep
- [ ] Knowledge base cua Xiaozhi.me: da bo qua vi tai khoan chi hien “Khong dung kho kien thuc”, khong co chuc nang upload file
- [x] Lay API key Dify: `app-QvbI...` (da luu trong .env)
- [x] Lay API key Gemini: `AQ.Ab8R...` (da luu trong .env)
- [x] Cau hinh .env day du (Dify, Gemini, Supabase, Device ID)
- [x] File `dify_bridge.py` da san sang (Flask server, /v1/chat/completions)
- [x] File `dashboard.html` da thiet ke lai hoan toan — giao dien chuyen nghiep, dark theme xanh navy, gioi thieu nganh QTCSDL cho hoc sinh

### 3. Deploy & DevOps (DA XONG)
- [x] Đã thử tích hợp MCP custom WebSocket của Xiaozhi.me nhưng không dùng tiếp: MCP endpoint chỉ là endpoint để công cụ kết nối vào, không tự gửi lịch sử hội thoại ra webhook
- [x] Đã bỏ qua phương án chạy WebSocket listener MCP trên Render vì không có dữ liệu hội thoại phù hợp để ghi log
- [x] Phương án đang dùng: firmware ESP32 tự POST question/answer trực tiếp tới Render `/webhook/log`
- [x] Cai dat Git va GitHub CLI (winget install)
- [x] Tao GitHub repo: https://github.com/ducan1911/AItuyensinh
- [x] Push code len GitHub (da co .gitignore bao ve .env)
- [x] Tao `requirements.txt` (flask, requests, python-dotenv, gunicorn)
- [x] Tao `Procfile` (cho Railway/Heroku) va `render.yaml` (cho Render)
- [x] Cap nhat `dify_bridge.py` — dung os.environ.get() de chong crash, ho tro port dong
- [x] Chuyen tu Railway (tra phi sau 30 ngay) sang Render (mien phi vinh vien)
- [x] Deploy `dify_bridge.py` len Render — **DA THANH CONG**
- [x] Cau hinh env vars tren Render
- [x] Test API tren cloud — Dify tra loi OK, Gemini phan loai OK
- **URL API:** `https://aituyensinh-bridge.onrender.com/v1/chat/completions`
- **URL ghi log firmware:** `https://aituyensinh-bridge.onrender.com/webhook/log`
- Render endpoint `/webhook/log` da them de ghi question/answer/category vao Supabase.
- Firmware gui log bang HTTP HTTPS task nen de tranh watchdog; dung CA GTS Root R4 va timeout 30 giay.

### 4. Phan mem (DA CO BAN)
- [x] Cai ESP-IDF 6.0.2 va build firmware Xiaozhi
- [x] Flash firmware len ESP32-S3 qua COM6
- [x] Chon board Bread Compact WiFi + LCD
- [x] Sua GPIO trong `main/boards/bread-compact-wifi-lcd/config.h`:
  - Mic: WS 17, SCK 16, DIN 18
  - PCM5102A: DOUT 14, BCLK 7, LRCK 15
  - ILI9341: MOSI 11, CLK 12, CS 10, DC 9, RST 8
  - Nut noi: GPIO 47
- [x] Sua nut GPIO 47: nhan giu bat dau nghe, tha nut dung nghe
- [x] Test end-to-end: mic → Xiaozhi → loa PCM5102A → nghe tra loi
- [x] Them firmware gui cau hoi/cau tra loi ve Render `/webhook/log`
- [x] Du lieu da ghi vao Supabase va hien tren dashboard sau khi tai lai
- [ ] Kiem tra/fix Supabase Realtime de dashboard cap nhat khong can tai lai
- [x] Xiaozhi da nhan va tra loi tieng Viet qua MQTT; STT doc duoc cau hoi tieng Viet
- [x] Da soan noi dung vai tro tieng Viet: tro ly tuyen sinh, than thien, ngan gon, hoi “danh da” vua phai
- [ ] Chua dan noi dung vai tro moi vao Xiaozhi.me (can lam ngay mai)
- [ ] Chua upload duoc `knowledge_base_mau.md` vao Xiaozhi.me vi muc Cơ sở tri thức chi co “Khong dung kho kien thuc”; da bo qua phuong an nay
- [x] LCD da doi tu board OLED sang board `bread-compact-wifi-lcd`; log xac nhan `SKU=bread-compact-wifi-lcd`, LVGL da khoi dong
- [ ] LCD chua duoc tuy chinh noi dung rieng cua truong; hien dang dung giao dien LVGL mac dinh cua Xiaozhi
- [ ] Chua them ten truong, logo, hotline, dia chi va thong tin tuyen sinh vao giao dien LCD
- [ ] Chua xu ly canh bao `Emoji not found: microchip_ai`
- [ ] Sua emoji `microchip_ai` dang bao `Emoji not found` neu can
- [ ] Chay test build/flash cuoi cung sau cac thay doi UI

---

## ICON CON MAT — KE HOACH SU DUNG

### Bo GIF da co (thu muc `Gif/`)

21 file GIF bieu cam — map voi tung trang thai Xiaozhi:

| File GIF | Trang thai Xiaozhi | Khi nao hien |
|---|---|---|
| `1macdinh.gif` | IDLE | Man hinh mac dinh khi khoi dong |
| `binhthuong.gif` | IDLE | Cho, khong co ai |
| `buonngu.gif` | IDLE (lau) | Screensaver / khong co ai hoi lau |
| `chamhoi.gif` | LISTENING | Vua nhan nut, chuan bi nghe |
| `domohoi.gif` | LISTENING | Dang thu am, nghe cau hoi |
| `suynghi2.gif` | THINKING | Dang gui len server, cho AI xu ly |
| `doxet.gif` | THINKING | Dang tim kiem trong knowledge base |
| `duamat.gif` | THINKING | Phuong an du phong cho suy nghi |
| `vuimung.gif` | SPEAKING | Dang doc cau tra loi qua loa |
| `vuive.gif` | SPEAKING | Tra loi xong, vui ve |
| `cuoito.gif` | SPEAKING | Cau tra loi vui / hoi ve hoc bong |
| `camlang.gif` | SPEAKING | Dang phat am thanh (mieng im) |
| `ngacnhien2.gif` | NEW_QUESTION | Vua nhan duoc cau hoi moi |
| `nhaymat.gif` | BOOT | Chao mung / khoi dong thanh cong |
| `nheomat.gif` | BOOT | Ket noi WiFi thanh cong |
| `hoamat.gif` | CONNECTING | Dang ket noi WiFi / server |
| `tucgian.gif` | ERROR | Loi ket noi / timeout |
| `tucgian2.gif` | ERROR | Khong hieu cau hoi |
| `khongchiudau.gif` | ERROR | Khong tim duoc cau tra loi |
| `buon.gif` | ERROR | Loi nhe / thu lai |
| `buon2.gif` | ERROR | Loi nghiem trong |

---

### Ke hoach trien khai GIF len ILI9341

**Buoc C — Lam SAU khi he thong chay duoc am thanh (Buoc B xong):**

#### C1. Chuan bi cong cu chuyen doi GIF → C array

GIF khong hien truc tiep len ILI9341 duoc — can chuyen sang mang C (RGB565).
Dung script Python sau (chay 1 lan cho tat ca file):

```python
# convert_gif.py — dat o thu muc goc du an
# Cai thu vien: pip install Pillow

from PIL import Image
import os

GIF_DIR = "Gif"
OUT_DIR = "xiaozhi-esp32/main/assets"
os.makedirs(OUT_DIR, exist_ok=True)

TARGET_W, TARGET_H = 240, 240  # resize cho vua man hinh

def gif_to_c(gif_path, out_path, var_name):
    img = Image.open(gif_path)
    frames = []
    try:
        while True:
            frame = img.convert("RGB").resize((TARGET_W, TARGET_H))
            pixels = []
            for y in range(TARGET_H):
                for x in range(TARGET_W):
                    r, g, b = frame.getpixel((x, y))
                    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                    pixels.append(rgb565)
            frames.append(pixels)
            img.seek(img.tell() + 1)
    except EOFError:
        pass

    with open(out_path, "w") as f:
        f.write(f"// Auto-generated from {os.path.basename(gif_path)}\n")
        f.write(f"#pragma once\n")
        f.write(f"#include <stdint.h>\n\n")
        f.write(f"#define {var_name.upper()}_FRAMES {len(frames)}\n")
        f.write(f"#define {var_name.upper()}_W {TARGET_W}\n")
        f.write(f"#define {var_name.upper()}_H {TARGET_H}\n\n")
        f.write(f"const uint16_t {var_name}[{len(frames)}][{TARGET_W * TARGET_H}] PROGMEM = {{\n")
        for fi, frame in enumerate(frames):
            f.write("  {" + ",".join(f"0x{p:04X}" for p in frame) + "},\n")
        f.write("};\n")

    print(f"OK: {var_name} — {len(frames)} frames")

# Map ten file → ten bien C
GIF_MAP = {
    "1macdinh.gif":      "gif_idle",
    "binhthuong.gif":    "gif_normal",
    "buonngu.gif":       "gif_sleepy",
    "chamhoi.gif":       "gif_ready",
    "domohoi.gif":       "gif_listen",
    "suynghi2.gif":      "gif_think",
    "doxet.gif":         "gif_think2",
    "duamat.gif":        "gif_think3",
    "vuimung.gif":       "gif_happy",
    "vuive.gif":         "gif_happy2",
    "cuoito.gif":        "gif_laugh",
    "camlang.gif":       "gif_speak",
    "ngacnhien2.gif":    "gif_surprise",
    "nhaymat.gif":       "gif_wink",
    "nheomat.gif":       "gif_squint",
    "hoamat.gif":        "gif_connect",
    "tucgian.gif":       "gif_angry",
    "tucgian2.gif":      "gif_angry2",
    "khongchiudau.gif":  "gif_confused",
    "buon.gif":          "gif_sad",
    "buon2.gif":         "gif_sad2",
}

for fname, vname in GIF_MAP.items():
    gif_to_c(
        os.path.join(GIF_DIR, fname),
        os.path.join(OUT_DIR, f"{vname}.h"),
        vname
    )

print("XONG! Kiem tra thu muc:", OUT_DIR)
```

Chay:
```bash
python convert_gif.py
```

---

#### C2. Code C++ hien thi GIF tren ILI9341

Tao file `gif_player.h` trong `xiaozhi-esp32/main/`:

```cpp
// gif_player.h
#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

// Include cac file anh da convert
#include "assets/gif_idle.h"
#include "assets/gif_listen.h"
#include "assets/gif_think.h"
#include "assets/gif_happy.h"
#include "assets/gif_speak.h"
#include "assets/gif_sad.h"
#include "assets/gif_angry.h"
#include "assets/gif_connect.h"
#include "assets/gif_sleepy.h"
#include "assets/gif_surprise.h"
#include "assets/gif_wink.h"

extern LGFX lcd;  // man hinh ILI9341 da khai bao o main

enum EyeState {
  EYE_IDLE,
  EYE_LISTENING,
  EYE_THINKING,
  EYE_SPEAKING,
  EYE_ERROR,
  EYE_CONNECTING,
  EYE_BOOT
};

struct GifAnim {
  const uint16_t (*frames)[GIF_IDLE_W * GIF_IDLE_H];
  int frameCount;
  int w, h;
  int delayMs;
};

// Khai bao cac animation
GifAnim ANIM_IDLE     = { gif_idle,     GIF_IDLE_FRAMES,     GIF_IDLE_W,     GIF_IDLE_H,     80  };
GifAnim ANIM_LISTEN   = { gif_listen,   GIF_LISTEN_FRAMES,   GIF_LISTEN_W,   GIF_LISTEN_H,   60  };
GifAnim ANIM_THINK    = { gif_think,    GIF_THINK_FRAMES,    GIF_THINK_W,    GIF_THINK_H,    70  };
GifAnim ANIM_SPEAK    = { gif_happy,    GIF_HAPPY_FRAMES,    GIF_HAPPY_W,    GIF_HAPPY_H,    60  };
GifAnim ANIM_ERROR    = { gif_sad,      GIF_SAD_FRAMES,      GIF_SAD_W,      GIF_SAD_H,      80  };
GifAnim ANIM_CONNECT  = { gif_connect,  GIF_CONNECT_FRAMES,  GIF_CONNECT_W,  GIF_CONNECT_H,  70  };
GifAnim ANIM_BOOT     = { gif_wink,     GIF_WINK_FRAMES,     GIF_WINK_W,     GIF_WINK_H,     80  };

EyeState currentState = EYE_BOOT;
GifAnim* currentAnim  = &ANIM_BOOT;
int currentFrame = 0;
unsigned long lastFrameTime = 0;

void setEyeState(EyeState state) {
  if (state == currentState) return;
  currentState = state;
  currentFrame = 0;
  switch (state) {
    case EYE_IDLE:       currentAnim = &ANIM_IDLE;    break;
    case EYE_LISTENING:  currentAnim = &ANIM_LISTEN;  break;
    case EYE_THINKING:   currentAnim = &ANIM_THINK;   break;
    case EYE_SPEAKING:   currentAnim = &ANIM_SPEAK;   break;
    case EYE_ERROR:      currentAnim = &ANIM_ERROR;   break;
    case EYE_CONNECTING: currentAnim = &ANIM_CONNECT; break;
    case EYE_BOOT:       currentAnim = &ANIM_BOOT;    break;
  }
}

void tickGif() {
  if (millis() - lastFrameTime < currentAnim->delayMs) return;
  lastFrameTime = millis();

  int x = (320 - currentAnim->w) / 2;
  int y = (240 - currentAnim->h) / 2;

  lcd.pushImage(x, y, currentAnim->w, currentAnim->h,
                currentAnim->frames[currentFrame]);

  currentFrame = (currentFrame + 1) % currentAnim->frameCount;
}
```

---

#### C3. Tich hop vao main.cpp cua Xiaozhi

Tim doan code xu ly trang thai trong `main.cpp` / `app_main.cpp`, them goi ham:

```cpp
#include "gif_player.h"

// Trong loop() hoac task chinh:
void loop() {
  tickGif();  // Goi lien tuc de chay animation

  // ...code Xiaozhi hien co...
}

// Khi trang thai thay doi, goi setEyeState():
// Vi du trong callback Xiaozhi:
void onStateChange(XiaozhiState s) {
  switch (s) {
    case XIAOZHI_IDLE:      setEyeState(EYE_IDLE);       break;
    case XIAOZHI_LISTENING: setEyeState(EYE_LISTENING);  break;
    case XIAOZHI_THINKING:  setEyeState(EYE_THINKING);   break;
    case XIAOZHI_SPEAKING:  setEyeState(EYE_SPEAKING);   break;
    case XIAOZHI_ERROR:     setEyeState(EYE_ERROR);      break;
  }
}
```

---

#### C4. Thu tu thuc hien toi nay

1. [ ] Hoan thanh Buoc B (dau day PCM5102, flash Xiaozhi, test am thanh)
2. [ ] Chay `python convert_gif.py` → sinh ra 21 file `.h`
3. [ ] Copy cac file `.h` vao `xiaozhi-esp32/main/assets/`
4. [ ] Tao file `gif_player.h` nhu tren
5. [ ] Them `#include "gif_player.h"` va goi `tickGif()` vao `main.cpp`
6. [ ] Them goi `setEyeState()` theo tung trang thai Xiaozhi
7. [ ] Build lai va flash — kiem tra mat hien dung animation

> **Luu y quan trong:**
> - GIF to (nhieu frame, do phan giai cao) se chiem RAM/Flash nhieu → neu loi bo nho, giam `TARGET_W/H` xuong 120x120
> - ILI9341 la 320x240 (landscape) hoac 240x320 (portrait) — can check huong man hinh
> - Neu GIF co nen trong suot (alpha) → nen dat nen mau den `#000000` truoc khi convert

## GHI CHU PHIEN LAM VIEC HIEN TAI

### Cac viec da thu nhung da bo qua
- [x] Da thu chọn board `bread-compact-wifi` nhưng sai vi board nay khoi tao OLED SSD1306; da bo qua.
- [x] Board dang dung phai la `bread-compact-wifi-lcd`, khong chon ban `+ camera` vi khong dung camera va chan camera xung dot voi day hien tai.
- [x] Da thu dùng GPIO 0/BOOT cho nut noi nhung sai; da bo qua. Nut noi that la GPIO 47.
- [x] Da thu `ToggleChatState()` cho nut GPIO 47 nhung khong phu hop nhan-giu; da sua sang `StartListening()` khi nhan va `StopListening()` khi tha.
- [x] Da thu doi phat audio sang stereo `I2S_STD_SLOT_BOTH` nhung khong can; da undo, giu code `NoAudioCodecSimplex` ban dau. Am thanh im truoc do do day PCM5102A long, khong phai do code.
- [x] Da them task nen gui log HTTPS de tranh watchdog; khong gui HTTP dong bo trong callback MQTT.
- [x] Da gap loi TLS voi Render; da them CA GTS Root R4, timeout 30 giay. Log ghi du lieu da thanh cong sau khi thay day va flash.

### Trang thai da xac nhan
- Firmware dang chay board `bread-compact-wifi-lcd`, log hien `SKU=bread-compact-wifi-lcd`.
- LCD ILI9341 da duoc khoi tao bang LVGL, khong con loi SSD1306.
- WiFi `CHU NHA` ket noi thanh cong; MQTT Xiaozhi ket noi thanh cong.
- Nhan giu nut GPIO 47 da tao duoc trang thai listening va STT nhan duoc cau hoi.
- Xiaozhi da tra loi bang tieng Viet qua loa PCM5102A sau khi sua day long.
- Dashboard doc du lieu Supabase duoc, nhung hien phai tai lai trang moi thay ban ghi moi; can kiem tra Realtime.
- Emoji `microchip_ai` bao `Emoji not found`, khong anh huong am thanh.

### Viec can lam ngay mai — thu tu uu tien
1. Dan vai tro tieng Viet da soan vao Xiaozhi.me, chon ngon ngu tieng Viet neu co; khong upload knowledge base vi tai khoan khong ho tro.
2. Test lai mot cau hoi tuyen sinh: “Truong co nhung nganh nao?” va kiem tra AI tra loi dung noi dung truong, khong tra loi lan man.
3. Kiem tra Supabase Realtime: mo `http://localhost:5500/dashboard.html`, F12 Console, xac nhan channel co trang thai `SUBSCRIBED`; neu khong, vao Supabase bat Realtime cho bang `questions`.
4. Tuy chinh LCD trong `main/display/lcd_display.cc` hoac board LCD: hien ten truong, “Trung cap KTKT Can Tho”, dia chi, hotline, WiFi va trang thai nghe/suy nghi/noi.
5. Khong sua lai GPIO neu khong can. Giu dung: mic 16/17/18; PCM5102A 7/15/14; LCD 11/12/10/9/8; nut noi GPIO 47.
6. Build/flash va kiem tra log: `Application: >>`, `Application: <<`, `Conversation log sent to Render`.
7. Neu can sua giao dien, chi sua sau khi da test am thanh va ghi log thanh cong.

### Lenh build/flash
```powershell
idf.py fullclean
idf.py build
idf.py -p COM6 flash
idf.py -p COM6 monitor
```

## BUOC TIEP THEO (THU TU UU TIEN)

### A. Khong can PCM5102 — DA XONG:
1. **Deploy `dify_bridge.py` len Render (Mien phi):** — **DA XONG 16/07/2026**
   - Tao `render.yaml` de tu dong hoa deploy
   - Them env vars vao Render dashboard
   - URL: `https://aituyensinh-bridge.onrender.com`

2. **Test dify_bridge tren cloud:** — **DA XONG 16/07/2026**
   - Gui curl test — Dify tra loi dung tu knowledge base
   - Kiem tra API endpoint hoat dong — Health check OK

### B. Can PCM5102 — LAM TOI NAY 17/07/2026 (DA CO LINH KIEN):

3. **Dau noi PCM5102A theo so do:**

| PCM5102A | ESP32-S3 | Ghi chu |
|---|---|---|
| VIN | 3.3V | |
| GND | GND | |
| BCK | GPIO 7 | I2S bit clock |
| LCK | GPIO 15 | I2S word select |
| DIN | GPIO 14 | I2S data out |
| SCK | **GND** | Khong dung master clock |
| FLT | GND | Bo loc chuan |
| DEMP | GND | Tat de-emphasis |
| FMT | GND | Dinh dang I2S chuan |
| XSMT | **3.3V** | ⚠️ QUAN TRONG: de GND se bi im tieng! |
| OUT (jack 3.5mm) | Cap AUX → loa | Cam vao AUX IN cua loa Bluetooth |

4. **Cai firmware Xiaozhi:**
   - Clone repo: `git clone https://github.com/78/xiaozhi-esp32.git`
   - Mo bang VS Code + PlatformIO hoac Arduino IDE
   - Chon board config: "Bread Compact WiFi"
   - Sua `config.h` cho khop voi phan cung thuc te:
   ```c
   // MIC (INMP441)
   #define I2S_MIC_SCK   16
   #define I2S_MIC_WS    17
   #define I2S_MIC_SD    18

   // SPEAKER (PCM5102A)
   #define I2S_SPK_BCK   7
   #define I2S_SPK_WS    15
   #define I2S_SPK_DOUT  14

   // NUT NHAN
   #define BTN_PIN       47
   ```
   - Build va flash len ESP32-S3

5. **Cau hinh Xiaozhi** (lan dau boot se tao WiFi AP de config):
   - LLM Base URL: `https://aituyensinh-bridge.onrender.com/v1`
   - WiFi SSID/password cua buoi tu van

6. **Test end-to-end:**
   - Nhan giu nut → noi → loa phat tieng tra loi
   - Mo dashboard.html → kiem tra cau hoi xuat hien realtime
   - Neu loa im tieng → kiem tra lai chan XSMT co noi 3.3V chua

---

## CAC FILE TRONG DU AN

| File | Muc dich | Trang thai |
|---|---|---|
| `.env` | API keys (Dify, Gemini, Supabase) | DA CAU HINH |
| `dify_bridge.py` | Backend Flask proxy | **DA DEPLOY LEN RAILWAY** |
| `dashboard.html` | Dashboard realtime + Screensaver + WordCloud + Waveform + Glow | DA CAP NHAT (17/07/2026) |
| `admin.html` | Trang quan tri BGH — lich su day du + xuat Excel | MOI TAO (17/07/2026) |
| `supabase_schema.sql` | Schema bang questions | DA CHAY TREN SUPABASE |
| `knowledge_base_mau.md` | Thong tin truong cho Dify RAG | DA UPLOAD LEN DIFY |
| `test_hardware_v2/test_hardware_v2.ino` | Test phan cung v2 (Freenove) | DA CHAY OK |
| `test_hardware.ino` | Test phan cung v1 (DevKitC goc) | KHONG SU DUNG (board khac) |
| `test.json` | Payload curl de test bridge | SAN SANG |
| `requirements.txt` | Dependencies Python cho Railway | **MOI TAO** |
| `Procfile` | Start command cho Railway (gunicorn) | **MOI TAO** |
| `railway.toml` | Cau hinh Railway deploy | **MOI TAO** |
| `.gitignore` | Bao ve .env khoi GitHub | **MOI TAO** |
| `KE_HOACH_TRIEN_KHAI.md` | Ke hoach tong the | THAM KHAO |
| `KE_HOACH_PROTOTYPE.md` | Ke hoach prototype | THAM KHAO |
| `HUONG_DAN_CHI_TIET_CODE.md` | Huong dan code | THAM KHAO |
| `HUONG_DAN_DAU_NOI.md` | Huong dan dau noi Freenove | DA THUC HIEN XONG |

---

## THONG TIN KY THUAT QUAN TRONG

- **Board:** Freenove ESP32-S3 WROOM CAM (N16R8) — KHONG phai DevKitC-1 goc
- **GPIO cam dung:** 26-37 (PSRAM), 19-20 (USB)
- **Man hinh:** ILI9341 dung thu vien LovyanGFX (Panel_ST7789 driver)
- **Supabase URL:** https://vkhxkwlmbyivcedrgstz.supabase.co
- **Dify model:** dang dung Dify cloud (api.dify.ai)
- **Gemini model:** gemini-3.5-flash
- **7 categories phan loai:** Quan tri CSDL, KHMT/Lap trinh, ATTT, Kinh te-QTKD, Ngon ngu-XHNH, Hoc phi-Hoc bong, Khac

## CAP NHAT PHIEN LAM VIEC 18/07/2026

### Trang thai da xac nhan
- WiFi da ket noi thanh cong toi `Truong TC KTKT TPCT`, IP `192.168.1.234`.
- MQTT Xiaozhi da ket noi thanh cong.
- Monitor xac nhan firmware chay board `bread-compact-wifi-lcd` va driver `ili9341` truoc khi chuyen sang test ST7789.
- ESP32-S3, mic, nut GPIO 47, PCM5102A va luong hoi dap van hoat dong.
- Cac thay doi LCD da thuc hien: logo truong nhung RGB565, giao dien trang, ten truong, trang thai WiFi/nghe/tra loi, cau hoi va cau tra loi.
- Da them pattern test TOP/BOTTOM/LEFT/RIGHT va 4 o mau de chan doan huong man hinh; pattern da duoc xoa sau khi test.
- Da xac dinh man hinh thuc te dang cho ket qua dung voi cau hinh `ST7789 240x320`; menuconfig dang chon `Xiaozhi Assistant -> LCD Type -> ST7789 240*320`.

### Cau hinh LCD hien tai can giu
- Board: `Bread Compact WiFi + LCD` (`CONFIG_BOARD_TYPE_BREAD_COMPACT_WIFI_LCD=y`).
- LCD: `ST7789 240x320`, `DISPLAY_WIDTH=240`, `DISPLAY_HEIGHT=320`.
- `DISPLAY_MIRROR_X=false`, `DISPLAY_MIRROR_Y=false`, `DISPLAY_SWAP_XY=false`.
- `DISPLAY_OFFSET_X=0`, `DISPLAY_OFFSET_Y=0`, `DISPLAY_INVERT_COLOR=false`.
- Da xoa test overlay; giao dien logo/trang thai/chat da bat lai.
- Khong sua MADCTL ILI9341 nua neu dang test ST7789.

### Viec tiep theo
1. Build sach va flash firmware ST7789:
   ```powershell
   python C:\esp\v6.0.2\esp-idf\tools\idf.py fullclean
   python C:\esp\v6.0.2\esp-idf\tools\idf.py build
   python C:\esp\v6.0.2\esp-idf\tools\idf.py -p COM6 flash
   ```
2. Mo monitor:
   ```powershell
   python C:\esp\v6.0.2\esp-idf\tools\idf.py -p COM6 monitor
   ```
3. Kiem tra LCD: logo dung mau, nen trang, chu dung chieu, khong soc nhieu/lech vien.
4. Kiem tra nhan giu GPIO 47: phai hien `Dang nghe...`, sau do `Dang tra loi...` va hien cau hoi/cau tra loi.
5. Neu LCD con lech, chi dieu chinh `DISPLAY_OFFSET_X/Y` sau khi xac dinh so pixel lech thuc te; khong doi GPIO.
6. Sau khi LCD on dinh, tiep tuc kiem tra Supabase Realtime va tuy chinh giao dien truong.

### Luu y moi truong build
- Neu `idf.py` bi Windows mo bang Node.js, dung Python truc tiep:
  ```powershell
  python C:\esp\v6.0.2\esp-idf\tools\idf.py build
  ```
- Neu thieu Python environment ESP-IDF:
  ```powershell
  C:\Espressif\tools\python\v6.0.2\venv\Scripts\python.exe C:\esp\v6.0.2\esp-idf\tools\idf_tools.py install-python-env
  ```
- Chi flash sau khi build thanh cong. Khong xoa flash vi se mat cau hinh WiFi.

## CAP NHAT PHIEN LAM VIEC - CHAN DOAN MIC/STT

- [x] Sua chuyen doi du lieu INMP441 tu I2S 32-bit sang PCM 16-bit bang phep dich `>> 16`, giup STT nhan cau hoi ro hon.
- [x] Them log chan doan mic RMS/Peak trong `main/audio/codecs/no_audio_codec.cc`.
- [x] Log moi giay co dang: `Mic level: RMS=... Peak=...` de xac dinh mic qua nho, qua lon, clipping hoac nhieu.
- [ ] Flash firmware moi va ghi lai RMS/Peak khi phong yen va khi noi cach mic 10-15 cm.
- [ ] Neu Peak gan 32767 thuong xuyen: giam gain/kiem tra clipping; neu RMS rat thap: kiem tra huong mic, L/R va day INMP441.
- [ ] Kiem tra STT trong moi truong dong nguoi sau khi xac nhan muc tin hieu mic on dinh.

## CAP NHAT CAI TIEN STT VA LCD

- [x] Bo sung chan doan RMS/Peak mic theo chu ky 1 giay.
- [x] Them canh bao `Mic level low` when tin hieu qua nho va `Mic clipping risk` when Peak gan nguong bao hoa.
- [x] Xac nhan `CONFIG_USE_AUDIO_PROCESSOR=y`; khong bat them AEC mu de tranh lam meo tin hieu INMP441.
- [x] LCD giu waveform animation chi hien thi khi trang thai dang nghe.
- [x] LCD hien cau tra loi chay ngang mot dong, tranh hien tuong tach thanh nhieu phan tren duoi.
- [ ] Flash va thu nghiem trong phong dong nguoi; dua cac dong RMS/Peak va cau STT nhan sai vao log de tinh chinh tiep.

## CAP NHAT CAO CAP: TRIEN KHAI SERVER RIENG CHO XIAOZHI TRÊN RENDER

- [x] Tạo thư mục riêng `xiaozhi-custom-server/` chứa WebSocket Server độc lập tương thích hoàn toàn giao thức của Xiaozhi.
- [x] Server giải mã Opus nhị phân trực tiếp từ ESP32, gom buffer âm thanh để gọi API Groq Whisper (miễn phí, nhanh) tăng chất lượng STT tiếng Việt.
- [x] Tích hợp Edge-TTS giọng đọc tiếng Việt chất lượng cao (vi-VN-HoaiMyNeural) hoàn toàn miễn phí.
- [x] Tự động cập nhật câu trả lời từ RAG Dify, phân loại bằng Gemini, lưu trữ trực tiếp vào Supabase để hiển thị trên dashboard.
- [x] Cấu hình `render.yaml` để chạy hai dịch vụ song song: `aituyensinh-bridge` (cũ) và `xiaozhi-custom-server` (mới).
- [ ] Thêm khóa `GROQ_API_KEY` vào file `.env` và Render Environment.
