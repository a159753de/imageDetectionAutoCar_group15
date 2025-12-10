#include "server.h"

// global variables
int carSpeed = 35; 
int carDirection = STOP;

httpd_handle_t index_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

//===========================
// 基本首頁
//===========================
esp_err_t index_handler(httpd_req_t *req) {
    // Content to be displayed at the root URL
    const char *html_response = "<html><body><h1>Welcome to My ESP Web Server</h1></body></html>";
    
    // Set the HTTP response headers
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Add CORS header if needed
    
    // Send the HTML content as the response
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

//==============================
// OV7725 影像傳輸：RAW FRAME CAPTURE
//==============================
esp_err_t capture_handler(httpd_req_t *req) {
    // Serial.println("capture request");
        // 從攝影機取得一張 frame
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // 設定 HTTP 回應格式：原始 RGB565 buffer（二進位資料）
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.raw");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    // 將整張 RGB565 frame 送出去
    httpd_resp_send(req, (const char*)fb->buf, fb->len);

    // 釋放 frame buffer 給 camera driver
    esp_camera_fb_return(fb);
    return ESP_OK;
}

//==============================
// OV7725 專用：RAW STREAM（MJPEG 改成 RAW）
//==============================
static esp_err_t stream_handler(httpd_req_t *req){
   esp_err_t res = ESP_OK;

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while (true) {
        // 獲取一張frame
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        // 傳 frame boundary
        res = httpd_resp_send_chunk(req, "--frame\r\n", strlen("--frame\r\n"));
        if (res != ESP_OK) break;

        // header：RGB565 raw frame
        char header[100];
        int hlen = snprintf(header, sizeof(header),
            "Content-Type: application/octet-stream\r\nContent-Length: %u\r\n\r\n",
            fb->len);
        res = httpd_resp_send_chunk(req, header, hlen);
        if (res != ESP_OK) break;

        // 送 frame buffer
        res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
        if (res != ESP_OK) break;

        // frame end
        res = httpd_resp_send_chunk(req, "\r\n", 2);

        esp_camera_fb_return(fb);
        if (res != ESP_OK) break;
    }

    return res;
}

//======================
// 方向控制 handler
//======================
static esp_err_t forward_handler(httpd_req_t *req) {
  carDirection = FORWARD;
  // Set the HTTP response headers
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Add CORS header if needed

  // Send a response message
  const char *response_message = "Direction changed to forward.";
  httpd_resp_sendstr(req, response_message);
  
  return ESP_OK;
}

static esp_err_t backward_handler(httpd_req_t *req) {
  carDirection = BACKWARD;
  // Set the HTTP response headers
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Add CORS header if needed

  // Send a response message
  const char *response_message = "Direction changed to backward.";
  httpd_resp_sendstr(req, response_message);
  
  return ESP_OK;
}

static esp_err_t left_handler(httpd_req_t *req) {
  carDirection = LEFT;
  // Set the HTTP response headers
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Add CORS header if needed

  // Send a response message
  const char *response_message = "Direction changed to left.";
  httpd_resp_sendstr(req, response_message);
  
  return ESP_OK;
}

static esp_err_t right_handler(httpd_req_t *req) {
  carDirection = RIGHT;
  // Set the HTTP response headers
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Add CORS header if needed

  // Send a response message
  const char *response_message = "Direction changed to right.";
  httpd_resp_sendstr(req, response_message);
  
  return ESP_OK;
}

static esp_err_t stop_handler(httpd_req_t *req) {
  carDirection = STOP;
  // Set the HTTP response headers
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Add CORS header if needed

  // Send a response message
  const char *response_message = "Car stopped successfully";
  httpd_resp_sendstr(req, response_message);
  
  return ESP_OK;
}

//===========================
// 設定車速 Speed Handler
// /set_speed?value=55
//===========================
static esp_err_t set_speed_handler(httpd_req_t *req) {
  
    char buf[64];

    if (httpd_req_get_url_query_len(req) >= sizeof(buf)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // 取得整個 query string
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) != ESP_OK) {
        httpd_resp_send_400(req);
        return ESP_FAIL;
    }

    char value[16];
    if (httpd_query_key_value(buf, "value", value, sizeof(value)) == ESP_OK) {
        int speed = atoi(value);
        if (speed >= 0 && speed <= 100) {
            carSpeed = speed;
            httpd_resp_sendstr(req, "speed updated");
            return ESP_OK;
        }
    }

    httpd_resp_send_400(req);
    return ESP_FAIL;
}

