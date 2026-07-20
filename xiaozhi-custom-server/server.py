import os
import io
import wave
import uuid
import json
import asyncio
import logging
import requests
import websockets
from dotenv import load_dotenv
from pydub import AudioSegment
import edge_tts
import opuslib_next

# Set up logging
logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
log = logging.getLogger("xiaozhi_custom_server")

# Load environment variables
dotenv_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), ".env")
load_dotenv(dotenv_path)

DIFY_BASE_URL = os.environ.get("DIFY_BASE_URL", "https://api.dify.ai/v1").rstrip("/")
DIFY_API_KEY = os.environ.get("DIFY_API_KEY", "")
GEMINI_API_KEY = os.environ.get("GEMINI_API_KEY", "")
GEMINI_MODEL = os.environ.get("GEMINI_MODEL", "gemini-2.5-flash-lite")
SUPABASE_URL = os.environ.get("SUPABASE_URL", "").rstrip("/")
SUPABASE_SERVICE_ROLE_KEY = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "")
DEVICE_ID = os.environ.get("DEVICE_ID", "kiosk-01")
GROQ_API_KEY = os.environ.get("GROQ_API_KEY", "")
TTS_VOICE = "vi-VN-HoaiMyNeural"

# Initialize Opus Decoder and Encoder (16kHz, mono)
OPUS_SAMPLE_RATE = 16000
OPUS_CHANNELS = 1
OPUS_FRAME_SIZE = 960 # 60ms
OPUS_FRAME_BYTES = OPUS_FRAME_SIZE * 2

try:
    opus_encoder = opuslib_next.Encoder(OPUS_SAMPLE_RATE, OPUS_CHANNELS, opuslib_next.APPLICATION_VOIP)
    opus_decoder = opuslib_next.Decoder(OPUS_SAMPLE_RATE, OPUS_CHANNELS)
    log.info("Opus Encoder/Decoder initialized successfully.")
except Exception as e:
    log.error(f"Failed to initialize Opus components: {e}")
    opus_encoder = None
    opus_decoder = None

def ask_dify(question: str) -> str:
    """Send question to Dify RAG backend"""
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
            },
            timeout=25,
        )
        resp.raise_for_status()
        data = resp.json()
        return data.get("answer", "").strip()
    except Exception as e:
        log.error(f"Error calling Dify: {e}")
        return "Xin lỗi, hệ thống đang gặp trục trặc kết nối."

def classify_category(question: str, answer: str) -> str:
    """Call Gemini to classify question category"""
    CATEGORIES = [
        "Quản trị Cơ sở dữ liệu",
        "Khoa học Máy tính / Lập trình",
        "An toàn thông tin",
        "Kinh tế - Quản trị Kinh doanh",
        "Ngôn ngữ - Xã hội Nhân văn",
        "Học phí - Học bổng",
        "Khác",
    ]
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
        log.warning(f"Error classifying category: {e}")
        return "Khác"

def log_to_supabase(question: str, answer: str, category: str):
    """Log the conversation to Supabase table"""
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
        log.warning(f"Error logging to Supabase: {e}")

async def call_groq_whisper(audio_bytes: bytes) -> str:
    """Call Groq Whisper API for high-quality Vietnamese Speech-to-Text"""
    if not GROQ_API_KEY:
        log.error("GROQ_API_KEY is not configured!")
        return ""
    try:
        # Save PCM bytes into WAV format for Whisper API compatibility
        wav_buffer = io.BytesIO()
        with wave.open(wav_buffer, 'wb') as wav_file:
            wav_file.setnchannels(OPUS_CHANNELS)
            wav_file.setsampwidth(2) # 16-bit
            wav_file.setframerate(OPUS_SAMPLE_RATE)
            wav_file.writeframes(audio_bytes)
        
        wav_data = wav_buffer.getvalue()
        audio_file = io.BytesIO(wav_data)
        audio_file.name = "input.wav"
        
        headers = {
            "Authorization": f"Bearer {GROQ_API_KEY}",
        }
        files = {
            "file": (audio_file.name, audio_file, "audio/wav"),
        }
        data = {
            "model": "whisper-large-v3",
            "language": "vi",
        }
        
        resp = requests.post(
            "https://api.groq.com/openai/v1/audio/transcriptions",
            headers=headers,
            files=files,
            data=data,
            timeout=15
        )
        resp.raise_for_status()
        result = resp.json()
        return result.get("text", "").strip()
    except Exception as e:
        log.error(f"Error calling Groq Whisper: {e}")
        return ""

def encode_pcm_to_opus(pcm_data: bytes) -> list:
    """Slice raw PCM data into 60ms frames and encode each into Opus"""
    if not opus_encoder:
        log.error("Opus Encoder is not available!")
        return []
        
    opus_frames = []
    offset = 0
    while offset < len(pcm_data):
        chunk = pcm_data[offset : offset + OPUS_FRAME_BYTES]
        if len(chunk) < OPUS_FRAME_BYTES:
            chunk = chunk + b"\x00" * (OPUS_FRAME_BYTES - len(chunk))
        
        try:
            encoded_frame = opus_encoder.encode(chunk, OPUS_FRAME_SIZE)
            opus_frames.append(encoded_frame)
        except Exception as e:
            log.error(f"Opus encoding failed for chunk: {e}")
            
        offset += OPUS_FRAME_BYTES
    return opus_frames

