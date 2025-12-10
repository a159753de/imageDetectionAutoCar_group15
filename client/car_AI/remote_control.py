import urllib.request
import time
from constants import *
import requests

def set_resolution(resolution_index: int = 1, show_resolution_options: bool = False):
    try:
        if show_resolution_options:
            available_resolutions = {
                10: "UXGA (1600x1200)",
                9: "SXGA (1280x1024)",
                8: "XGA (1024x768)",
                7: "SVGA (800x600)",
                6: "VGA (640x480)",
                5: "CIF (400x296)",
                4: "QVGA (320x240)",
                3: "HQVGA (240x176)",
                0: "QQVGA (160x120)"
            }
            print("Available resolutions:")
            for index, resolution in available_resolutions.items():
                print(f"{index}: {resolution}")
        
        valid_resolution_indices = [10, 9, 8, 7, 6, 5, 4, 3, 0]
        if resolution_index in valid_resolution_indices:
            requests.get(f"{CAR_ESP_32_URI}/control?var=framesize&val={resolution_index}")
            print(f"resulution is {available_resolutions.get(resolution_index)}")
        else:
            print("Invalid resolution index selected.")
    except requests.RequestException:
        print("An error occurred while trying to set the resolution.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

################
# 車輛動作邏輯
################    
def move_forward():
    try:
        urllib.request.urlopen(FORWARD_URI,timeout=2)
    except urllib.error.URLError as e:
        print(f"URLError occurred while sending the 'go forward' command:\n {e}")

def move_backward():
    try:
        urllib.request.urlopen(BACKWARD_URI,timeout=2)
    except urllib.error.URLError as e:
        print(f"URLError occurred while sending the 'go back' command:\n {e}")

def turn(direction: str = "right", turn_duration: float = 0.2):
    """
    direction: "right" or "left"
    turn_duration: 轉彎多久(秒)，轉完會自動 stop()
    """
    try:
        if direction == "right":
            urllib.request.urlopen(RIGHT_URI, timeout=2)
        else:
            urllib.request.urlopen(LEFT_URI, timeout=2)
    except Exception as e:
        print(f"[ERROR] turn({direction}) failed: {e}")
        return

    time.sleep(turn_duration)
    stop()

def stop():
    try:
        urllib.request.urlopen(f"{STOP_URI}", timeout=2)
    except urllib.error.URLError as e:
        print(f"URL error occurred while stopping the car:\n {e}")

################
# 速度控制
################  
def set_speed(speed):
    """
    speed: 0~100
    ESP32 端的 /set_speed handler 是用 ?value=xxx
    """
    if speed < 30 or speed > 100:
        print(f"[WARN] set_speed({speed}) out of range (0~100)")
        return
    
    try:
        urllib.request.urlopen(f"{SET_SPEED_URI}?value={speed}" ,timeout=2)
        print(f"set car speed to {speed} successfully")
    except Exception as e:
     print(f"An unexpected error occurred while setting the speed to {speed} {e}\n")

#################
# 影像擷取 RAW565
#################
def capture_img():
    """
    從 ESP32 的 /capture 取得一張影像 (RGB565 Raw buffer)
    回傳: bytes（長度 = width * height * 2）
    """
    try:
        resp = urllib.request.urlopen(CAPTURE_URI,timeout=5)
        data = resp.read()
        return data
    except Exception as e:
        print(f"[ERROR] capture_img failed: {e}")
        return b""


# 檢查esp32 server是否可用
def server_available():
    """
    簡單 ping 一下 ESP32 是否可連線
    """
    try:
        urllib.request.urlopen(CAR_ESP_32_URI,timeout=3)
        return True
    except Exception as e:
        return False