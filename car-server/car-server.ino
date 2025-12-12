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

// 改成 OV2640 / ESP32-CAM 風格的腳位
// ★ 這裡請依你的實際接線修改 ★
// 以下只是範例（假設類似 ESP32-CAM / ESP32-S3-EYE）
// =====================
#define CAMERA_MODEL_AI_THINKER
#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#endif

// 馬達腳位
#define PIN_IN1   2   // 左馬達方向 1
#define PIN_IN2  14   // 左馬達方向 2
#define PIN_ENA  4   // 左馬達 PWM

#define PIN_IN3  15   // 右馬達方向 1
#define PIN_IN4  12   // 右馬達方向 2
#define PIN_ENB  13    // 右馬達 PWM


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

  // =======================
  // OV2640 / ESP32-CAM 參數
  // =======================
  camera_config_t config;
  memset(&config, 0, sizeof(config));

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk       = XCLK_GPIO_NUM;
  config.pin_pclk       = PCLK_GPIO_NUM;
  config.pin_vsync      = VSYNC_GPIO_NUM;
  config.pin_href       = HREF_GPIO_NUM;
  config.pin_sscb_sda   = SIOD_GPIO_NUM;
  config.pin_sscb_scl   = SIOC_GPIO_NUM;
  config.pin_pwdn       = PWDN_GPIO_NUM;
  config.pin_reset      = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;   // ★ 改成 JPEG

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_VGA;  // 640x480
    config.jpeg_quality = 12;             // 0-63，數字越小畫質越好
    config.fb_count     = 2;
  } else {
    config.frame_size   = FRAMESIZE_QVGA; // 320x240
    config.jpeg_quality = 12;
    config.fb_count     = 1;
  }

  // 相機初始化
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  // ⭐⭐ 在這裡加入，位置要正確 ⭐⭐
  sensor_t * s = esp_camera_sensor_get();
  s->set_hmirror(s, 1);

  
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
      WheelAct(HIGH , LOW , 209, HIGH, LOW, 185);
      break;
    case SPEED_LIMIT_30:
      WheelAct(HIGH , LOW , 145, HIGH, LOW, 125);
      break;
    case SPEED_LIMIT_120:
      WheelAct(HIGH , LOW , 255, HIGH, LOW, 247);
      break;
    default:
      // Stop the car
      WheelAct(LOW, LOW, 0, LOW, LOW, 0);
      break;
  }
  delay(parton);
  WheelAct(LOW, LOW, 0, LOW, LOW, 0);
  delay(cycle - parton);
}
