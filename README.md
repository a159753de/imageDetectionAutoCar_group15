# 系統需求
1. python3.10(超過無法使用YOLOv5)
# 系統運行
1. 先在專案資料夾建立python虛擬環境venv
```
py -3.10 -m venv {venv名字}
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

  看 carDirection 控制馬達

* 控車：
  
    /forward

    /stop

    /speed/30

    /speed/120

## client
### car_AI
* 負責主要的邏輯判斷，接收image後根據偵測到的標示來判斷需要做的行為，如暫停、減速等，再將判斷後的結果透過HTTP回傳ESP32

### 實際運作流程

ai_driver.py — 主流程（Main Loop）
---

* 用途：

  控制迴圈：抓圖 → 偵測 → 決策 → 控制車

  remote_control.py 發送 HTTP

  呼叫 image_data_handler.py 處理 ROI / 畫框等

  呼叫模型 detect()

* 輸入：車子傳回的影像

* 輸出：對車子的控制（HTTP）

remote_control.py — 車子控制（HTTP API）
---

* 用途：

  控制 ESP32-CAM API（forward / stop / capture / speed）

  使用 urllib or requests 呼叫 URI

  透過 CAPTURE_URI 取得影像

image_data_handler.py — 圖像處理
---

* 用途：

  ROI 擷取

  YOLO 偵測後畫框 / 標記

  儲存圖片


constants.py — 存放 URI 與常用常數
---

* 用途：

  定義 CAR_ESP_32_URI、FORWARD_URI、CAPTURE_URI 等 URI

  供 remote_control.py / ai_driver.py 使用

### web_app動web server，可實時觀看偵測結果
