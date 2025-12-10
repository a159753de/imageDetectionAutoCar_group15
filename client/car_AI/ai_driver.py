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

# 載入預訓練模型
modelPath = "client/car_AI/weights/best_93.pt"
detection_model = load_custom_model(path=modelPath)

car_speed = INITIAL_SPEED

# RAW565 → RGB888 轉換
def raw565_to_rgb888(raw: bytes, width: int = 160, height: int = 120) -> np.ndarray:
    """
    將 OV7725 的 RGB565 Raw buffer 轉成 OpenCV 可用的 BGR 圖片
    """
    arr = np.frombuffer(raw, dtype=np.uint8).reshape((height, width, 2))

    r = ((arr >> 11) & 0x1F) << 3
    g = ((arr >> 5) & 0x3F) << 2
    b = (arr & 0x1F) << 3

    # OpenCV 用 BGR
    bgr = np.stack([b, g, r], axis=-1).astype(np.uint8)
    return bgr

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
    從車子抓一張 RAW565 影像，轉成 BGR 圖片
    """
    fetch_start_time = time.time()

    try:
        # 1. 從車抓 raw bytes
        capture_start_time = time.time()
        img_data = car.capture_img()   # 已經是 bytes，不用再 .read()
        capture_time = time.time() - capture_start_time
        print(f"{ANSI_COLOR_GREEN}Time to capture image: {capture_time:.4f} seconds{ANSI_COLOR_RESET}")

        if not img_data:
            raise RuntimeError("Empty image data from car")

        print(f"{ANSI_COLOR_GREEN}data size: {len(img_data)} bytes {ANSI_COLOR_RESET}")

        # 2. RAW565 → BGR 圖片
        decode_start = time.time()
        print("====================================")
        img = raw565_to_rgb888(img_data, width=320, height=240)
        decode_time = time.time() - decode_start
        print(f"{ANSI_COLOR_GREEN}Time to decode RAW565: {decode_time:.4f} seconds{ANSI_COLOR_RESET}")

        # 3. 如果你真的需要旋轉再打開（先關掉比較容易 debug）
        # img = cv.rotate(img, cv.ROTATE_90_CLOCKWISE)

        total_time = time.time() - fetch_start_time
        print(f"\n{ANSI_COLOR_GREEN}Total time for image fetching and processing: {total_time:.4f} seconds{ANSI_COLOR_RESET}\n")

        return img

    except Exception as e:
        raise Exception(f"{ANSI_COLOR_RED}Error fetching or processing image: {e}{ANSI_COLOR_RESET}")
        
def detect(img, img_counter, save_detection_img = True):
    img = img_handler.roi(img)
    rgb_img = cv.cvtColor(img, cv.COLOR_BGR2RGB)
    # Run the object detection model
    model_start_time = time.time()
    modelResults = detection_model(rgb_img) 
    objects_in_img, bboxes, colors, confidence = img_handler.extract_detection_info(modelResults)
    # print(f"detection info: {objects_in_img,bboxes,colors,confidence}")
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


    
