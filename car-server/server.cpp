#include "server.h"

// global variables
int carSpeed = 20; 
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

//===========================
// 單張拍照（JPEG）
//===========================
esp_err_t capture_handler(httpd_req_t *req) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, (const char *)fb->buf, fb->len);

    esp_camera_fb_return(fb);
    return ESP_OK;
}

//===========================
// MJPEG Stream
//===========================
esp_err_t stream_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char buf[64];
    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) break;

        httpd_resp_send_chunk(req, "--frame\r\n", 8);

        size_t len = snprintf(buf, sizeof(buf),
                              "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                              fb->len);
        httpd_resp_send_chunk(req, buf, len);
        httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        httpd_resp_send_chunk(req, "\r\n", 2);

        esp_camera_fb_return(fb);
    }
    return ESP_OK;
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

static esp_err_t speed_limit_30_handler(httpd_req_t *req) {
  carDirection = SPEED_LIMIT_30;
  // Set the HTTP response headers
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Add CORS header if needed

  // Send a response message
  const char *response_message = "Speed set to 30.";
  httpd_resp_sendstr(req, response_message);
  
  return ESP_OK;
}

static esp_err_t speed_limit_120_handler(httpd_req_t *req) {
  carDirection = SPEED_LIMIT_120;
  // Set the HTTP response headers
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Add CORS header if needed

  // Send a response message
  const char *response_message = "Speed set to 120.";
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

//======================
// 啟動 server
//======================
void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    // 把上限改大
    config.max_uri_handlers = 16;
    
    Serial.println("=== SERVER CPP VERSION A LOADED ===");
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
    httpd_uri_t stop_uri = {
        .uri       = "/stop",
        .method    = HTTP_GET,
        .handler   = stop_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t speed_limit_30_uri = {
        .uri       = "/speed/30",
        .method    = HTTP_GET,
        .handler   = speed_limit_30_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t speed_limit_120_uri = {
        .uri       = "/speed/120",
        .method    = HTTP_GET,
        .handler   = speed_limit_120_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    // ====== 啟動 Web Server ======
    if (httpd_start(&index_httpd, &config) == ESP_OK) {

        httpd_register_uri_handler(index_httpd, &index_uri);
        httpd_register_uri_handler(index_httpd, &capture_uri);
        httpd_register_uri_handler(index_httpd, &forward_uri);
        httpd_register_uri_handler(index_httpd, &speed_limit_30_uri);
        httpd_register_uri_handler(index_httpd, &speed_limit_120_uri);
        httpd_register_uri_handler(index_httpd, &stop_uri);


        // ⭐⭐ 重點：直接把 stream 註冊在同一個 server
        httpd_register_uri_handler(index_httpd, &stream_uri);

        Serial.printf("HTTP server started on port 80\n");
    }
}
