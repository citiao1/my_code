# MSPM0G3507 vehicle project handoff

Last updated: 2026-07-18

## Current status

The complete hardware test stack is working on the real vehicle. V9 changed
AT8236 PWM from fast decay to slow decay. Ground testing then measured start/run
thresholds of 8/8 percent left forward and 7/7 percent for left reverse and both
right directions.

V10 added the measured four-direction motor map, enabled only the incremental
wheel-speed loop, retained raw `MOTOR` PWM for diagnostics, and added VOFA+
speed-loop telemetry and PID commands. V11 corrected the encoder scale using
direct measurements from this vehicle. V12 temporarily disabled speed
feedforward for PID-only tuning and allowed PID to use the full +/-100 percent
output range. V13 disabled the unused OLED and fixed the periodic control delay
and wheel-speed calculation; the user confirmed this completely fixed the
periodic stop and that `4000,800,0` tracks very well. V14 locked those speed
gains and enabled yaw-rate -> wheel-speed cascade control. Its first vehicle
test exposed positive feedback: target yaw rate was `+12.0 degrees/s` while GZ
reported `-316.5 degrees/s`. V15 corrected the LSM6DSR yaw-rate polarity. V16
keeps that control behavior, splits the monolithic application into hardware
modules, and adds debounced onboard keys plus a non-blocking buzzer. V16 builds
cleanly. V17 assigns the keys to real calibration actions, adds per-channel
grayscale normalization, and separates adjacent buzzer patterns. V17 builds
cleanly. Real-board testing then proved K1/K2 were reversed relative to the
vendor pin table; V18 swaps PB15/PB14 in `board_io` so physical K1 is white and
physical K2 is black. That physical key order is verified; the new V22
blue-background/white-line mode still requires its first track test.
V19 added stored line-direction PID/differential parameters and the matching web
controls. V20 enables the 20 ms grayscale direction loop, feeds its target yaw
rate into the verified yaw-rate loop, adds LINE start/stop UI and LIN telemetry,
and stops after sustained line loss. V21 replaces the single loss timeout with
TRACKING, GAP_HOLD, BLIND_TURN and LOST recovery states for right-angle turns.
V22 adds SW1-selected white-background/black-line and blue-background/white-line
modes, background/target-line calibration semantics, 500 ms key-held local
start, continuous arming sound, SW2 reporting, PB19 battery monitoring and the
matching web readouts. Direction defaults are now `200/0/350`, differential
ratio `0.65`. V23 changes local/browser line speed to 200 mm/s, enables the
reference heading PD/feedforward loop for remote straight-line hold, adds
`HEAD/HEADPID/HEADSET/HEADCFG`, and adds a browser heading step-test state
machine. It also reduces 9600-baud traffic and isolates GATT write failures from
the browser WebSocket. V24 locks remote heading at `4.000/0.300/1.000`, 80
degrees/s, automatically selects that loop for remote straight driving, retains
the separate `200/0/350` grayscale direction loop for line mode, adds a
four-side 1 m/left-90-degree square state machine, and moves UART0 transmission
to DMA channel 0.

## Repository layout

- `Code/diansai_app.c/.h`: 10 ms scheduler, typed-command semantics, mode
  ownership and watchdogs; it no longer contains peripheral or protocol formatting.
- `Code/vehicle_motor.c/.h`: dual AT8236 slow-decay PWM and direction protection.
- `Code/vehicle_encoder.c/.h`: encoder ownership, calibrated conversion and
  wheel-speed filtering.
- `Code/vehicle_imu.c/.h`: LSM6DSR probing, calibration and attitude/yaw update.
- `Code/vehicle_gray.c/.h`: eight-channel ADC sampling, calibration snapshots
  and background=0/target-line=1000 normalization.
- `Code/vehicle_battery.c/.h`: PB19 ADC1_CH6 sampling, divider conversion and
  low-pass filtering.
- `Code/vehicle_command.c/.h`: pure conversion of UART text into typed commands;
  it deliberately owns no motors, parameters or mode state.
- `Code/vehicle_telemetry.c/.h`: compatible TEL/STA/SPD/LIN/SQR/MOD/KEY/CAL/NRM/DBG
  formatting from an application-owned read-only snapshot.
