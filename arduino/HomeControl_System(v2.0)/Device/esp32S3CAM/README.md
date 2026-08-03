# ESP32-S3-CAM N4R2 camera test

This is a separate PlatformIO project. It does not modify the existing
`esp32CAM` project.

Detected hardware:

- ESP32-S3 revision 0.2
- 4 MB flash
- 2 MB embedded PSRAM
- CH340/CH340K USB serial adapter (the COM number can change after reset)

The initial test uses the common ESP32-S3-CAM / ESP32-S3-EYE camera pinout:

| Signal | GPIO |
| --- | ---: |
| XCLK | 15 |
| SIOD | 4 |
| SIOC | 5 |
| D0-D7 | 11, 9, 8, 10, 12, 18, 17, 16 |
| VSYNC | 6 |
| HREF | 7 |
| PCLK | 13 |

Build and upload:

```powershell
platformio run
platformio run --target upload
platformio device monitor
```

Expected serial output includes `PSRAM: OK` and `Camera test frame OK`. If
camera initialization fails, the physical board uses a different camera
pinout and its exact model or schematic is required.

This board's installed sensor reports no hardware JPEG support. The test
firmware therefore captures RGB565 frames and converts them to JPEG for the
browser endpoints.
