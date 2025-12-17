# 控制系統邏輯，負責從車輛獲取圖像，進行物體檢測，並根據檢測結果控制車輛行駛。
# 資料流程：
# 1. 從車輛獲取圖像。
# 2. 使用訓練好的物體檢測模型檢測圖像中的物體。
# 3. 根據檢測到的物體調整車輛的速度和方向。

import cv2 as cv
import numpy as np
import time
import remote_control as car
import image_data_handler as img_handler
from hubconf import custom as load_custom_model
from constants import *
import torch

# 載入預訓練模型
modelPath = "client/car_AI/weights/best_93.pt"
detection_model = torch.hub.load(
    'ultralytics/yolov5',
    'custom',
    path=modelPath,
)

car_speed = INITIAL_SPEED

# 定義駕駛邏輯，根據檢測到的物體調整車速和方向
def drive(max_priority_obj,last_obj_in_image, obj_size,car_speed):
    """
    根據當前最高優先物件，決定車子的行為 & 更新 car_speed
    回傳：更新後的 car_speed
    """
    if max_priority_obj == STOP_SIGN and last_obj_in_image != STOP_SIGN:
        car.stop()
        time.sleep(3)  # 停3秒
        car.move_forward()
        car_speed = INITIAL_SPEED
    elif max_priority_obj == SPEED_LIMIT_30_SIGN:
        car.speed_limit_30()
        car_speed = LIMIT_30_SPEED
    elif max_priority_obj == SPEED_LIMIT_120_SIGN:
        car.speed_limit_120()
        car_speed = LIMIT_120_SPEED
    else:
        car.move_forward()
    
    return car_speed

# 等待car的server可用
def wait_for_car_server():
    """
    Wait until the server is available.
    """
    while not car.server_available():
        print(f"car uri : {CAR_ESP_32_URI} is not available yet")
        time.sleep(2)
    
def fetch_img_from_car():
    """
    從 ESP32-CAM 抓一張 JPEG → BGR 圖片
    """
    try:
        img = car.capture_img()   # 已是 BGR
        if img is None:
            raise RuntimeError("Failed to decode JPEG from ESP32-CAM")
        
        # === 修正圖片方向 ===
        img = cv.flip(img, 0)     # 上下翻轉
        img = cv.flip(img, 1)     # 左右翻轉
        
        return img

    except Exception as e:
        raise Exception(f"Error fetching image: {e}")

def detect(img, img_counter, save_detection_img = True):
    img = img_handler.roi(img)
    
    # === 前處理 ===
    rgb_img = cv.cvtColor(img, cv.COLOR_BGR2RGB)
    modelResults = detection_model(rgb_img) # 這裡給出YOLO判斷結果
    
    objects_in_img, bboxes, colors, confidence = img_handler.extract_detection_info(modelResults)
    print(f"detection info: {objects_in_img,bboxes,colors,confidence}")
    return objects_in_img

# max_priority_object： 從檢測到的物體列表中返回優先級最高的物體
def get_max_priority_object(detected_objects : list):
    def get_priority_of_object(obj, obj_size):
        if obj == STOP_SIGN and obj_size > 400 :
            return 6
        elif obj == SPEED_LIMIT_30_SIGN and obj_size > 100:
            return 5
        elif obj == SPEED_LIMIT_120_SIGN and obj_size > 100 :
            return 4
        else:
            return -10 #
  
    max_priority = -1
    size = 0
    max_priority_obj = None
    for obj, obj_size in detected_objects:
            obj_priority = get_priority_of_object(obj, obj_size)
            if obj_priority > max_priority:
                max_priority = obj_priority
                size = obj_size
                max_priority_obj = obj
    return max_priority_obj,size

# 主要運作的function
def run():
    wait_for_car_server()
    global car_speed
    last_obj_in_image = "none"
    img_counter = 0
    car_speed = INITIAL_SPEED
    car.set_speed(car_speed)

    while True:
        start_time = time.time()
        try:
            img = fetch_img_from_car()

            # return 偵測的標誌
            objects_in_img = detect(img, img_counter, save_detection_img=True)
            # 設定標誌的優先順序並取得最高優先標誌
            max_priority_obj, obj_size = get_max_priority_object(objects_in_img)
            # 根據最高優先標誌調整車速
            car_speed = drive(max_priority_obj, last_obj_in_image, obj_size, car_speed)

            if max_priority_obj is not None:
                last_obj_in_image = max_priority_obj

            img_counter += 1

        except Exception as e:
            print(f"An unexpected error occurred: {e}\n")

        finally:
            elapsed_time = time.time() - start_time
            print(f"elapsed_time = {elapsed_time:.4f}s")

if __name__ == '__main__':
    run_ind = img_handler.create_run_folder_for_images()
    print(f"Assigned run index: {run_ind}")
    img_handler.update_run_index(run_ind)

    run()