- `Code/vehicle_line_control.c/.h`: normalized grayscale error, filtering,
  direction PID, target-yaw mapping and lost-line protection.
- `Code/vehicle_square_test.c/.h`: encoder-distance square-test state machine,
  phase settling and timeout protection.
- `Code/wheeltec_link.c/.h`: UART0 RX line assembly and DMA0-driven 1024-byte TX queue.
- `Code/board_io.c/.h`: verified K1/K2/K3 mapping, SW1/SW2 debounce and PA28
  buzzer patterns/continuous arming output.
- `Code/vehicle_cascade_control.c/.h`: staged control logic; V20 accepts the
  line loop's target yaw rate and limits yaw correction by the differential ratio.
- `User/main_diansai.c`: actual Keil application entry point.
- `User/main.c`: retained vendor example; it is not the active application.
- `Keil/LQ_MSPM0GX_LIB.uvprojx`: project to build and flash.
- `Keil/Objects/LQ_MSPM0GX_LIB.hex`: latest built firmware image.
- `host-console/`: browser vehicle console.
- `host-console/bridge/wheeltec_bridge.py`: PC BLE/WebSocket/VOFA bridge.
- `host-console/bridge/start-wheeltec.ps1`: normal bridge launcher.
- `README.md`: wiring, commands, telemetry and build instructions.
- `documentation/MSPM0G3507_智能车完整说明书.md`: complete Chinese AI/reference manual.
- `output/pdf/MSPM0G3507_智能车完整说明书.pdf`: rendered human manual.
- `documentation/manual_artifacts/`: PDF scripts, extracted text, rendered pages,
  contact sheets and QA summary; keep future documentation byproducts here.

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
| Onboard K1/K2/K3 | PB15/PB14/PB16, active low; K1/K2 order verified on board |
| Onboard buzzer | PA28, currently configured active high |
| SW1/SW2 | PB6/PB8, active low through 2.2 kOhm pull-ups |
| Battery divider | PB19/ADC1_CH6; BAT--10 kOhm--ADC--1.5 kOhm--GND |

The firmware probes both possible PA13/PA14 MOSI/MISO assignments and keeps the
mapping that returns LSM6DSR `WHO_AM_I = 0x6B`. Do not hard-code the opposite
mapping based only on one conflicting pin diagram.

## Firmware design and resolved problems

- AT8236 uses four independent 20 kHz PWM channels. V9 and later use slow decay:
  positive is `IN1=1, IN2=PWM`, negative is `IN1=PWM, IN2=1`, and zero clears
  both inputs. Both inputs are cleared before direction changes. PB4/PB5 are
  not used.
- `DRV,throttle,steering` now requests forward speed and yaw rate. Each throttle
  percent is 6 mm/s; steering maps to +/-`max_yaw_rate`. The yaw PID produces
  the differential wheel target, and `MOTOR,left,right` remains raw percent PWM.
- Speed feedback is filtered as `0.65*old + 0.35*raw`. The speed PID is the
  standard incremental form used by both reference projects. With feedforward
  disabled for current tuning, both cumulative correction and final output are
  limited to +/-100 percent. V14 locks both sides to `4000,800,0`.
- V14 yaw control is a position-form PID matching `diansai_test`: target minus
  measured error, dt-based integral and derivative, target-rate feedforward,
  +/-300 degree-second integral limit, anti-windup and +/-600 mm/s output limit.
  Defaults in command micro-units are `1000,2000,0,1205`; maximum yaw rate is
  150 degrees/s.
- V23 heading control matches `diansai_test`: a reference angle advances at
  most 80 degrees/s, then `4.0*angle_error + 0.30*(reference_rate-yaw_rate) +
  1.0*reference_rate` generates the yaw-rate target. It activates when remote
  steering returns to zero while throttle remains nonzero; manual turns bypass it.
- V15 multiplies both LSM6DSR GZ-to-yaw-rate conversion paths by `-1`. The V14
  test showed opposite target/feedback signs and runaway rotation, so this is a
  coordinate-polarity correction rather than a PID change. Target generation,
  differential mixing and all gains are unchanged.
