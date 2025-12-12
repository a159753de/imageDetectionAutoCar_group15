import os
# Data
OUTPUT_DETECTION_IMAGES_DIR = os.path.join(os.getcwd(), "data", "images", "detection")
OUTPUT_ORIGINAL_IMAGES_DIR = os.path.join(os.getcwd(), "data", "images","original")
RUN_INDEX_PATH_FILE = os.path.join(os.getcwd(), "data", "last_run_index.txt")
OUTPUT_TEST_PATH = os.path.join(os.getcwd(), "data", "test", "output")
TEST_IMAGES_PATH = os.path.join(os.getcwd(), "data", "test", "images")
TEST_LABELS_PATH = os.path.join(os.getcwd(), "data", "test", "labels")

# Car control URIs
CAR_ESP_32_URI = "http://192.168.0.114"
PORT = 80

# URI設定
FORWARD_URI = f"{CAR_ESP_32_URI}:{PORT}/forward"
CAPTURE_URI = f"{CAR_ESP_32_URI}:{PORT}/capture"
STOP_URI = f"{CAR_ESP_32_URI}:{PORT}/stop"
STREAM_URI = f"{CAR_ESP_32_URI}:{PORT}/stream"
SPEED_LIMIT_30_URI = f"{CAR_ESP_32_URI}:{PORT}/speed/30"
SPEED_LIMIT_120_URI = f"{CAR_ESP_32_URI}:{PORT}/speed/30"

# 車輛速度設定
INITIAL_SPEED = 35
LIMIT_120_SPEED = 120
LIMIT_30_SPEED = 30

# Object names
STOP_SIGN = 'Stop'
SPEED_LIMIT_30_SIGN = '30'
SPEED_LIMIT_120_SIGN = '120'



# ANSI Colors
ANSI_COLOR_RED = '\033[91m'
ANSI_COLOR_GREEN = '\033[92m'
ANSI_COLOR_BLUE = '\033[94m'
ANSI_COLOR_RESET = '\033[0m' 


# List of All Objects with attributes
ALL_OBJECTS = [     {"name" : STOP_SIGN       ,  "threshold" : 0.75   ,   "color" : (0, 0, 255) },
                    {"name" : SPEED_LIMIT_30_SIGN  ,  "threshold" : 0.84    ,  "color" : (0, 255, 0) },
                    {"name" : SPEED_LIMIT_120_SIGN ,  "threshold" : 0.83    ,  "color" : (50, 100, 60) }   ]

# objects area in meters
OBJECTS_AREA = {SPEED_LIMIT_30_SIGN: 0.36, 
                SPEED_LIMIT_120_SIGN: 0.49,
                STOP_SIGN:0.33
                }