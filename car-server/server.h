// 標頭檔，實際邏輯寫在 server.cpp
#ifndef SERVER_H
#define SERVER_H

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include <string.h>
#include "fb_gfx.h"
#include "soc/soc.h"         // disable brownout problems
#include "soc/rtc_cntl_reg.h"// disable brownout problems

// ─────────────────────────────
// 車子方向常數
// ─────────────────────────────
#define FORWARD           1
#define BACKWARD          2
#define LEFT              3
#define RIGHT             4
#define STOP              5
#define SPEED_LIMIT_30    6
#define SPEED_LIMIT_120   7
#define TURN_SPEED        55

// 全域車速 / 方向變數（在 server.cpp 定義）
extern int carSpeed;
extern int carDirection;

// 對外只需要這一個入口函式
void startCameraServer();

#endif // SERVER_H
