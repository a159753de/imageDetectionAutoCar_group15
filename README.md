# 系統需求
1. python3.10(超過無法使用YOLOv5)
# 系統運行
1. 先在專案資料夾建立python虛擬環境venv
```
py -3.10 -m venv venv
```
2. 啟動venv
```
.\venv\Scripts\activate
```
3. 安裝所需套件
```
pip install -r .\client\car_AI\requirements.txt
```
4. 啟動畫面前端與偵測程式
```
py .\client\main.py
```
或是分別啟動也可以
```
// 啟動畫面前端
py  .\client\web_app\app.py
// 啟動偵測程式
py .\client\car_AI\ai_driver.py
```
如果執行後找不到套件，就再用pip單獨安裝套件
# 系統邏輯介紹
## car_server 
* 負責ESP32、相機的控制，並處理：
  1. 將讀取到的圖片傳給client
  2. 啟動server，接收http request後執行對應的行為
### 實際運作流程

* setup()：

  初始化 Serial

  設定腳位（馬達 / LED）

  配好 camera_config_t

  esp_camera_init(&config);

  連 Wi-Fi

  startCameraServer();


* loop()：

  看 carDirection / carSpeed 控制馬達

  從電腦 / 筆電端：

    抓單張圖：http://<ESP32_IP>/capture → RAW RGB565

    串流：http://<ESP32_IP>/stream → multipart raw

* 控車：
  
    /forward

    /backward

    /left

    /right

    /stop

* 設定速度：/set_speed?value=50

* 調相機參數：/control?var=brightness&val=1

## client
### car_AI
* 負責主要的邏輯判斷，接收image後根據偵測到的標示來判斷需要做的行為，如暫停、減速等，再將判斷後的結果透過HTTP回傳ESP32

### 實際運作流程

ai_driver.py — 主流程（Main Loop）
---

* 用途：

  控制迴圈：抓圖 → 偵測 → 決策 → 控制車

  呼叫 remote_control.py 去控制 ESP32S3

  呼叫 image_data_handler.py 處理 ROI / 畫框等

  呼叫 obj_handlers.py 決定遇到路牌的行為

  呼叫模型 detect()

* 輸入：車子傳回的影像

* 輸出：對車子的控制（HTTP）

remote_control.py — 車子控制（HTTP API）
---

* 用途：

  封裝 ESP32S3 API（forward / backward / stop / capture / set_speed）

  使用 urllib or requests 呼叫你的 /forward /capture /stop 等 URI

  透過 CAPTURE_URI 取得影像

image_data_handler.py — 圖像處理
---

* 用途：

  ROI 擷取

  YOLO 偵測後畫框 / 標記

  儲存圖片

obj_handlers.py — 路牌行為邏輯
---

* 用途：

  停止、減速、變速、轉彎的邏輯

  呼叫 remote_control.py 內的控制函式

constants.py — 存放 URI 與常用常數
---

* 用途：

  定義 CAR_ESP_32_URI、FORWARD_URI、CAPTURE_URI 等全部硬連 URI

  供 remote_control.py / ai_driver.py 使用

### web_app動web server，可實時觀看偵測結果
