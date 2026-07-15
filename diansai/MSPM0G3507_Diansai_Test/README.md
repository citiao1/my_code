# MSPM0G3507 Diansai hardware test

This is an independent Keil project based on `LQ_MSPM0GX_LIB_V2.0.0`.
It does not modify the original LQ library checkout.

## Included tests

- WHEELTEC-IOS UART/BLE command and telemetry link at 9600 8N1
- Dual AT8236 motor driver with PWM dynamically routed to the active input
- Two directional encoders with pulse accumulation and speed reporting
- Eight-channel grayscale sensor in OUT/S0/S1/S2 polling mode, raw 12-bit ADC
- LQ OLED status display
- 9AGM LSM6DSR six-axis readout and ID check (`WHO_AM_I = 0x6B`)
- Browser host console over Web Serial or Web Bluetooth

## Wiring

All signal levels are 3.3 V. The MCU, expansion board, sensors, WHEELTEC-IOS and
AT8236 must share GND.

| Device | Module signal | MSPM0G3507 signal |
| --- | --- | --- |
| WHEELTEC-IOS | RXD | PA10 / UART0 TX, 9600 baud |
| WHEELTEC-IOS | TXD | PA11 / UART0 RX, 9600 baud |
| WHEELTEC-IOS | VCC | Use the voltage printed on the module |
| WHEELTEC-IOS | GND | GND |
| AT8236 channel A | AIN1 | PB2 / TIMA1 CH0 PWM |
| AT8236 channel A | AIN2 | PB3 / TIMA1 CH1 PWM |
| AT8236 channel B | BIN1 | PB10 / Servo2 / TIMG0 CH0 PWM |
| AT8236 channel B | BIN2 | PB11 / Servo3 / TIMG0 CH1 PWM |
| Left encoder | pulse/A | PA7 |
| Left encoder | direction/B | PA3 |
| Right encoder | pulse/A | PA8 |
| Right encoder | direction/B | PB7 |
| Grayscale | OUT | PA27 / ADC |
| Grayscale | S0 | PA26 |
| Grayscale | S1 | PA25 |
| Grayscale | S2 | PA24 |
| OLED | SCK / SDA / RST / DC / CS | PA17 / PA16 / PB21 / PB23 / PB22 |
| 9AGM | SCK / SPI data pair / CS | PA12 / PA13+PA14 auto-probed / PA2 |

The four AT8236 inputs use four independent PWM channels. For positive output,
only `AIN1`/`BIN1` receives PWM; for negative output, only `AIN2`/`BIN2`
receives PWM. Both inputs are cleared before changing direction. PB4/PB5 are
not used by the motor driver.

AT8236 motor power `VM` must come from the motor battery, not from the MCU
3.3 V rail. Connect motors to `AOUT1/AOUT2` and `BOUT1/BOUT2`. Lift the car
off the floor for the first motor test.

## Build and flash

1. Install Keil MDK and `TexasInstruments.MSPM0G1X0X_G3X0X_DFP 1.3.1`.
2. Open `Keil/LQ_MSPM0GX_LIB.uvprojx`.
3. Select the `LQ_MSPM0G3X_LIB` target and build.
4. Flash `Keil/Objects/LQ_MSPM0GX_LIB.hex` with the selected SWD probe.

The project compiles with ARM Compiler 6.19.

## Host console

From this project directory, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\host-console\bridge\start-wheeltec.ps1
```

Keep the PowerShell window open, then visit `http://127.0.0.1:8770` in Chrome
or Edge. The script starts the web console and connects to `WHEELTEC-IOS`
through BLE service FFE0. It exposes a local WebSocket with automatic reconnect.
Direct COM troubleshooting uses 9600 baud.

Press `Ctrl+C` in that PowerShell window to stop the bridge. The script also
stops the HTTP server that it started. Closing only the browser tab stops the
motors but does not stop the bridge process.

## Commands

| Command | Function |
| --- | --- |
| `MOTOR,left,right` | Set each motor from -100 to 100 percent |
| `DRV,throttle,steering` | Differential drive mixing compatible with `diansai_test` |
| `STOP` | Immediately coast both AT8236 channels low |
| `ENCZERO` or `ZERO` | Clear both encoder totals and yaw |
| `IMUZERO` | Restart gyro Z bias collection and clear yaw |
| `GRAYWHITE` | Capture current eight channels as white reference |
| `GRAYBLACK` | Capture current eight channels as black reference |
| `GRAY` or `GRAYCAL` | Return captured grayscale references |
| `PING` | Link check; does not extend the motor watchdog |
| `HELP` | Print supported commands |

Motor commands must be refreshed within 500 ms. Loss of Bluetooth/serial
traffic therefore stops both motors even if the host UI closes unexpectedly.

## Telemetry

- `TEL,...`: motor state, encoder speed, yaw and IMU health; compatible with
  the short telemetry format used by the existing `vehicle-console`.
- `STA,...`: encoder totals, pitch/roll and eight raw grayscale ADC readings
  in the range 0 to 4095.
- `DBG,id,ax,ay,az,gx,gy,gz,mosi,miso`: raw LSM6DSR diagnostic values and
  the detected SPI data pin mapping.
- `CAL,...`: eight white and eight black grayscale reference values.

The LQ library and the expansion-board material disagree about PA13/PA14 MOSI
and MISO direction. This project tries both mappings at startup and locks onto
the one that returns `WHO_AM_I = 0x6B`. The OLED and `DBG` line report the
selected mapping; the original vendor files remain intact.

At boot, keep the vehicle completely still for about three seconds while the
OLED shows `IMU CAL HOLD STILL`. The LSM6DSR gyro runs at 104 Hz and yaw uses
the correct 2000 dps sensitivity of 70 mdps/LSB. V8 calibrates all three gyro
axes and tracks bias while the motors and encoders confirm that the car is
stationary. Yaw is still a relative gyro-only angle; long-term absolute heading
requires calibrated LIS3MDL magnetometer fusion.

V8 reconfigures SysTick as a hardware 1 ms interrupt and uses a non-blocking
UART transmit queue. IMU integration uses the actual elapsed milliseconds even
when another device task takes longer than one control period. The 9600 baud
telemetry link therefore no longer pauses IMU sampling or prematurely expires
the motor command watchdog.

UART0 RX is polled every millisecond in V8 instead of relying on the vendor RX
interrupt wrapper. This makes WHEELTEC commands independent of FIFO interrupt
index and threshold behavior.

## Known issue

Both installed motors currently have a large measured start-up dead zone. The
vehicle does not begin moving until the absolute PWM command is above about
55 percent. This observation is recorded for later calibration; V8 does not
apply minimum-duty remapping or a start-up boost.
