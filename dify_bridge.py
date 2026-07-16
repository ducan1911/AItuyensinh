"""
dify_bridge.py
------------------------------------------------------------
Script trung gian chạy trên laptop mang theo sự kiện. Nhiệm vụ:

  1. Nhận câu hỏi (văn bản, đã qua STT) từ xiaozhi-esp32-server
     theo định dạng tương thích OpenAI chat completion.
  2. Gọi Dify (đã có Knowledge Base + system prompt "đanh đá")
     để lấy câu trả lời.
  3. Gọi Gemini (một lượt gọi nhỏ, rẻ) để phân loại câu hỏi vào
     1 trong các nhóm ngành cố định.
  4. Ghi câu hỏi + câu trả lời + category vào Supabase.
  5. Trả câu trả lời về cho xiaozhi-esp32-server theo định dạng
     OpenAI chat completion để nó đọc TTS ra loa.

CÀI ĐẶT:
  pip install flask requests python-dotenv

CHẠY:
  Tạo file .env cùng thư mục (xem mẫu bên dưới), rồi:
  python dify_bridge.py
  (mặc định lắng nghe cổng 8080)

FILE .env MẪU (tạo tay, đừng commit lên git):
  DIFY_BASE_URL=https://your-dify-host/v1
  DIFY_API_KEY=app-xxxxxxxxxxxxxxxx
  GEMINI_API_KEY=AIzaSy...
  GEMINI_MODEL=gemini-2.5-flash-lite
  SUPABASE_URL=https://your-project.supabase.co
  SUPABASE_SERVICE_ROLE_KEY=eyJ...
  DEVICE_ID=kiosk-01
------------------------------------------------------------
"""

import os
import time
import uuid
import logging
import requests
from flask import Flask, request, jsonify
from dotenv import load_dotenv

load_dotenv()

DIFY_BASE_URL = os.environ["DIFY_BASE_URL"].rstrip("/")
DIFY_API_KEY = os.environ["DIFY_API_KEY"]
GEMINI_API_KEY = os.environ["GEMINI_API_KEY"]
GEMINI_MODEL = os.environ.get("GEMINI_MODEL", "gemini-2.5-flash-lite")
SUPABASE_URL = os.environ["SUPABASE_URL"].rstrip("/")
SUPABASE_SERVICE_ROLE_KEY = os.environ["SUPABASE_SERVICE_ROLE_KEY"]
DEVICE_ID = os.environ.get("DEVICE_ID", "kiosk-01")

CATEGORIES = [
    "Quản trị Cơ sở dữ liệu",
    "Khoa học Máy tính / Lập trình",
    "An toàn thông tin",
    "Kinh tế - Quản trị Kinh doanh",
    "Ngôn ngữ - Xã hội Nhân văn",
    "Học phí - Học bổng",
    "Khác",
]
# Danh sách này PHẢI khớp với danh sách demoCats trong dashboard.html
# (chỉ ảnh hưởng dữ liệu demo hiển thị khi chưa nối Supabase thật — dữ liệu
# thật thì dashboard tự vẽ theo bất kỳ giá trị category nào có trong bảng,
# nhưng nên giữ khớp tên để tránh 2 cách viết cho cùng 1 ngành).

FALLBACK_ANSWER = (
    "Xin lỗi, hệ thống đang gặp trục trặc kỹ thuật. "
    "Bạn ghé quầy tư vấn hỏi trực tiếp thầy cô nhé!"
)

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
log = logging.getLogger("dify_bridge")

app = Flask(__name__)


def ask_dify(question: str) -> str:
    """Gửi câu hỏi sang Dify (Knowledge Base + system prompt đã cấu hình sẵn trong app)."""
    try:
        resp = requests.post(
            f"{DIFY_BASE_URL}/chat-messages",
            headers={
                "Authorization": f"Bearer {DIFY_API_KEY}",
                "Content-Type": "application/json",
            },
            json={
                "inputs": {},
                "query": question,
                "response_mode": "blocking",
                "user": DEVICE_ID,
                # Không truyền conversation_id -> mỗi lượt hỏi là 1 hội thoại mới,
                # phù hợp kiosk walk-up (mỗi học sinh hỏi độc lập).
            },
            timeout=20,
        )
        resp.raise_for_status()
        data = resp.json()
        answer = data.get("answer", "").strip()
        return answer if answer else FALLBACK_ANSWER
    except Exception as e:
        log.error("Lỗi gọi Dify: %s", e)
        return FALLBACK_ANSWER


