#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "img_converters.h"

// Wi-Fi settings copied from the working ESP32-CAM project.
const char *WIFI_SSID = "abc";
const char *WIFI_PASSWORD = "fsx20060809";

// Common ESP32-S3-CAM / ESP32-S3-EYE camera pinout.
constexpr int CAM_PIN_PWDN = -1;
constexpr int CAM_PIN_RESET = -1;
constexpr int CAM_PIN_XCLK = 15;
constexpr int CAM_PIN_SIOD = 4;
constexpr int CAM_PIN_SIOC = 5;
constexpr int CAM_PIN_D0 = 11;
constexpr int CAM_PIN_D1 = 9;
constexpr int CAM_PIN_D2 = 8;
constexpr int CAM_PIN_D3 = 10;
constexpr int CAM_PIN_D4 = 12;
constexpr int CAM_PIN_D5 = 18;
constexpr int CAM_PIN_D6 = 17;
constexpr int CAM_PIN_D7 = 16;
constexpr int CAM_PIN_VSYNC = 6;
constexpr int CAM_PIN_HREF = 7;
constexpr int CAM_PIN_PCLK = 13;

constexpr char STREAM_BOUNDARY[] = "123456789000000000000987654321";
constexpr char STREAM_CONTENT_TYPE[] =
    "multipart/x-mixed-replace;boundary=123456789000000000000987654321";
constexpr char STREAM_SEPARATOR[] =
    "\r\n--123456789000000000000987654321\r\n";
