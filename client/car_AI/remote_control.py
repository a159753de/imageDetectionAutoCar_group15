import urllib.request
import cv2
import numpy as np
from constants import *

################
# 車輛動作邏輯
################    
def move_forward():
    try:
        urllib.request.urlopen(FORWARD_URI,timeout=2)
    except urllib.error.URLError as e:
        print(f"URLError occurred while sending the 'go forward' command:\n {e}")

def stop():
    try:
        urllib.request.urlopen(f"{STOP_URI}", timeout=2)
    except urllib.error.URLError as e:
        print(f"URL error occurred while stopping the car:\n {e}")

def speed_limit_30():
    try:
        urllib.request.urlopen(SPEED_LIMIT_30_URI, timeout=2)
    except urllib.error.URLError as e:
        print(f"URL error occurred while setting speed limit to 30:\n {e}")

def speed_limit_120():
    try:
        urllib.request.urlopen(SPEED_LIMIT_120_URI, timeout=2)
    except urllib.error.URLError as e:
        print(f"URL error occurred while setting speed limit to 120:\n {e}")

#################
# 影像擷取
#################
def capture_img():
    """
    從 ESP32 的 /capture 取得一張 JPEG 影像
    回傳: numpy array (BGR)
    """
    try:
        resp = urllib.request.urlopen(CAPTURE_URI, timeout=5)
        jpg = resp.read()

        # JPEG → numpy array
        img_array = np.frombuffer(jpg, dtype=np.uint8)
        img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

        return img

    except Exception as e:
        print(f"[ERROR] capture_img failed: {e}")
        return None


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