- A 500 ms command watchdog stops both motors after command-link loss.
- UART0 runs at 9600 8N1 on PA10/PA11. V8 and later poll its RX FIFO every millisecond
  before the slower 10 ms task guard. Do not restore the earlier vendor RX
  interrupt/ring-buffer path; it allowed telemetry uplink but lost commands.
- UART TX uses DMA channel 0 behind a non-blocking 1024-byte queue, so 9600-baud
  telemetry does not stall IMU integration or the motor watchdog. RX remains a
  low-cost FIFO poll because commands are short and variably delimited; RX DMA
  would require an idle-line/ring-buffer boundary layer without meaningful load reduction.
- SysTick is a real 1 ms interrupt. IMU integration uses measured elapsed time.
- LSM6DSR is configured for 104 Hz and 2000 dps with 70 mdps/LSB sensitivity.
  All gyro axes are calibrated at boot and bias is slowly tracked only while
  the vehicle is confirmed stationary.
- Yaw is relative gyro-only heading. It is now responsive and usable, but it
  will not have long-term absolute north reference without LIS3MDL fusion.
- Grayscale telemetry reports raw 12-bit ADC readings from 0 to 4095. V22 maps
  every channel with `(raw-background)*1000/(line-background)` and clamps it to
  0..1000. Signed division is required: blue is measured large and white small,
  so blue-background/white-line calibration has a negative denominator while
  still yielding background=0 and target-line=1000. Every span must be at least
  32 ADC counts. References are RAM-only and reset when SW1 changes mode.
- V16 contains no OLED calls in the application scheduler. The vendor OLED
  library remains in the base project, but firmware does not initialize or
  refresh the screen.
- V16 configures K1/K2/K3 directly as PB14/PB15/PB16 pull-up inputs because the
  current vendor `LQ_Key_Init()` incorrectly selects push-pull output mode.
  V18 corrects the real-board order. In V22, K1/PB15 captures background and
  K2/PB14 captures target line; K3/PB16 recalibrates the gyroscope. SW1 down is
  white background/black line, SW1 up is blue background/white line. Each
  capture gives one beep, valid normalization gives a separate three-beep
  pattern, and successful gyro calibration gives two beeps.
- The PA28 buzzer is driven through an S8050 and behaves as an active buzzer.
  Software controls duration/rhythm, not reliable pitch. V22 adds an exclusive
  continuous mode for the 500 ms local-start interval; changing pitch would
  require a passive buzzer and timer PWM.
- Key changes emit `KEY,time,pressed,released,short,long,held`; each mask uses
  bit 0/1/2 for K1/K2/K3. `KEYS` requests a snapshot and `BEEP,ms` queues one
  10..5000 ms tone.
- V17 emits `NRM,time,valid,n0..n7` after calibration queries or captures. This
  is an additive frame; existing `TEL`, `STA`, `SPD` and `CAL` layouts are unchanged.
- V20 accepts `LINEPID,kp_milli,ki_milli,kd_milli`, `LINEDIFF,ratio_milli`,
  `LINECFG` and `LINE,enable,speed_mm_s`. V23 defaults are
  `200000,0,350000,650`. `LINE,1,200` starts at 200 mm/s after grayscale
  normalization and IMU checks pass.
- With both references valid, holding K1 or K2 for 500 ms starts local line
  following at 200 mm/s. Arming/running reject remote state-changing commands;
  local running bypasses the remote command watchdog and stops on confirmed
  loss or another K1/K2 press. SW1 mode changes stop and clear calibration. SW2
  is only debounced/reported until a later function is specified.
- Battery telemetry uses PB19/ADC1_CH6, eight-sample averaging, a one-quarter
  new-value low-pass and the schematic divider ratio 23/3. The initial 3.300 V
  VDDA assumption must be checked against a multimeter before adding cutoff logic.
- The compiled AI8051U path runs `Chassis_Control()` every 2 ms and its direction
  outer loop every third call. Direction PID produces target yaw rate; yaw-rate
  PID produces `direction_output`; every inner-loop tick converts that with
  `k=direction_output*0.01`, clamps k to +/-0.65, and mixes inner/outer wheel
  targets as `1-k` and `1+0.2k`. This confirms differential is continuously
  driven by yaw-rate output. The old `User_Code/control.c` values 0.65/1.88 are
  not from the source file included by the active Keil project.