constexpr char STREAM_HEADER[] =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_server = nullptr;
httpd_handle_t stream_server = nullptr;

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32-S3-CAM Test</title>
  <style>
    html,body{margin:0;background:#111;color:#eee;font-family:Arial,sans-serif}
    main{max-width:900px;margin:0 auto;padding:16px}
    h1{font-size:22px;margin:0 0 12px}
    .status{color:#8ddf9b;margin-bottom:12px}
    img{display:block;width:100%;height:auto;background:#000;border:1px solid #444}
    nav{display:flex;gap:10px;margin-top:12px}
    a{color:#fff;background:#c43d35;padding:9px 14px;text-decoration:none;border-radius:4px}
  </style>
</head>
<body>
  <main>
    <h1>ESP32-S3-CAM camera test</h1>
    <div class="status">QVGA 320 x 240, stream port 81</div>
    <img id="camera" alt="Camera stream">
    <nav>
      <a id="start" href="#">Start stream</a>
      <a href="/capture" target="_blank">Capture</a>
      <a href="/status" target="_blank">Status</a>
    </nav>
  </main>
  <script>
    const image = document.getElementById('camera');
    document.getElementById('start').addEventListener('click', event => {
      event.preventDefault();
      image.src = `${location.protocol}//${location.hostname}:81/stream`;
    });
  </script>
</body>
</html>
)HTML";

esp_err_t index_handler(httpd_req_t *request) {
  httpd_resp_set_type(request, "text/html; charset=utf-8");
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");
  return httpd_resp_send(request, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t status_handler(httpd_req_t *request) {
  char response[192];
  sensor_t *sensor = esp_camera_sensor_get();
  const int length = snprintf(
      response,
      sizeof(response),
      "{\"camera\":%s,\"psram\":%s,\"psram_bytes\":%u,\"framesize\":%d}",
      sensor ? "true" : "false",
      psramFound() ? "true" : "false",
      static_cast<unsigned>(ESP.getPsramSize()),
      sensor ? sensor->status.framesize : -1);
  httpd_resp_set_type(request, "application/json");
  httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(request, response, length);
}

esp_err_t capture_handler(httpd_req_t *request) {
  camera_fb_t *frame = esp_camera_fb_get();
  if (frame == nullptr) {
    Serial.println("ERROR: /capture could not get a frame");
    return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "Camera capture failed");
  }

  uint8_t *jpeg_buffer = nullptr;
  size_t jpeg_length = 0;
  const bool converted = frame->format != PIXFORMAT_JPEG;
  if (converted && !frame2jpg(frame, 80, &jpeg_buffer, &jpeg_length)) {
    Serial.println("ERROR: /capture JPEG conversion failed");
    esp_camera_fb_return(frame);
    return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "JPEG conversion failed");
  }
  if (!converted) {
    jpeg_buffer = frame->buf;
    jpeg_length = frame->len;
  }

  httpd_resp_set_type(request, "image/jpeg");
  httpd_resp_set_hdr(request, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
  const esp_err_t result = httpd_resp_send(
      request, reinterpret_cast<const char *>(jpeg_buffer), jpeg_length);
  esp_camera_fb_return(frame);
  if (converted) {
    free(jpeg_buffer);
  }
  return result;
}

esp_err_t stream_handler(httpd_req_t *request) {
  esp_err_t result = httpd_resp_set_type(request, STREAM_CONTENT_TYPE);
  if (result != ESP_OK) {
    return result;
  }
  httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");

  char header[80];
  while (true) {
    camera_fb_t *frame = esp_camera_fb_get();
    if (frame == nullptr) {
      Serial.println("ERROR: /stream could not get a frame");
      return ESP_FAIL;
    }

    uint8_t *jpeg_buffer = nullptr;
    size_t jpeg_length = 0;
    const bool converted = frame->format != PIXFORMAT_JPEG;
    if (converted && !frame2jpg(frame, 80, &jpeg_buffer, &jpeg_length)) {
      Serial.println("ERROR: /stream JPEG conversion failed");
      esp_camera_fb_return(frame);
      return ESP_FAIL;
    }
    if (!converted) {
      jpeg_buffer = frame->buf;
      jpeg_length = frame->len;
    }

    result = httpd_resp_send_chunk(request, STREAM_SEPARATOR,
                                   strlen(STREAM_SEPARATOR));
    if (result == ESP_OK) {
      const int header_length = snprintf(header, sizeof(header), STREAM_HEADER,
                                         static_cast<unsigned>(jpeg_length));
      result = httpd_resp_send_chunk(request, header, header_length);
    }
    if (result == ESP_OK) {
      result = httpd_resp_send_chunk(
          request, reinterpret_cast<const char *>(jpeg_buffer), jpeg_length);
    }

    esp_camera_fb_return(frame);
    if (converted) {
      free(jpeg_buffer);
    }
    if (result != ESP_OK) {
      Serial.printf("Stream client disconnected: 0x%x\n", result);
      return result;
    }
    delay(10);
  }
}

bool init_camera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_PIN_D0;
  config.pin_d1 = CAM_PIN_D1;
  config.pin_d2 = CAM_PIN_D2;
  config.pin_d3 = CAM_PIN_D3;
  config.pin_d4 = CAM_PIN_D4;
  config.pin_d5 = CAM_PIN_D5;
  config.pin_d6 = CAM_PIN_D6;
  config.pin_d7 = CAM_PIN_D7;
  config.pin_xclk = CAM_PIN_XCLK;
  config.pin_pclk = CAM_PIN_PCLK;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href = CAM_PIN_HREF;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;
  config.pin_pwdn = CAM_PIN_PWDN;
  config.pin_reset = CAM_PIN_RESET;
  config.xclk_freq_hz = 20000000;
  // The camera fitted to this N4R2 board reports no hardware JPEG support.
  // Capture RGB565 and convert frames to JPEG in the HTTP handlers.
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  Serial.printf("PSRAM: %s, size: %u bytes\n",
                psramFound() ? "OK" : "NOT FOUND",
                static_cast<unsigned>(ESP.getPsramSize()));
  Serial.println("Camera pins: XCLK=15 SDA=4 SCL=5 VSYNC=6 HREF=7 PCLK=13");

  const esp_err_t error = esp_camera_init(&config);
  if (error != ESP_OK) {
    Serial.printf("ERROR: esp_camera_init failed: 0x%x\n", error);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    Serial.printf("Camera sensor PID: 0x%02x (RGB565 capture)\n", sensor->id.PID);
  }

  for (int attempt = 1; attempt <= 5; ++attempt) {
    camera_fb_t *frame = esp_camera_fb_get();
    if (frame != nullptr) {
      Serial.printf("Camera test frame OK: %ux%u, %u bytes\n",
                    frame->width, frame->height,
                    static_cast<unsigned>(frame->len));
      esp_camera_fb_return(frame);
      return true;
    }
    Serial.printf("Camera test frame %d/5 failed\n", attempt);
    delay(200);
  }

  Serial.println("ERROR: camera initialized but produced no frames");
  return false;
}

void start_web_servers() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 8;

  const httpd_uri_t index_uri = {
      .uri = "/", .method = HTTP_GET, .handler = index_handler, .user_ctx = nullptr};
  const httpd_uri_t capture_uri = {
      .uri = "/capture", .method = HTTP_GET, .handler = capture_handler, .user_ctx = nullptr};
  const httpd_uri_t status_uri = {
      .uri = "/status", .method = HTTP_GET, .handler = status_handler, .user_ctx = nullptr};
  const httpd_uri_t stream_uri = {
      .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = nullptr};

  if (httpd_start(&camera_server, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_server, &index_uri);
    httpd_register_uri_handler(camera_server, &capture_uri);
    httpd_register_uri_handler(camera_server, &status_uri);
  } else {
    Serial.println("ERROR: failed to start HTTP server on port 80");
  }

  config.server_port = 81;
  config.ctrl_port = 32769;
  if (httpd_start(&stream_server, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_server, &stream_uri);
  } else {
    Serial.println("ERROR: failed to start stream server on port 81");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("ESP32-S3-CAM N4R2 diagnostic firmware");
  Serial.printf("Flash: %u bytes\n", static_cast<unsigned>(ESP.getFlashChipSize()));

  if (!init_camera()) {
    Serial.println("Camera startup failed; web server will not start.");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to Wi-Fi: %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("Camera ready: http://");
  Serial.println(WiFi.localIP());
  start_web_servers();
}

void loop() {
  delay(10000);
}
