# MSPM0G3507 Diansai hardware test

This is an independent Keil project based on `LQ_MSPM0GX_LIB_V2.0.0`.
It does not modify the original LQ library checkout.

## Included tests

- WHEELTEC-IOS UART/BLE command and telemetry link at 9600 8N1
- Dual AT8236 motor driver using 20 kHz slow-decay PWM
- Two directional encoders with pulse accumulation and speed reporting
- Eight-channel grayscale sensor in OUT/S0/S1/S2 polling mode, raw 12-bit ADC
- LQ OLED wiring retained; display disabled in V13 and later
- 9AGM LSM6DSR six-axis readout and ID check (`WHO_AM_I = 0x6B`)
- Three debounced onboard keys, two debounced DIP switches and a non-blocking
  onboard buzzer
- PB19/ADC1_CH6 battery-divider measurement and browser voltage display
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
| Onboard K1 / K2 / K3 | Active-low keys | PB15 / PB14 / PB16 after real-board verification |
| Onboard buzzer | Digital output | PA28, currently configured active high |
| DIP switch SW1 / SW2 | Active-low switch inputs | PB6 / PB8, 2.2 kOhm pull-ups |
| Battery monitor | `ADC_bat` divider input | PB19 / ADC1_CH6, 10 kOhm over 1.5 kOhm |

The four AT8236 inputs use four independent PWM channels. V9 and later use the
datasheet's slow-decay mode: positive output is `IN1=1, IN2=PWM`, negative
output is `IN1=PWM, IN2=1`, and the PWM compare is complemented so the command
still represents drive duty from 0 to 100 percent. A zero command clears both
inputs for coast/sleep, and both inputs are cleared before changing direction.
PB4/PB5 are not used by the motor driver.

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

The direction pad supports pointer control and `W`/`A`/`S`/`D`, including
combined throttle and steering keys. Releasing the final key, losing window
focus, hiding the page or disconnecting sends the normal safe stop path.

Press `Ctrl+C` in that PowerShell window to stop the bridge. The script also
stops the HTTP server that it started. Closing only the browser tab stops the
motors but does not stop the bridge process.

## Commands

| Command | Function |
| --- | --- |
| `MOTOR,left,right` | Raw -100 to 100 percent PWM; bypass speed PID and mapping |
| `DRV,throttle,steering` | Cascaded command; throttle maps to -600..600 mm/s and steering maps to the configured yaw-rate limit |
| `SQUARE,1/2/0` | Start / keep alive / stop the four-side 1 m + left 90 degree square test |
| `PID`, `PIDL`, `PIDR` | Speed gains are locked at `4000,800,0`; these commands return `ERR,SPEED_PID_LOCKED` |
| `YAW,0/1` | Disable/enable yaw-rate PID; enable requires a working IMU |
| `YAWRATE,dps` | Set maximum commanded yaw rate from 10 to 360 degrees/s |
| `YAWPID,kp,ki,kd,kff` | Set yaw-rate gains in `diansai_test` micro-units |
| `HEAD,0/1` | Disable/enable remote heading hold; enable requires the yaw-rate loop and IMU |
| `HEADPID,...` | Remote heading gains are locked at `4000,300,1000,80`; returns `ERR,HEADING_PID_LOCKED` |
| `HEADSET,target10` | Activate an absolute -180.0..180.0 degree heading target for step testing |
| `HEADCFG` | Return heading enable, gains and maximum rate |
| `LINEPID,kp,ki,kd` | Set line-direction gains as x1000 integers |
| `LINEDIFF,ratio` | Store maximum differential ratio as x1000, from 0 to 1000 |
| `LINECFG` | Return the stored line PID and differential ratio |
| `LINE,enable,speed` | Start/stop line following at 50..300 mm/s |
| `STOP` | Immediately coast both AT8236 channels low |
| `ENCZERO` or `ZERO` | Clear both encoder totals and yaw |
| `IMUZERO` | Stop, restart gyro bias collection, clear yaw and beep twice on success |
| `GRAYWHITE` | Stop and capture current eight channels as white reference |
| `GRAYBLACK` | Stop and capture current eight channels as black reference |
| `GRAY` or `GRAYCAL` | Return captured grayscale references |
| `BEEP,ms` | Queue one non-blocking buzzer tone from 10 to 5000 ms |
| `KEYS` | Return the current debounced onboard-key state in a `KEY` frame |
| `PING` | Link check; does not extend the motor watchdog |
| `HELP` | Print supported commands |