- The former bit-banged OLED full-screen refresh blocked the main loop every
  200 ms. Encoder pulses accumulated during that pause, while the old speed
  formula still divided by a fixed 10 ms interval. The resulting false speed
  spike made PID reduce PWM at a regular 200 ms cadence. V13 converts counts
  with the actual elapsed milliseconds and schedules grayscale reads by elapsed
  time rather than `now_ms % 20`.
- Incremental speed PID gains remain referenced to the original 10 ms period.
  V13 scales the Ki term by actual-period/10 ms and the Kd term inversely when
  the control task is delayed; the scale is bounded to avoid one extreme pause
  causing an extreme correction.

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

The direction pad also supports `W`/`A`/`S`/`D` keyboard control and combined
keys. Keyboard input is ignored while an input, slider or select has focus.
Key release, window blur, page hiding and disconnect all use the existing safe
stop path.

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

Useful commands are `HELP`, `PING`, `STOP`, `MOTOR`, `DRV`, `YAW`, `YAWRATE`,
`YAWPID`, `HEAD`, `HEADPID`, `HEADSET`, `HEADCFG`, `SQUARE`, `ENCZERO`, `IMUZERO`,
`GRAYWHITE`, `GRAYBLACK` and `GRAY`.
V16 adds `KEYS` and `BEEP,ms`; neither command changes the vehicle control
mode or refreshes the 500 ms motor-command watchdog.
V20 uses `LINEPID`, `LINEDIFF` and `LINECFG` for direction-loop parameters.
`LINE,1,speed` starts line following for 50..300 mm/s and must be refreshed
within 500 ms. Applying either parameter command stops the current motor mode.

Speed `PID`, `PIDL` and `PIDR` commands are retained only for protocol
compatibility and return `ERR,SPEED_PID_LOCKED,4000,800,0`. `YAWPID` uses the
reference micro-unit integers, for example `YAWPID,1000,2000,0,1205`. Any yaw
configuration change stops the motors and clears controller history.
`HEADPID` is likewise read-only and returns
`ERR,HEADING_PID_LOCKED,4000,300,1000,80`. `SQUARE,1` starts, `SQUARE,2` only
refreshes the 500 ms watchdog, and `SQUARE,0` stops.

After any future communication change, verify in this order:

1. Confirm the bridge reports FFE1 notify and FFE2 write.
2. Send `HELP` and require the expected `ACK` response.
3. Send `ENCZERO` and confirm both encoder totals clear in telemetry/web UI.
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
  -o 'D:\my_code\my_code\diansai\MSPM0G3507_Diansai_Test\Keil\build_v24.log'