//======================
// 相機參數調整 handler
//======================
static esp_err_t cmd_handler(httpd_req_t *req){
        char* buf;
    size_t buf_len;
    char variable[32] = {0};
    char value[32] = {0};

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len <= 1) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    buf = (char*)malloc(buf_len);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (httpd_req_get_url_query_str(req, buf, buf_len) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) != ESP_OK ||
        httpd_query_key_value(buf, "val", value, sizeof(value)) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    free(buf);

    int val = atoi(value);
    sensor_t *s = esp_camera_sensor_get();
    int res = 0;

    //==========================
    // OV7725 可用參數
    //==========================
    if (!strcmp(variable, "brightness")) {
        res = s->set_brightness(s, val);       // -2 ~ 2
    } 
    else if (!strcmp(variable, "contrast")) {
        res = s->set_contrast(s, val);         // -2 ~ 2
    }
    else if (!strcmp(variable, "saturation")) {
        res = s->set_saturation(s, val);       // -2 ~ 2
    }
    else if (!strcmp(variable, "aec")) {
        res = s->set_exposure_ctrl(s, val);    // auto exposure (0/1)
    }
    else if (!strcmp(variable, "aec_value")) {
        res = s->set_aec_value(s, val);        // 0~1200
    }
    else if (!strcmp(variable, "agc")) {
        res = s->set_gain_ctrl(s, val);        // auto gain
    }
    else if (!strcmp(variable, "hmirror")) {
        res = s->set_hmirror(s, val);          // 水平鏡像
    }
    else if (!strcmp(variable, "vflip")) {
        res = s->set_vflip(s, val);            // 垂直翻轉
    }
    else {
        res = -1;  // unsupported command
    }

    if (res) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

//======================
// 啟動 server
//======================
void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    // ====== API URI 定義 ======
    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t forward_uri = {
        .uri       = "/forward",
        .method    = HTTP_GET,
        .handler   = forward_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t backward_uri = {
        .uri       = "/backward",
        .method    = HTTP_GET,
        .handler   = backward_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t left_uri = {
        .uri       = "/left",
        .method    = HTTP_GET,
        .handler   = left_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t right_uri = {
        .uri       = "/right",
        .method    = HTTP_GET,
        .handler   = right_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t stop_uri = {
        .uri       = "/stop",
        .method    = HTTP_GET,
        .handler   = stop_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t speed_uri = {
        .uri       = "/set_speed",
        .method    = HTTP_GET,
        .handler   = set_speed_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    // ====== 啟動 Web Server ======
    if (httpd_start(&index_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(index_httpd, &index_uri);
        httpd_register_uri_handler(index_httpd, &capture_uri);
        httpd_register_uri_handler(index_httpd, &forward_uri);
        httpd_register_uri_handler(index_httpd, &backward_uri);
        httpd_register_uri_handler(index_httpd, &left_uri);
        httpd_register_uri_handler(index_httpd, &right_uri);
        httpd_register_uri_handler(index_httpd, &stop_uri);
        httpd_register_uri_handler(index_httpd, &speed_uri);
        httpd_register_uri_handler(index_httpd, &cmd_uri);
    }

    // ====== 啟動 RAW Stream Server 在 port 81 ======
    config.server_port = 81;
    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    Serial.printf("Starting stream server on port: %d\n", config.server_port);

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}
