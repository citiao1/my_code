# MSPM0G3507 vehicle project handoff

Last updated: 2026-07-16

## Current status

The complete hardware test stack is working on the real vehicle. The user has
confirmed that the BLE command link, motors, encoders, grayscale sensor, OLED,
9AGM IMU and browser console are all functioning. The current firmware is V8
and builds with Keil ARM Compiler 6.19 with zero errors and zero warnings.

The only newly recorded issue is the large motor start-up dead zone. Both
motors need an absolute PWM command above approximately 55 percent before they
start rotating. This is deliberately not compensated in the current code.

## Repository layout

- `Code/diansai_app.c`: vehicle application, drivers and command processing.
- `Code/diansai_app.h`: application entry declarations.
- `User/main_diansai.c`: actual Keil application entry point.
- `User/main.c`: retained vendor example; it is not the active application.
- `Keil/LQ_MSPM0GX_LIB.uvprojx`: project to build and flash.
- `Keil/Objects/LQ_MSPM0GX_LIB.hex`: latest V8 firmware image.
- `host-console/`: browser vehicle console.
- `host-console/bridge/wheeltec_bridge.py`: PC BLE/WebSocket/VOFA bridge.
- `host-console/bridge/start-wheeltec.ps1`: normal bridge launcher.
- `README.md`: wiring, commands, telemetry and build instructions.

The project was created as an independent copy based on the Longqiu
`LQ_MSPM0GX_LIB_V2.0.0` architecture. The original Longqiu library and the
reference `diansai_test` project remain available elsewhere under `diansai/`.

## Hardware wiring in the working configuration

All logic is 3.3 V and every module must share GND with the MSPM0G3507 board.
The AT8236 motor supply `VM` comes from the motor battery, not the MCU 3.3 V
rail.

| Device signal | MSPM0G3507 connection |
| --- | --- |
| WHEELTEC RXD | PA10, UART0 TX, 9600 8N1 |
| WHEELTEC TXD | PA11, UART0 RX, 9600 8N1 |
| AT8236 AIN1 | PB2, TIMA1 CH0 PWM |
| AT8236 AIN2 | PB3, TIMA1 CH1 PWM |
| AT8236 BIN1 | PB10, Servo2, TIMG0 CH0 PWM |
| AT8236 BIN2 | PB11, Servo3, TIMG0 CH1 PWM |
| Left encoder pulse/A | PA7 |
| Left encoder direction/B | PA3 |
| Right encoder pulse/A | PA8 |
| Right encoder direction/B | PB7 |
| Grayscale OUT | PA27, ADC |
| Grayscale S0/S1/S2 | PA26/PA25/PA24 |
| OLED SCK/SDA/RST/DC/CS | PA17/PA16/PB21/PB23/PB22 |
| 9AGM SCK/data pair/CS | PA12/PA13+PA14/PA2 |

The firmware probes both possible PA13/PA14 MOSI/MISO assignments and keeps the
mapping that returns LSM6DSR `WHO_AM_I = 0x6B`. Do not hard-code the opposite
mapping based only on one conflicting pin diagram.

## Firmware design and resolved problems

- AT8236 uses four independent 20 kHz PWM channels. Positive output drives
  IN1 and negative output drives IN2; both inputs are cleared before direction
  changes. PB4/PB5 are not used.
- `DRV,throttle,steering` calculates left as `throttle + steering` and right as
  `throttle - steering`. `MOTOR,left,right` controls each side directly.
- A 500 ms command watchdog stops both motors after command-link loss.
- UART0 runs at 9600 8N1 on PA10/PA11. V8 polls its RX FIFO every millisecond
  before the slower 10 ms task guard. Do not restore the earlier vendor RX
  interrupt/ring-buffer path; it allowed telemetry uplink but lost commands.
- UART TX uses a non-blocking 1024-byte queue so 9600-baud telemetry does not
  stall IMU integration or the motor watchdog.
- SysTick is a real 1 ms interrupt. IMU integration uses measured elapsed time.
- LSM6DSR is configured for 104 Hz and 2000 dps with 70 mdps/LSB sensitivity.
  All gyro axes are calibrated at boot and bias is slowly tracked only while
  the vehicle is confirmed stationary.
- Yaw is relative gyro-only heading. It is now responsive and usable, but it
  will not have long-term absolute north reference without LIS3MDL fusion.
