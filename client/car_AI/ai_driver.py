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
from obj_handlers import *
import torch

# 載入預訓練模型
modelPath = "client/car_AI/weights/best_93.pt"
# detection_model = load_custom_model(path=modelPath)
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
    if max_priority_obj == PEDESTRIAN:
        car_speed = pedestrian_handler(obj_size, car_speed)
    elif max_priority_obj == STOP_SIGN and last_obj_in_image != STOP_SIGN:
        car_speed = stop_sign_handler(obj_size, car_speed, sleep_time=3)
    elif max_priority_obj == CROSSWALK_SIGN:
        car_speed =  crosswalk_handler(obj_size, car_speed)
    elif max_priority_obj == KEEP_RIGHT:
        car.turn(direction="right", turn_duration=10)
    elif max_priority_obj == SPEED_LIMIT_50_SIGN:
        car_speed = speed_limit_handler(SPEED_LIMIT_50_SIGN)
    elif max_priority_obj == SPEED_LIMIT_100_SIGN:
        car_speed = speed_limit_handler(SPEED_LIMIT_100_SIGN)
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


def prepare_for_yolo(img):
    # img: BGR uint8
    img = cv.resize(img, (640, 640))        # YOLOv5 default input size
    img = cv.cvtColor(img, cv.COLOR_BGR2RGB)
    img = img.astype(np.float32) / 255.0
    img = np.transpose(img, (2, 0, 1))      # CHW
    img = np.ascontiguousarray(img)
    tensor = torch.from_numpy(img).unsqueeze(0)  # (1,3,640,640)
    return tensor
        
def detect(img, img_counter, save_detection_img = True):
    img = img_handler.roi(img)
    
    # === 前處理 ===
    rgb_img = cv.cvtColor(img, cv.COLOR_BGR2RGB)
    model_start_time = time.time()
    modelResults = detection_model(rgb_img) # 這裡給出YOLO判斷結果
    print(f"model results: {modelResults}")
    
    print("================================")
    df = modelResults.pandas().xyxy[0]
    print(df)
    print("================================")
    objects_in_img, bboxes, colors, confidence = img_handler.extract_detection_info(modelResults)
    print(f"detection info: {objects_in_img,bboxes,colors,confidence}")
    modelElapsedTime = time.time() - model_start_time
    
    if save_detection_img:
        # TODO: new thread to save the detection image
        img_handler.save_detection_img(img,objects_in_img, bboxes, colors, confidence, img_counter, run_ind)
    
    # img_handler.print_results(objects_in_img, confidence, modelElapsedTime)
    return objects_in_img

# max_priority_object： 從檢測到的物體列表中返回優先級最高的物體
def get_max_priority_object(detected_objects : list):
    """
    Returns the object with the highest priority from the list of detected objects.

    Parameters:
    - detected_objects (list): A list of tuples containing detected objects and their areas.

    Returns:
    - str: The object with the maximum priority.
    """
    def get_priority_of_object(obj, obj_size ):
        if obj == PEDESTRIAN and (obj_size > 250):
            return 7
        elif obj == STOP_SIGN and obj_size > 400 :
            return 6
        elif obj == CROSSWALK_SIGN and obj_size > 350 :
            return 5
        elif obj == KEEP_RIGHT and obj_size > 1000:
            return 4
        elif obj == SPEED_LIMIT_50_SIGN and obj_size > 100:
            return 3
        elif obj == SPEED_LIMIT_100_SIGN and obj_size > 100 :
            return 2
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
            # img_handler.save_original_img(img, img_counter, run_ind)  # 如果你需要存原圖

            # return 偵測的標誌
            objects_in_img = detect(img, img_counter, save_detection_img=True)
            max_priority_obj, obj_size = get_max_priority_object(objects_in_img)

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
    # Initialize a new directory to store images (both original and detection) for the current execution run.
    run_ind = img_handler.create_run_folder_for_images()
    print(f"Assigned run index: {run_ind}")
    img_handler.update_run_index(run_ind)

    run()