Motor commands must be refreshed within 500 ms. Loss of Bluetooth/serial
traffic therefore stops both motors even if the host UI closes unexpectedly.

## Telemetry

- `TEL,...`: motor state, encoder speed, yaw and IMU health; compatible with
  the short telemetry format used by the existing `vehicle-console`.
- `STA,...`: encoder totals, pitch/roll and eight raw grayscale ADC readings
  in the range 0 to 4095; it also reports locked speed gains, yaw settings and
  the four stored line-control parameters at the end of the frame.
- `SPD,time,active,targetL,actualL,errorL,ffL10,pidL10,pwmL,targetR,actualR,`
  `errorR,ffR10,pidR10,pwmR,kpL,kiL,kdL,kpR,kiR,kdR,targetYaw10,actualYaw10,`
  `errorYaw10,yawFfMm,yawPidMm,yawCorrectionMm,yawEnabled,maxYawRate,`
  `yawKp,yawKi,yawKd,yawKff,...heading`: 300 ms cascade tuning data while `DRV` is active.
  Wheel targets use mm/s, yaw rates use 0.1 degrees/s, and yaw correction uses
  mm/s.
- `SQR,time,active,phase,leg,progressMm,remainingMm,targetHeading10`: square-test
  state. Phases are idle/drive/turn/complete/error = 0/1/2/3/4.
- `DBG,id,ax,ay,az,gx,gy,gz,mosi,miso`: raw LSM6DSR diagnostic values and
  the detected SPI data pin mapping.
- `CAL,...,mode`: eight background and eight target-line reference values. The
  old white/black field slots are retained on the wire for compatibility.
- `NRM,time,valid,n0,...,n7,mode`: grayscale normalization result. Background
  is always 0 and the target line is always 1000; `valid=1` requires adequate
  contrast on all eight channels.
- `MOD,time,mode,sw1Down,sw2Down,localRun,backgroundValid,lineValid,batteryMv`:
  selected track-color mode, debounced switch states, local-run state,
  calibration validity and measured battery voltage.
- `KEY,time,pressed,released,short,long,held`: onboard-key event masks. Bit 0,
  bit 1 and bit 2 represent K1, K2 and K3 respectively.

The LQ library and the expansion-board material disagree about PA13/PA14 MOSI
and MISO direction. This project tries both mappings at startup and locks onto
the one that returns `WHO_AM_I = 0x6B`. The `DBG` line reports the selected
mapping; the original vendor files remain intact.

At boot, keep the vehicle completely still for about three seconds while the
IMU calibrates. V17 does not initialize or refresh the OLED. Successful IMU
startup queues two short beeps; an IMU detection failure queues three long
beeps. The LSM6DSR gyro runs at 104 Hz and yaw
uses the correct 2000 dps sensitivity of 70 mdps/LSB. V8 and later calibrate all
three gyro axes and track bias while the motors and encoders confirm that the
car is stationary. Yaw is still a relative gyro-only angle; long-term absolute
heading requires calibrated LIS3MDL magnetometer fusion.

V8 and later reconfigure SysTick as a hardware 1 ms interrupt and use a non-blocking
UART transmit queue. IMU integration uses the actual elapsed milliseconds even
when another device task takes longer than one control period. The 9600 baud
telemetry link therefore no longer pauses IMU sampling or prematurely expires
the motor command watchdog.

UART0 RX is polled every millisecond in V8 and later instead of relying on the vendor RX
interrupt wrapper. This makes WHEELTEC commands independent of FIFO interrupt
index and threshold behavior.

## V16-V24 module structure and onboard IO

V16 replaces the former monolithic `diansai_app.c` hardware implementation with
small modules that own their hardware state:

| Module | Responsibility |
| --- | --- |
| `vehicle_motor.c/.h` | Dual AT8236 PWM, output limiting and direction-change protection |
| `vehicle_encoder.c/.h` | Encoder counting, 1867 count/m conversion and speed filtering |
| `vehicle_imu.c/.h` | LSM6DSR probing, calibration, yaw-rate sign and attitude update |
| `vehicle_gray.c/.h` | Eight-channel ADC sampling, references and 0..1000 normalization |
| `vehicle_battery.c/.h` | PB19 ADC sampling, divider conversion and low-pass filtering |
| `wheeltec_link.c/.h` | UART0 line parsing and the non-blocking 1024-byte TX queue |
| `board_io.c/.h` | Key/switch debounce, events and the non-blocking buzzer pattern queue |
| `vehicle_cascade_control.c/.h` | Prepared heading -> yaw-rate -> wheel-speed controller |
| `diansai_app.c/.h` | Scheduling, command semantics, watchdogs and telemetry assembly |

