#include "esp_camera.h"
#include <WiFi.h>

// ===================
// 1. 选择摄像头型号
// ===================
// 你的板子是 AI Thinker (安信可) 版本，请取消这一行的注释
#define CAMERA_MODEL_AI_THINKER 

// 这里的其他型号都注释掉
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
//#define CAMERA_MODEL_ESP32_CAM_BOARD

#include "camera_pins.h"

// ===========================
// 2. 填写你的 WiFi 账号密码
// ===========================
const char* ssid = "abc";
const char* password = "fsx20060809";

void startCameraServer();
void setupLedFlash(int pin);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config = {};
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  // ============================================
  // 3. 核心优化配置 (解决卡顿的关键！)
  // ============================================
  
  // 默认使用 QVGA (320x240) 分辨率，确保流畅度
  // 原版是 UXGA (1600x1200)，那肯定会卡的
  config.frame_size = FRAMESIZE_QVGA; 
  
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.jpeg_quality = 12; // 质量适中
  config.fb_count = 1;

  const bool hasPsram = psramFound();
  Serial.printf("PSRAM: %s, size: %u bytes\n",
                hasPsram ? "found" : "not found",
                hasPsram ? ESP.getPsramSize() : 0);

  // 先用单缓冲稳定模式验证摄像头，避免双缓冲占用过多 PSRAM。
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(hasPsram){
      config.fb_location = CAMERA_FB_IN_PSRAM;
    } else {
      // 没有 PSRAM 时保持 QVGA，避免 DRAM 帧缓冲过大。
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // 初始化摄像头
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  Serial.printf("Camera sensor PID: 0x%04x\n", s->id.PID);
  // 修正部分摄像头的色偏和翻转问题
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  
  // 这里再强制设置一下 QVGA，双重保险
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  camera_fb_t *testFrame = esp_camera_fb_get();
  if (testFrame) {
    Serial.printf("Initial camera capture OK: %ux%u, %u bytes\n",
                  testFrame->width, testFrame->height, testFrame->len);
    esp_camera_fb_return(testFrame);
  } else {
    Serial.println("Initial camera capture FAILED: check 5V power, camera ribbon cable, and OV2640 module");
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// 设置闪光灯
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  // 连接 WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false); // 关闭 WiFi 休眠，提高响应速度

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // 启动 Web 服务器
  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // 主循环留空，任务都在 Web Server 线程里跑
  delay(10000);
}
