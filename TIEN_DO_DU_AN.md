# TIẾN ĐỘ DỰ ÁN: Kiosk AI Tư vấn Tuyển sinh
> File này dùng để AI phiên sau đọc và tiếp tục công việc.
> Cập nhật lần cuối: 16/07/2026 (21:30)

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
| **PCM5102 / PCM5102A (DAC am thanh)** | **CHUA CO - CAN MUA** |
| Cap AUX 3.5mm | **CHUA CO - CAN MUA** |

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
- [x] Dau noi INMP441 (mic I2S) → ESP32-S3 — da test OK
- [x] Dau noi nut nhan → GPIO 47 — da test OK
- [x] Chay file `test_hardware_v2.ino` — tat ca thiet bi hoat dong binh thuong
- [ ] Dau noi PCM5102 (DAC) → ESP32-S3 — **CHUA CO LINH KIEN**

### 2. Phan mem - Cloud (DA XONG)
- [x] Tao Supabase project, chay `supabase_schema.sql` (bang `questions` + RLS + Realtime)
- [x] Upload `knowledge_base_mau.md` len Dify lam knowledge base
- [x] Tao Dify app (chatbot voi knowledge base truong)
- [x] Lay API key Dify: `app-QvbI...` (da luu trong .env)
- [x] Lay API key Gemini: `AQ.Ab8R...` (da luu trong .env)
- [x] Cau hinh .env day du (Dify, Gemini, Supabase, Device ID)
- [x] File `dify_bridge.py` da san sang (Flask server, /v1/chat/completions)
- [x] File `dashboard.html` da thiet ke lai hoan toan — giao dien chuyen nghiep, dark theme xanh navy, gioi thieu nganh QTCSDL cho hoc sinh

### 3. Deploy & DevOps (DA XONG)
- [x] Cai dat Git va GitHub CLI (winget install)
- [x] Tao GitHub repo: https://github.com/ducan1911/AItuyensinh
- [x] Push code len GitHub (da co .gitignore bao ve .env)
- [x] Tao `requirements.txt` (flask, requests, python-dotenv, gunicorn)
- [x] Tao `Procfile` (gunicorn dify_bridge:app)
- [x] Tao `railway.toml` (start command)
- [x] Cap nhat `dify_bridge.py` — dung os.environ.get() thay os.environ[] de tranh crash khi thieu env vars
- [x] Cap nhat `dify_bridge.py` — doc PORT tu env (Railway gan port dong)
- [x] Deploy `dify_bridge.py` len Railway — **DA THANH CONG**
- [x] Cau hinh env vars tren Railway dashboard
- [x] Test API tren cloud — Dify tra loi OK, Gemini phan loai OK
- **URL API:** `https://web-production-11e6b.up.railway.app/v1/chat/completions`
- **Health check:** `https://web-production-11e6b.up.railway.app/health`

### 4. Phan mem - Chua lam
- [ ] Deploy `dify_bridge.py` len Railway — ~~DA XONG (xem muc 3)~~
- [ ] Cai dat va build firmware Xiaozhi len ESP32-S3
- [ ] Cau hinh Xiaozhi tro den URL: `https://web-production-11e6b.up.railway.app/v1`
- [ ] Test end-to-end: noi vao mic → nghe tra loi qua loa → thay du lieu tren dashboard
- [ ] Ket noi loa Bluetooth qua PCM5102 + cap AUX 3.5mm

---

## BUOC TIEP THEO (THU TU UU TIEN)

### A. Khong can PCM5102 — ~~lam ngay duoc~~ DA XONG:
1. ~~**Deploy `dify_bridge.py` len Railway:**~~ — **DA XONG 16/07/2026**
   - ~~Tao `requirements.txt` (flask, requests, python-dotenv)~~ ✅
   - ~~Tao tai khoan Railway.app → New Project → deploy tu GitHub hoac CLI~~ ✅
   - ~~Them env vars tu file `.env` vao Railway dashboard~~ ✅
   - URL: `https://web-production-11e6b.up.railway.app`

2. ~~**Test dify_bridge tren cloud:**~~ — **DA XONG 16/07/2026**
   - ~~Gui curl test~~ ✅ — Dify tra loi dung tu knowledge base
   - ~~Kiem tra API endpoint hoat dong~~ ✅ — Health check OK

### B. Can PCM5102 — lam khi co linh kien:
3. **Dau noi PCM5102 theo so do o tren**
4. **Cai firmware Xiaozhi:**
   - Clone repo: `git clone https://github.com/78/xiaozhi-esp32.git`
   - Chon board config: "Bread Compact WiFi"
   - Sua `config.h` de khop voi so do chan thuc te (MIC: 16/17/18, SPK: 7/15/14, BTN: 47)
   - Build va flash len ESP32-S3
5. **Cau hinh Xiaozhi:**
   - LLM Base URL: `https://web-production-11e6b.up.railway.app/v1`
   - WiFi SSID/password cua buoi tu van
6. **Test end-to-end**

---

## CAC FILE TRONG DU AN

| File | Muc dich | Trang thai |
|---|---|---|
| `.env` | API keys (Dify, Gemini, Supabase) | DA CAU HINH |
| `dify_bridge.py` | Backend Flask proxy | **DA DEPLOY LEN RAILWAY** |
| `dashboard.html` | Dashboard realtime + pipeline demo | DA CAP NHAT (co DB demo + SQL visualization) |
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