```

The latest V24 build result is `0 Error(s), 0 Warning(s)` and its boot signature
is `BOOT,MSPM0G3507_DIANSAI_TEST,V24,WHEELTEC,9600`. After extracting command
parsing and telemetry formatting, program size is
`Code=39892 RO-data=2680 RW-data=48 ZI-data=22592`. This is build verification;
the line sensor order and steering sign still require the lifted-wheel check.

## Motor dead-zone calibration

V8 fast-decay thresholds were approximately 57 percent and are retained only
as diagnostic history. V9 slow-decay ground measurements are left forward 8/8,
left reverse 7/7, right forward 7/7 and right reverse 7/7 percent. These V9
values are the V10 calibration source.

V10 maps each direction independently from its measured threshold at a nonzero
target to 100 percent at 600 mm/s. Zero is an unconditional stop. There is no
start kick because every measured start value equals its run value.

V12 through V22 set `SPEED_FEEDFORWARD_ENABLED` to zero. The calibration data is
retained for later use but contributes zero motor output in these builds.

The browser console measurement panel uses the existing `MOTOR,left,right`,
`TEL` and `STA` paths. It records the four start/run pairs and an optional
under-load voltage in browser local storage. The independent motor sliders
expose the full `-100..100` raw firmware range.

## Staged cascade controller

`vehicle_cascade_control.c/.h` implements heading angle -> yaw rate -> left/right
wheel speed control. V23 enables `VEHICLE_LOOP_HEADING` only while remote heading
hold or `HEADSET` is active. The grayscale module still supplies yaw rate directly.

The speed loop uses the incremental equation `Kp*(e-e1) + Ki*e +
Kd*(e-2*e1+e2)`. Defaults are the `diansai_test` reference `4000,800,0`, scaled
internally from m/s and PWM timer counts to mm/s and percent. V12 through V22 feed
zero forward compensation into the controller and limit the accumulated PID
output and final output to +/-100 percent. V14 and later fix both controllers at
`4000,800,0`; the 10 ms Ki/Kd timing compensation remains active.

The V14-and-later yaw-rate loop uses position-form PID with the same formulas, limits and
defaults as `diansai_test`. Reference gains in m/s units are `0.001,0.002,0`
with `Kff=0.001205`; firmware converts them to mm/s internally. `D`/right turn
maps to a negative target yaw rate to preserve the established UI and chassis
direction convention. Confirm this feedback sign with wheels lifted first.

Reference caveat: Longqiu `UserCode/MIDDLE/pid.c` contains the correct standard
incremental formula. The older `User_Code/control.c` has a stray semicolon after
its Ki expression and therefore drops Kd; its Kd is configured as zero, so this
did not change that example's current PI behavior.

While `DRV` is active, firmware sends an extended `SPD` frame every 300 ms. The bridge
updates one fixed 66-value `tuning:` FireWater frame with speed, yaw-rate,
heading, system and line data. This is deliberately one row: VOFA reuses its
`I0`, `I1`, ... positions for every received line and does not isolate rows by
their textual prefix. Normal `TEL`/`STA` slow to 4000 ms.

While `LINE` is active, `SPD` runs every 1000 ms and `LIN` every 400 ms to stay
comfortably below the 9600-baud ceiling. Line telemetry occupies `I48..I65` of
the shared VOFA frame: `I52/I53` raw/filtered error, `I54` direction PID output,
`I55/I56` target/actual yaw rate, `I57` wheel correction, `I58/I59` wheel
targets, `I60` base speed, `I61` differential ratio and `I62` normalized sum.

V21 introduced, and V23 retains, recovery state fields at `I63..I65`.
Modes are WAIT=0, TRACKING=1, GAP_HOLD=2, BLIND_TURN=3 and LOST=4.
GAP_HOLD keeps the previous command for 150 ms. A fresh G0/G7 or >=35 percent
error direction allows BLIND_TURN: 90 degrees/s, effective differential ratio
1.0, maximum 1200 ms. Reacquisition requires 1..4 active channels in two
consecutive 20 ms samples; broad full-line corner patterns do not end blind turn.

The browser console shows six locked wheel-speed gain fields and provides yaw
enable, maximum rate and `Kp/Ki/Kd/Kff` inputs. It displays yaw target, actual,
error, feedforward, PID contribution and final differential correction. VOFA
uses heading target/reference/actual at `I4/I5/I6`, speed target/actual pairs at
`I15/I16` and `I18/I19`, and yaw-rate target/actual at `I34/I35`.

## Encoder calibration

The current Longqiu driver counts only the falling edge of encoder channel A.
Measured on this vehicle: left 3914 counts over 10 wheel revolutions, right 3873
counts over 10 wheel revolutions, and 1867 counts over one metre on both sides.
This gives 391.4/387.3 counts per revolution and effective wheel diameters of
66.73/66.03 mm.

V11 sets both `LEFT_COUNTS_PER_METER` and `RIGHT_COUNTS_PER_METER` to 1867. The
old STM32 values 7514/7263 used a different encoder counting multiplier and
made current-firmware speed feedback approximately four times too small. Do not
reuse those old constants in the MSPM0 project.

## Current module-boundary rule

The staged refactor is complete. Hardware ownership is in the motor, encoder,
IMU, grayscale, battery and board-I/O modules; control math is in the cascade,
line and square modules; UART transport, command syntax and telemetry formatting
are separate modules. `diansai_app.c` intentionally remains the single owner of
mode transitions, safety permissions, targets and the 10 ms schedule. Do not move
those values into protocol modules or create a second copy of vehicle state.

After future structural changes, rebuild with zero warnings and repeat `HELP`,
`ENCZERO`, lifted motor direction/watchdog, encoder polarity, IMU rotation and
grayscale checks before continuing.
