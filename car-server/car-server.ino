#include "server.h"

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "esp_http_server.h"

// WIFI setting
const char* ssid = "HCILab_2.4G";
const char* password = "hcilab@307";

// OV7725
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     6
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y2_GPIO_NUM 20   // D0
#define Y3_GPIO_NUM 21   // D1
#define Y4_GPIO_NUM 11   // D2
#define Y5_GPIO_NUM 12   // D3
#define Y6_GPIO_NUM 16   // D4
#define Y7_GPIO_NUM 17   // D5
#define Y8_GPIO_NUM 18   // D6
#define Y9_GPIO_NUM 19   // D7

#define VSYNC_GPIO_NUM 35
#define HREF_GPIO_NUM  36
#define PCLK_GPIO_NUM  37

// 馬達腳位
#define PIN_IN1  7 
#define PIN_IN2  8 
#define PIN_ENA  14 

#define PIN_IN3  9 
#define PIN_IN4  10 
#define PIN_ENB  15


void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
 
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);

  // 初始化，輪胎不動
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW);
  digitalWrite(PIN_IN4, LOW);

  // OV7725 參數
  camera_config_t config;
  memset(&config, 0, sizeof(config));

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;   // 320×240 → 無 PSRAM 最穩
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_DRAM;
  
  // OV7725 初始化
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 1);   // 稍微變亮
    s->set_contrast(s, 2);     // 增加對比
    s->set_saturation(s, -1);  // OV7725 顏色偏重，降低會更自然
    s->set_whitebal(s, 1);     // 開啟自動白平衡
    s->set_gainceiling(s, GAINCEILING_32X);
    s->set_awb_gain(s, 1);
    s->set_aec2(s, 1);         // improve exposure
    s->set_raw_gma(s, 1);      // gamma correction
  }
  
  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.print("Camera Capture Ready! Go to: http://");
  Serial.print(WiFi.localIP());

  // Start streaming web server
  startCameraServer();
}

#define PIN_IN1  7 
#define PIN_IN2  8 
#define PIN_ENA  14 

#define PIN_IN3  9 
#define PIN_IN4  10 
#define PIN_ENB  15

void WheelAct(int IN1, int IN2, int ENA, int IN3, int IN4, int ENB){
  digitalWrite(PIN_IN1, IN1);
  digitalWrite(PIN_IN2, IN2);
  analogWrite(PIN_ENA, ENA);
  digitalWrite(PIN_IN3, IN3);
  digitalWrite(PIN_IN4, IN4);
  analogWrite(PIN_ENB, ENB);
}

const int cycle = 10;

void loop() {
  int parton = cycle * carSpeed/100;
  int lastSpeed = 0;
  // Serial.println("speed = " + String(carSpeed) + " direction is " + String(carDirection));
  switch (carDirection) {
    case FORWARD:
      WheelAct(LOW ,HIGH , 200, HIGH, LOW, 200);
      break;

    // case BACKWARD:
    //   WheelAct(LOW, HIGH, 30, LOW, HIGH, 30);
    //   break;
    
    // case LEFT:
    //   lastSpeed = carSpeed;
    //   carSpeed = TURN_SPEED;
    //   WheelAct(HIGH, LOW, HIGH, HIGH);
    //   carSpeed = lastSpeed;
    //   break;
      
    // case RIGHT:
    //   lastSpeed = carSpeed;
    //   carSpeed = TURN_SPEED;
    //   WheelAct(HIGH, HIGH, HIGH, LOW);
    //   carSpeed = lastSpeed;
    //   // Right turn logic
    //   break;
      
    default:
      // Stop the car
      WheelAct(LOW, LOW, 0, LOW, LOW, 0);
      break;
  }
  delay(parton);
  WheelAct(LOW, LOW, 0, LOW, LOW, 0);
  delay(cycle - parton);
}