V22 keeps the verified real-board key order. K1/PB15 captures the current
background and K2/PB14 captures the current target line. With SW1 down this is
white background / black line; with SW1 up it is blue background / white line.
K3/PB16 stops and recalibrates the gyroscope for about three seconds. Each
reference capture produces one short beep. Once both references exist and every
channel has at least 32 ADC counts of contrast, valid normalization produces a
separate three-beep pattern. Gyroscope calibration success produces two short
beeps; IMU failure produces three long warning beeps.

Normalization is calculated independently for every channel as
`(raw-background)*1000/(line-background)`, then clamped to 0..1000. The signed
denominator is intentional. In blue/white mode the measured blue value is large
and the white value is small, so the negative denominator still maps blue to 0
and white to 1000. Calibration references live in RAM, are cleared whenever SW1
changes mode, and must be captured again after every power cycle.

After valid calibration, holding K1 or K2 for 500 ms starts local line following
at 200 mm/s. The buzzer stays continuously on during the 500 ms arming interval.
Local arming and running reject remote drive/configuration commands and bypass
the remote 500 ms motor-command watchdog. Confirmed line loss or another K1/K2
press stops the vehicle; changing SW1 also stops and clears incompatible
calibration. SW2 is debounced and reported but has no vehicle function yet.

All ordinary buzzer timing is advanced by the 10 ms scheduler. V17 also keeps
the configured off-time between adjacent patterns, so the one-beep reference
confirmation and three-beep normalization confirmation do not merge together.

The schematic drives the PA28 buzzer through an S8050 transistor. The fitted
part behaves as an active buzzer: software can change duration and rhythm, but
cannot reliably change pitch. Pitch control would require a passive buzzer and
a timer PWM output. `BOARD_BUZZER_ACTIVE_LEVEL` centralizes the current
active-high assumption.

V23 enables the heading-angle controller only for remote straight-line hold and
`HEADSET` tests. Nonzero remote steering still directly commands yaw rate;
releasing steering while throttle remains nonzero captures the current yaw and
holds it. Line following continues to supply target yaw rate directly and does
not pass through heading hold.

V24 locks the accepted remote heading parameters at `4.000/0.300/1.000` and
80 degrees/s. `DRV` automatically enables this heading loop when steering is
zero; nonzero steering still commands the yaw-rate loop directly. `LINE` clears
heading hold and continues to use the independent grayscale direction PID
`200/0/350` with differential ratio 0.65.

V24 also adds a board-side square state machine. It drives each side to
1000 mm using the measured 1867 count/m encoders, settles, turns left 90 degrees,
and repeats four times. The browser sends `SQUARE,1` once and `SQUARE,2` every
200 ms; loss of keepalive stops the test after 500 ms. UART0 transmission now
uses DMA channel 0 from the existing 1024-byte software queue. Short variable
length receive commands remain on the low-cost FIFO poll path.

## Motor mapping and speed PID

V8 used `PWM,0` / `0,PWM`, which the AT8236 datasheet defines as fast decay.
Ground testing measured 57/57 percent for left forward, 58/57 for left reverse,
and 57/57 for both right directions (start/run). The common threshold near 50
percent is consistent with fast-decay recirculation rather than four independent
motor dead zones.

V9 changed the H-bridge PWM state to slow decay and kept the frequency at
20 kHz. Ground testing then measured start/run thresholds of 8/8 percent left
forward and 7/7 percent for left reverse and both right directions. Since start
and run values match, V10 does not use a start kick.

V10 keeps `MOTOR` as raw PWM for diagnostics. `DRV` uses an affine, four-way
feedforward map before feedback: zero target remains exactly zero; a nonzero
target starts at its measured 7 or 8 percent threshold and increases linearly
to 100 percent at 600 mm/s. Only the wheel-speed loop is enabled.

V12 through V22 disable this feedforward map for PID-only tuning. The measured
dead-zone constants remain in source, but `SPEED_FEEDFORWARD_ENABLED` is zero,
reported feedforward stays at zero, and the incremental PID output is allowed
to use the full `-100..100` percent range.

The speed loop uses the same standard incremental form as `diansai_test` and
the Longqiu `PID_INCREMENT` implementation:

```text
delta = Kp*(e-e1) + Ki*e + Kd*(e-2*e1+e2)
pid_output = clamp(pid_output + delta, -100%, 100%)
motor_output = pid_output
```

The newer Longqiu `UserCode/MIDDLE/pid.c` implements this formula correctly.
Its older `User_Code/control.c` example has a stray semicolon after the Ki term,
so that specific file accidentally discards Kd. Its configured Kd is zero, but
V10 and later deliberately use the corrected standard three-term expression.

Measured wheel speed uses the reference project's `0.65*old + 0.35*raw`
filter. The verified legacy gains `4000,800,0` are scaled internally from m/s
and timer counts to mm/s and percent. V14 locks both wheel controllers to these
values; runtime `PID`, `PIDL` and `PIDR` changes are rejected.

V13 fixes a periodic speed-control interruption. The bit-banged OLED full-screen
refresh was scheduled every 200 ms and blocked the 10 ms control task. The old
speed conversion then treated all accumulated encoder counts as if they came
from exactly 10 ms, producing a false speed spike and making PID briefly reduce
motor output. V13 disables all OLED initialization and refresh, and converts
encoder counts using the measured elapsed milliseconds. Incremental PID gains
remain based on the original 10 ms tuning period; when an update is late, the
Ki contribution scales with elapsed time and the Kd contribution scales
inversely. This prevents a delayed task from changing the intended PI/PID rate.

## Yaw-rate and speed cascade

V14 enables the yaw-rate loop when the LSM6DSR passes its ID check. Heading
control remains disabled. `DRV` steering requests a yaw rate; the yaw controller
produces a differential wheel-speed correction, then the two locked speed PI
controllers produce motor PWM:

```text
yaw_error = target_yaw_rate - measured_yaw_rate
correction = Kff*target_yaw_rate + Kp*yaw_error
           + Ki*integral(yaw_error) + Kd*d(yaw_error)/dt
left_target  = forward_speed - correction
right_target = forward_speed + correction
```

The yaw controller is the position-form PID used by `diansai_test` and the
Longqiu `PID_POSITION` example. Its correction is limited to +/-600 mm/s and
its error integral to +/-300 degree-seconds. Defaults are the reference values
`Kp=0.001`, `Ki=0.002`, `Kd=0`, `Kff=0.001205` in m/s units. The command and UI
use the same micro-unit integers: `YAWPID,1000,2000,0,1205`. Internally these
are converted to mm/s units without changing the physical gain.

Positive `D` steering remains a right turn. The LSM6DSR/body convention makes
that a negative target yaw rate, so the command sign is inverted before the
standard `target - measured` error calculation. `DRV,0,0` is a true stop.

The first V14 vehicle test showed `+12.0 degrees/s` target while the measured
rate ran away to `-316.5 degrees/s`. This proves a feedback-polarity fault, not
a PID-gain fault. V15 applies `IMU_YAW_RATE_SIGN=-1` to both normal GZ conversion
and the conversion repeated after online bias correction. Left turns now use
positive target and feedback; right turns use negative target and feedback.

## Encoder calibration

V11 uses vehicle-specific, one-edge encoder measurements made with the current
MSPM0G3507 firmware:

| Measurement | Left | Right |
| --- | ---: | ---: |
| Counts over 10 wheel revolutions | 3914 | 3873 |
| Counts per wheel revolution | 391.4 | 387.3 |
| Counts over 1 metre | 1867 | 1867 |
| Effective wheel diameter | 66.73 mm | 66.03 mm |

The previous `7514/7263 counts/m` values came from the STM32 reference project,
whose encoder timer used a different counting multiplier. The current Longqiu
driver counts one falling edge on channel A, so using the old values understated
wheel speed by approximately four times. Both V11 speed conversions therefore
use the directly measured `1867 counts/m`.

`Code/vehicle_cascade_control.c` contains the staged heading -> yaw-rate ->
wheel-speed cascade. V23 uses the reference `diansai_test` heading algorithm:
a rate-limited reference angle followed by
`Kff*reference_rate + Kp*tracking_error + Kd*(reference_rate-yaw_rate)`.
Defaults are `4.000/0.300/1.000` with an 80 degree/s limit.

## Grayscale line-direction loop

The active AI8051U project compiles `UserCode/APP/chassis.c`, not the retained
legacy `User_Code/control.c`. Timer1 calls `Chassis_Control()` every 2 ms. Every
third call, the direction PID converts electromagnetic deviation into
`target_yaw_rate`; every call, the yaw-rate PID converts measured yaw-rate error
into `direction_output`. The active reference parameters are direction
`400/0/120` with output +/-8000 and yaw-rate `0.07/0/0.007` with output +/-100.