async def tts_and_send(websocket, text: str, session_id: str):
    """Generate Vietnamese speech using Edge-TTS, convert to Opus and send over WebSocket"""
    try:
        log.info(f"Generating TTS: {text}")
        communicate = edge_tts.Communicate(text, TTS_VOICE)
        mp3_buffer = io.BytesIO()
        async for chunk in communicate.stream():
            if chunk["data"]:
                mp3_buffer.write(chunk["data"])
        
        mp3_buffer.seek(0)
        
        audio = AudioSegment.from_file(mp3_buffer, format="mp3")
        audio = audio.set_frame_rate(OPUS_SAMPLE_RATE).set_channels(OPUS_CHANNELS).set_sample_width(2)
        raw_pcm = audio.raw_data
        
        opus_frames = encode_pcm_to_opus(raw_pcm)
        
        await websocket.send(json.dumps({
            "type": "tts",
            "session_id": session_id,
            "state": "start"
        }))
        
        await websocket.send(json.dumps({
            "type": "tts",
            "session_id": session_id,
            "state": "sentence_start",
            "text": text
        }))
        
        log.info(f"Streaming {len(opus_frames)} Opus frames to device...")
        for frame in opus_frames:
            await websocket.send(frame)
            await asyncio.sleep(0.06)
            
        await websocket.send(json.dumps({
            "type": "tts",
            "session_id": session_id,
            "state": "stop"
        }))
        
    except Exception as e:
        log.error(f"Error in TTS generation or streaming: {e}")

async def handle_connection(websocket, path):
    """Handle incoming WebSocket connection from Xiaozhi device"""
    log.info(f"New client connected from {websocket.remote_address}")
    session_id = str(uuid.uuid4())
    audio_buffer = bytearray()
    listening = False
    
    try:
        async for message in websocket:
            if isinstance(message, str):
                # Text message (JSON)
                try:
                    data = json.loads(message)
                    msg_type = data.get("type")
                    
                    if msg_type == "hello":
                        # Respond to hello handshake
                        response = {
                            "type": "hello",
                            "transport": "websocket",
                            "session_id": session_id,
                            "audio_params": {
                                "format": "opus",
                                "sample_rate": OPUS_SAMPLE_RATE,
                                "channels": OPUS_CHANNELS,
                                "frame_duration": 60
                            }
                        }
                        await websocket.send(json.dumps(response))
                        log.info(f"Handshake complete. Session ID: {session_id}")
                        
                    elif msg_type == "listen":
                        state = data.get("state")
                        if state == "start":
                            log.info("Device started listening/mic recording...")
                            audio_buffer.clear()
                            listening = True
                        elif state == "stop":
                            log.info("Device stopped listening. Processing audio...")
                            listening = False
                            
                            # Perform speech to text, ask Dify, log, and speak back
                            if len(audio_buffer) > 0:
                                log.info(f"Recorded {len(audio_buffer)} bytes of PCM. Calling Whisper...")
                                # Call Groq Whisper
                                question = await call_groq_whisper(bytes(audio_buffer))
                                log.info(f"Recognized question: {question}")
                                
                                if question:
                                    # Send recognized STT text to screen
                                    await websocket.send(json.dumps({
                                        "type": "stt",
                                        "session_id": session_id,
                                        "text": question
                                    }))
                                    
                                    # Call Dify RAG
                                    answer = ask_dify(question)
                                    log.info(f"Dify answer: {answer}")
                                    
                                    # Classify category & log to Supabase
                                    category = classify_category(question, answer)
                                    log_to_supabase(question, answer, category)
                                    
                                    # TTS response stream
                                    await tts_and_send(websocket, answer, session_id)
                                else:
                                    # Empty question or no voice recognized
                                    await websocket.send(json.dumps({
                                        "type": "tts",
                                        "session_id": session_id,
                                        "state": "start"
                                    }))
                                    await websocket.send(json.dumps({
                                        "type": "tts",
                                        "session_id": session_id,
                                        "state": "sentence_start",
                                        "text": "Mình chưa nghe rõ câu hỏi, bạn hỏi lại giúp mình nhé."
                                    }))
                                    await tts_and_send(websocket, "Mình chưa nghe rõ câu hỏi, bạn hỏi lại giúp mình nhé.", session_id)
                                    
                    elif msg_type == "abort":
                        log.info("Device aborted session.")
                        listening = False
                        audio_buffer.clear()
                        
                except json.JSONDecodeError:
                    log.error(f"Invalid JSON string received: {message}")
                    
            elif isinstance(message, bytes):
                # Binary message (Audio frame)
                if listening:
                    try:
                        # Decode Opus frame to PCM bytes
                        pcm_frame = opus_decoder.decode(message, OPUS_FRAME_SIZE)
                        audio_buffer.extend(pcm_frame)
                    except Exception as e:
                        # Fail-silent for audio glitches
                        pass
                        
    except websockets.exceptions.ConnectionClosed:
        log.info(f"Client disconnected: {websocket.remote_address}")
    except Exception as e:
        log.error(f"Error handling connection: {e}")

async def main():
    port = int(os.environ.get("PORT", 8080))
    log.info(f"Starting Custom Xiaozhi server on port {port}...")
    async with websockets.serve(handle_connection, "0.0.0.0", port):
        await asyncio.Future() # keep-alive

if __name__ == "__main__":
    asyncio.run(main())