def classify_category(question: str, answer: str) -> str:
    """Gọi Gemini Flash-Lite để phân loại nhanh vào 1 trong CATEGORIES."""
    prompt = (
        "Phân loại câu hỏi sau vào ĐÚNG MỘT nhóm trong danh sách: "
        f"{', '.join(CATEGORIES)}.\n"
        f"Câu hỏi: {question}\n"
        f"Câu trả lời: {answer}\n"
        "Chỉ trả về đúng tên nhóm, không giải thích gì thêm."
    )
    try:
        resp = requests.post(
            "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions",
            headers={
                "Authorization": f"Bearer {GEMINI_API_KEY}",
                "Content-Type": "application/json",
            },
            json={
                "model": GEMINI_MODEL,
                "messages": [{"role": "user", "content": prompt}],
                "temperature": 0,
                "max_tokens": 20,
            },
            timeout=10,
        )
        resp.raise_for_status()
        text = resp.json()["choices"][0]["message"]["content"].strip()
        for c in CATEGORIES:
            if c.lower() in text.lower():
                return c
        return "Khác"
    except Exception as e:
        log.warning("Lỗi phân loại (không chặn luồng chính): %s", e)
        return "Khác"


def log_to_supabase(question: str, answer: str, category: str):
    """Ghi log vào Supabase — lỗi ở bước này KHÔNG được làm gián đoạn câu trả lời cho học sinh."""
    try:
        requests.post(
            f"{SUPABASE_URL}/rest/v1/questions",
            headers={
                "apikey": SUPABASE_SERVICE_ROLE_KEY,
                "Authorization": f"Bearer {SUPABASE_SERVICE_ROLE_KEY}",
                "Content-Type": "application/json",
                "Prefer": "return=minimal",
            },
            json={
                "question": question,
                "answer": answer,
                "category": category,
                "device_id": DEVICE_ID,
            },
            timeout=10,
        )
    except Exception as e:
        log.warning("Lỗi ghi Supabase (không chặn luồng chính): %s", e)


@app.route("/health", methods=["GET"])
def health():
    return jsonify({"status": "ok", "time": time.time()})


@app.route("/v1/chat/completions", methods=["POST"])
def chat_completions():
    """
    Endpoint tương thích OpenAI — trỏ 'LLM base URL' của xiaozhi-esp32-server
    vào http://<ip-laptop>:8080/v1 để nó gọi vào đây như gọi OpenAI bình thường.
    """
    body = request.get_json(force=True, silent=True) or {}
    messages = body.get("messages", [])

    # Lấy nội dung câu hỏi mới nhất của user (bỏ qua system prompt nếu server gửi kèm)
    question = ""
    for m in reversed(messages):
        if m.get("role") == "user":
            question = m.get("content", "")
            break

    if not question:
        answer = "Mình chưa nghe rõ câu hỏi, bạn hỏi lại giúp mình nhé."
        category = "Khác"
    else:
        answer = ask_dify(question)
        category = classify_category(question, answer)
        log_to_supabase(question, answer, category)

    # Trả về đúng khung định dạng OpenAI chat completion
    return jsonify({
        "id": f"chatcmpl-{uuid.uuid4().hex[:12]}",
        "object": "chat.completion",
        "created": int(time.time()),
        "model": GEMINI_MODEL,
        "choices": [{
            "index": 0,
            "message": {"role": "assistant", "content": answer},
            "finish_reason": "stop",
        }],
        "usage": {"prompt_tokens": 0, "completion_tokens": 0, "total_tokens": 0},
    })


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 8080))
    log.info("Dify bridge đang chạy tại http://0.0.0.0:%d (endpoint: /v1/chat/completions)", port)
    app.run(host="0.0.0.0", port=port)
