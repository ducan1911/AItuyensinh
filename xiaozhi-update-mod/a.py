import os
import time
import serial.tools.list_ports
import subprocess

# Thêm vào:
from plyer import notification
import winsound  # Chỉ hỗ trợ Windows

FLASH_BIN_PATH = "xiaozhi-mini-2.0.5.bin"  # File .bin cần nạp
BAUD_RATE = "921600"
CHIP_TYPE = "esp32s3"

def find_esp32_port(previous_ports):
    while True:
        ports = list(serial.tools.list_ports.comports())
        new_ports = [port.device for port in ports if port.device not in previous_ports]
        if new_ports:
            return new_ports[0]
        time.sleep(1)

def notify_success(port):

    # Hiển thị thông báo
    notification.notify(
        title="✅ Nạp Firmware Thành Công",
        message=f"Thiết bị trên cổng {port} đã được nạp xong!",
        timeout=5
    )

def flash_firmware(port):
    print(f"\n🔌 Phát hiện thiết bị tại {port}, bắt đầu nạp firmware...")

    if not os.path.exists(FLASH_BIN_PATH):
        print(f"❌ Không tìm thấy file: {FLASH_BIN_PATH}")
        return

    try:
        process = subprocess.Popen(
            ["python", "-m", "esptool",
             "--chip", CHIP_TYPE,
             "--port", port,
             "--baud", BAUD_RATE,
             "write-flash", "0x0", FLASH_BIN_PATH],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True
        )

        for line in process.stdout:
            print(line.strip())

        process.wait()
        if process.returncode == 0:
            print(f"✅ Nạp thành công trên {port}!\n")
            notify_success(port)
        else:
            print(f"❌ Nạp thất bại trên {port}. Mã lỗi: {process.returncode}\n")

    except Exception as e:
        print(f"❌ Lỗi trong quá trình nạp: {e}\n")

def main():
    print("🚀 Tool nạp ESP32-C3 tự động bắt đầu. Đang chờ thiết bị...\n")
    previous_ports = [port.device for port in serial.tools.list_ports.comports()]

    while True:
        current_port = find_esp32_port(previous_ports)
        flash_firmware(current_port)

        print("⏳ Chờ bạn rút thiết bị ra...")
        while current_port in [port.device for port in serial.tools.list_ports.comports()]:
            time.sleep(1)

        print("🔁 Thiết bị đã được rút. Chờ thiết bị tiếp theo...\n")
        previous_ports = [port.device for port in serial.tools.list_ports.comports()]

if __name__ == "__main__":
    main()
