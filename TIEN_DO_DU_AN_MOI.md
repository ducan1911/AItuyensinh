# TIẾN ĐỘ DỰ ÁN MỚI: Tích hợp WebSocket Server Xiaozhi với Groq Whisper và Edge-TTS

## Tổng quan giải pháp mới
Để cải thiện độ chính xác khi nhận dạng giọng nói tiếng Việt cho robot Xiaozhi (thay thế hệ thống STT mặc định chất lượng kém của server Xiaozhi.me), chúng tôi đã xây dựng một **WebSocket Server riêng biệt** đặt tại thư mục `xiaozhi-custom-server/`.

### Sơ đồ luồng hoạt động mới:
`Mic INMP441` ➔ `ESP32-S3 (Opus 16kHz)` ➔ `Render: xiaozhi-custom-server` ➔ `Giải mã Opus sang PCM` ➔ `Groq Whisper API (STT tiếng Việt)` ➔ `Dify RAG (Trả lời)` ➔ `Edge-TTS (Giọng Hoài My)` ➔ `Mã hóa Opus` ➔ `Loa PCM5102A phát lại`.

---

## Các thành phần đã triển khai
1. **Thư mục code mới**: `xiaozhi-custom-server/` chứa:
   * `server.py`: Mã nguồn WebSocket Server điều khiển và xử lý âm thanh.
   * `requirements.txt`: Các thư viện phục vụ (websockets, edge-tts, pydub, opuslib-next).
   * `Dockerfile`: Tự động build môi trường Linux cài sẵn `libopus-dev` và `ffmpeg`.
2. **Cập nhật Render**:
   * Cập nhật tệp `render.yaml` ở thư mục gốc để Render chạy đồng thời cả web bridge cũ (`aituyensinh-bridge`) và server mới (`xiaozhi-custom-server`) trên tài khoản miễn phí.

---

## Hướng dẫn cấu hình Key Groq (`GROQ_API_KEY`)
Để nhận diện giọng nói tiếng Việt tốt nhất bằng mô hình **Whisper-large-v3**, bạn cần thực hiện:

### Bước 1: Lấy API Key miễn phí từ Groq
1. Truy cập: [https://console.groq.com/keys](https://console.groq.com/keys)
2. Đăng ký tài khoản và tạo một API Key mới (ví dụ: `gsk_xxxxxxxxxxxxxxxxxxxx`).

### Bước 2: Thêm key vào tệp `.env` cục bộ
Mở tệp `.env` ở thư mục gốc dự án và thêm dòng sau:
```env
GROQ_API_KEY=gsk_xxxxxxxxxxxxxxxxxxxx
```

### Bước 3: Cấu hình trên Render
1. Truy cập trang quản trị của dự án trên [Render Dashboard](https://dashboard.render.com).
2. Chọn dịch vụ mới tạo `xiaozhi-custom-server`.
3. Đi tới mục **Environment** -> **Add Environment Variable**.
4. Thêm biến `GROQ_API_KEY` với giá trị key Groq của bạn và nhấn **Save Changes**.

---

## Kế hoạch kiểm tra tiếp theo
1. **Deploy & Kiểm tra log**: Xác nhận server mới build Docker và chạy thành công trên Render.
2. **Cấu hình trên thiết bị ESP32-S3**:
   * Chuyển URL WebSocket đích trên robot từ server cũ sang địa chỉ server mới của bạn (ví dụ: `ws://xiaozhi-custom-server.onrender.com/`).
3. **Thử nghiệm tiếng Việt**: Nhấn nút nói thử một câu tiếng Việt bất kỳ, kiểm tra độ nhạy nhận dạng trên màn hình LCD và loa phát lại.
