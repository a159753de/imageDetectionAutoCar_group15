# 系統需求
## 系統運行
先在專案資料夾建立python虛擬環境venv

<code>
  py -3.10 -m venv venv
</code>

啟動venv

<code>
  .\venv\Scripts\activate
<code>
安裝所需套件
  pip install -r .\client\car_AI\requirements.txt
# 系統邏輯
## car_server 
負責ESP32、相機的控制，並處理：
1. 將讀取到的圖片傳給client
2. 啟動server，接收http request後執行對應的行為
## client
### car_AI
負責主要的邏輯判斷，接收image後根據偵測到的標示來判斷需要做的行為，如暫停、減速等，再將判斷後的結果透過HTTP回傳ESP32
### web_app
啟動web server，可實時觀看偵測結果
