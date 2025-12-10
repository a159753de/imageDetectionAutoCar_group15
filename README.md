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
4. 啟動前端與偵測程式
```
py .\client\main.py
```
或是分別啟動也可以
```

```
如果執行後找不到套件，就再用pip單獨安裝套件
# 系統邏輯
## car_server 
負責ESP32、相機的控制，並處理：
1. 將讀取到的圖片傳給client
2. 啟動server，接收http request後執行對應的行為
### 實際運作流程
setup()：

初始化 Serial

設定腳位（馬達 / LED）

配好 camera_config_t

esp_camera_init(&config);

連 Wi-Fi

startCameraServer();

loop()：

看 carDirection / carSpeed 控制馬達

從電腦 / 筆電端：

抓單張圖：http://<ESP32_IP>/capture → RAW RGB565

串流：http://<ESP32_IP>:81/stream → multipart raw

控車：
/forward

/backward

/left

/right

/stop

設定速度：/set_speed?value=50

調相機參數：/control?var=brightness&val=1

## client
### car_AI
負責主要的邏輯判斷，接收image後根據偵測到的標示來判斷需要做的行為，如暫停、減速等，再將判斷後的結果透過HTTP回傳ESP32
### web_app
啟動web server，可實時觀看偵測結果