Differential drive is therefore continuously driven by the yaw-rate loop:
`k=direction_output*0.01`, limited to +/-0.65. For positive k the inner wheel is
`speed*(1-k)` and the outer wheel is `speed*(1+0.2*k)`; negative k mirrors the
two sides. The fixed 0.01 conversion, 0.65 limit and 0.2 outer-wheel factor are
the senior developer's “set differential first, then fine tune” structure.

The planned grayscale implementation is:

1. Use normalized black intensity 0..1000 and channel weights
   `[-7,-5,-3,-1,1,3,5,7]` to calculate a centroid error in -1..1. Verify the
   physical channel order before fixing the sign.
2. Run a dedicated 20 ms line-direction PID on error x100. Its output is target
   yaw rate, not left/right wheel speed. Begin as PD with `Ki=0` and explicit
   lost-line handling.
3. Convert direction output to a target yaw rate and pass it through the already
   verified yaw-rate PID, then through the locked left/right speed PI loops.
4. The current yaw-rate loop already outputs physical wheel correction in mm/s,
   so it does not need the reference code's 0.01 unit conversion. V23 limits it
   to `abs(base_speed)*diff_ratio` and keeps the verified symmetric mix
   `left=base-correction`, `right=base+correction`. Longqiu's `+0.2*k` outer-wheel
   factor is chassis-specific and is not copied into the first tuning stage.

Tune strictly from inside out: keep the verified speed PI locked, confirm the
yaw-rate loop with step targets, then hold the differential structure fixed and
  tune the line direction loop at 200 mm/s. Start with direction `Ki=Kd=0`,
raise Kp until it follows but begins to oscillate, then reduce Kp by 20 to 30
percent. Add Kd in small steps until oscillation is damped. Keep Ki zero unless
a repeatable steady offset remains; if needed, add only a small limited integral
with anti-windup. Only after those three loops track correctly should 0.65 or
the outer-wheel 0.2 factor be changed.

V23 uses the confirmed defaults. `LINEPID,200000,0,350000` represents
`200.000/0.000/350.000`, `LINEDIFF,650` represents 0.650, and
`LINE,1,200` starts line following at 200 mm/s. The browser repeats `LINE` every
200 ms. Firmware stops after 500 ms without a motor command. A center-line gap
holds the previous target for 150 ms; loss immediately after a fresh edge/large
error enters blind turn for at most 1200 ms at 90 degrees/s and temporarily
raises the differential limit to 1.0. Two consecutive valid line samples return
to normal PID; otherwise the vehicle stops when recovery times out.

For VOFA+, start the normal bridge and connect a FireWater TCP client to
`127.0.0.1:1347`. The bridge emits one fixed 66-value `tuning:` row. `TEL`,
`SPD`, and `LIN` update that shared row instead of sending different-length rows
that would reuse VOFA's `I0`, `I1`, ... indices. VOFA can send newline-terminated
control commands back through this TCP connection.

Heading tuning uses target/reference/actual at `I4/I5/I6`, heading error at
`I7`, reference rate at `I8`, heading output at `I9`, and actual yaw rate at
`I10`. Speed target/actual pairs are `I15/I16` and `I18/I19`; yaw-rate
target/actual/error are `I34/I35/I36`. Line-control telemetry occupies
`I48..I65`, including filtered error at `I53`, target/actual yaw rate at
`I55/I56`, and wheel targets at `I58/I59`.

Battery voltage uses the schematic's `(10k+1.5k)/1.5k = 23/3` divider and an
initial 3.300 V VDDA assumption. Compare the browser reading with a multimeter
under load before using it for a low-voltage cutoff; resistor tolerance and the
actual VDDA require a board-specific correction factor for precision.

V23 lowers sustained 9600-baud load: normal `SPD` is 300 ms, line-mode `SPD` is
1000 ms, `LIN` is 400 ms and debug is 4000 ms. Browser drive refresh is 200 ms.
The BLE bridge now catches GATT write timeouts, keeps the browser WebSocket open,
and asks the BLE loop to reconnect instead of propagating one write failure into
a second apparent web-console disconnect.

The browser shows the locked speed gains and provides yaw-rate enable, limit and
`Kp/Ki/Kd/Kff` controls.