- Grayscale telemetry reports raw 12-bit ADC readings from 0 to 4095. White and
  black references can be captured separately.
- OLED refresh is limited to 200 ms while fast control and IMU work continue
  independently, eliminating the earlier blocking refresh behavior.

## WHEELTEC bridge operation

The Longqiu LQ208 module was abandoned for this project. The working module is
`WHEELTEC-IOS`; the vehicle-side UART baud rate is 9600.

Start from PowerShell:

```powershell
cd D:\my_code\my_code\diansai\MSPM0G3507_Diansai_Test
powershell -ExecutionPolicy Bypass -File .\host-console\bridge\start-wheeltec.ps1
```

Keep that PowerShell window open and open:

```text
http://127.0.0.1:8770
```

Normal shutdown is `Ctrl+C` in the launcher window. The script stops both the
bridge and the HTTP server it created. Closing the browser tab alone sends a
safe `STOP` but intentionally leaves the bridge running.

The repository snapshot includes the current Windows `.venv` because the user
requested every workspace change to be uploaded. A virtual environment is
machine-specific. On another computer, if its embedded Python path is invalid,
delete `host-console/bridge/.venv` and run `start-wheeltec.ps1` again; the
script will create a fresh environment and install `requirements.txt`.

If the launcher was forcibly closed and background processes remain, stop only
the processes listening on the three project ports:

```powershell
Get-NetTCPConnection -State Listen |
  Where-Object { $_.LocalPort -in 8766, 8770, 1347 } |
  Select-Object -ExpandProperty OwningProcess -Unique |
  ForEach-Object { Stop-Process -Id $_ }
```

Bridge endpoints and BLE roles:

- HTTP console: `127.0.0.1:8770`
- Browser WebSocket: `127.0.0.1:8766`
- VOFA+ TCP client endpoint: `127.0.0.1:1347`
- BLE service: FFE0
- BLE notification: FFE1
- BLE host write: FFE2 with response

The bridge must report `notify=FFE1 write=FFE2 mode=response`. Windows can
occasionally return a partial GATT table. The bridge now rejects a missing
FFE1 or FFE2 instead of falsely reporting a receive-only FFE1 link as usable.
If the module stops advertising, make sure no phone is connected and power
cycle the WHEELTEC module.

## Commands and verification sequence

Useful commands are `HELP`, `PING`, `STOP`, `MOTOR`, `DRV`, `ENCZERO`,
`IMUZERO`, `GRAYWHITE`, `GRAYBLACK` and `GRAY`.

After any future communication change, verify in this order:

1. Confirm the bridge reports FFE1 notify and FFE2 write.
2. Send `HELP` and require the expected `ACK` response.
3. Send `ENCZERO` and confirm both OLED encoder totals clear.
4. Only after the downlink is proven, lift the vehicle and test PWM/motors.
5. Check encoder polarity, a physical 90-degree IMU rotation and all eight raw
   grayscale channels.

This order matters. Earlier missing motor PWM was caused by commands never
reaching `MotorSet()`, not by the PWM peripherals themselves.

## Build and flash

Open `Keil/LQ_MSPM0GX_LIB.uvprojx`, select target `LQ_MSPM0G3X_LIB`, build and
flash `Keil/Objects/LQ_MSPM0GX_LIB.hex`. The known command-line build is:

```powershell
& 'D:\Keil5\UV4\UV4.exe' -b `
  'D:\my_code\my_code\diansai\MSPM0G3507_Diansai_Test\Keil\LQ_MSPM0GX_LIB.uvprojx' `
  -t 'LQ_MSPM0G3X_LIB' -j0 `
  -o 'D:\my_code\my_code\diansai\MSPM0G3507_Diansai_Test\Keil\build_v8.log'
```

The latest verified result is `0 Error(s), 0 Warning(s)` and the boot signature
is `BOOT,MSPM0G3507_DIANSAI_TEST,V8,WHEELTEC,9600`.

## Pending motor dead-zone work

Observed but intentionally unresolved: neither motor reliably starts below an
absolute command of roughly 55 percent. Do not silently change the current
linear command mapping in unrelated work.

A future session should first measure left/right and forward/reverse start and
run thresholds separately, along with motor battery voltage under load. Then it
can choose between minimum-duty remapping, a brief start-up kick, or closed-loop
encoder speed control. Any compensation must preserve zero as a true stop and
must be tested with the vehicle lifted before road testing.
