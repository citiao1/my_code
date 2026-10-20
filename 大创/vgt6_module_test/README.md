# VGT6 BM50 module test

Current source/build is R17 (not flashed): [rate/heading RAM log handoff](HLOG_R17_HANDOFF.md).
The deployed R16 motion-control baseline is unchanged; incomplete FAST streaming was removed.

This STM32F407VGT6 HAL project tests the extension board one module at a
time: OLED, Bluetooth, IMU660RC, four WHEELTEC BM50 motors, four quadrature
encoders and the battery-voltage ADC. The `.ioc` file is the pin/peripheral
source of truth and the MDK-ARM project is built with Keil/Arm Compiler 5.

## Hardware mapping

| Function | Pins | Configuration |
| --- | --- | --- |
| OLED CLK/DIN/RST/DC | PD14/PD13/PD12/PD11 | SSD1306 software SPI |
| IMU660RC SCK/MISO/MOSI | PA5/PA6/PA7 | SPI1 mode 3, prescaler 64 |
| IMU660RC CS/INT1 | PB12/PC4 | software CS; INT1 reserved |
| Bluetooth TX/RX | PA2/PA3 | USART2, 9600 8N1, DMA |
| BM50 A PWM/DIR/BTAK | PC6/PD0/PD4 | TIM8_CH1, direction, brake |
| BM50 B PWM/DIR/BTAK | PC7/PD1/PD5 | TIM8_CH2, direction, brake |
| BM50 C PWM/DIR/BTAK | PC8/PD2/PD6 | TIM8_CH3, direction, brake |
| BM50 D PWM/DIR/BTAK | PC9/PD3/PD7 | TIM8_CH4, direction, brake |
| Encoder A | PA8/PA9 | TIM1 encoder mode |
| Encoder B | PA0/PA1 | TIM2 encoder mode |
| Encoder C | PB14/PB15 | both-edge EXTI software quadrature |
| Encoder D | PB6/PB7 | TIM4 encoder mode |
| Battery ADC | PC0 | ADC1_IN10, 100 kOhm / 22 kOhm divider |
| Status LED | PE2 | on while IMU communication is healthy |

The installed chassis numbering is viewed from above, starting at the
upper-right and counting counter-clockwise: motor 1 = right-front, motor 2 =
left-front, motor 3 = left-rear, and motor 4 = right-rear. Firmware channels
A/B/C/D follow motors 1/2/3/4 in that order. The mecanum transform, encoder
sign table, drive polarity table, and speed-loop target array all use this
same physical order.

For command inputs `(F,L,R)` = `(forward,left,counter-clockwise turn)`, the
four signed wheel targets are:

```text
1 RF = F + L + R       2 LF = F - L - R
3 LR = F + L - R       4 RR = F - L + R
```

The left-side BM50 DIR and encoder signs are inverted at their hardware
boundaries (`[+1,-1,-1,+1]` for channels 1..4), so the speed PID sees one
consistent chassis-forward sign for all four wheels.

PB14/PB15 can be routed to TIM12_CH1/CH2, but STM32F407 TIM12 does not have
hardware encoder mode. Therefore encoder C is decoded from both GPIO edges;
the other three encoders use timer encoder mode.

## BM50 safety behavior

The BM50 interface is single PWM + DIR + BTAK, not the dual-PWM H-bridge
interface used by the older motor project. TIM8 is configured for 10 kHz
PWM2. A compare value of zero keeps PWM high (stop); increasing compare
increases the low-level run duty.

At reset all compare values are zero and all four BTAK pins are low, so the
motors are actively braked. Motion commands are rejected until `ARM` is
received. Only one motor can jog at a time, the duty is limited to 30%, and
the command automatically stops after at most 5 seconds. `STOP` and
`DISARM` immediately set PWM to stop and pull all BTAK pins low.

Before applying the motor supply, raise all four wheels off the bench. The
BM50 supply must be 9--14 V and its ground must be connected to the MCU
ground. Do not power a motor from the MCU 5 V/3.3 V rail.

## Bluetooth commands

Send one ASCII line ending in `CR`, `LF` or `CRLF`:

```text
PING
VER
UART
STACK
STATUS
ARM
DISARM
STOP
A+
A-
B+
B-
C+
C-
D+
D-
JOG,A,10,1000
JOG,A,-10,1000
ENC
ENCZERO
CPR,380
BAT
RECOVER,A
SPEED,1
SPEED,0
SPEED,120
SPD,A,30
DRV,30,0,0
PID,A,1.2,0.2,0
PIDGET
SPEEDSTATUS
TEL,1
TEL,0
CAL
ZERO
WHO
HELP
```

`A+` through `D-` are fixed 10% / 1000 ms jogs. `JOG` accepts a signed duty
from -30 to +30 percent and a duration from 50 to 5000 ms. A positive
software command means chassis-forward: motors 1/2 invert the BM50 DIR level
because they are mirrored on the left side, while motors 3/4 keep it.
Encoder feedback uses the matching `[-1,-1,+1,+1]` sign table so positive
RPM has the same chassis-forward meaning. Verify each wheel on the
raised-wheel bench before applying load.

The default encoder count is 380 transitions per output-shaft revolution,
which assumes the BM50 motor with a 19:1 gearbox. Raw totals and deltas are
valid even if this assumption is wrong; use `CPR,<value>` to correct only the
RPM calculation. `RECOVER,<A-D>` performs the BM50 BTAK high-low-high reset
sequence while stopped, then returns to the braked state.

`BAT` reports ADC raw value and filtered millivolts using:

```text
Vbat = ADC / 4095 * 3.3 V * (100 + 22) / 22
```

This image measures voltage only. It does not impose an undervoltage cutoff
because the battery chemistry/cell count and threshold have not been set.

The diagnostic response formats are:

```text
IMU,...
ENC,tick,ready,cpr,totalA,totalB,totalC,totalD,deltaA,...,rpmA_x10,...
BAT,tick,ready,raw,millivolts,failures
DRV,tick,motor_ready,armed,active,channel,direction,duty,remaining_ms
```

Periodic telemetry is disabled in the current motor-test image. The four
formats above are emitted only when `STATUS`, `ENC` or `BAT` is requested.
When enabled with `TEL,1`, the speed stream uses the compact 27-field
`TEL` format consumed by the pure BM50 console and VOFA bridge. It contains
tick, four targets, four PWM outputs, four actual RPM×10 values, battery
voltage and basic gyro/status fields, and remains below the UART TX message
limit.

`SPEED,1`/`SPEED,0` enable or disable the four-wheel speed loop after `ARM`.
`SPEED,<rpm>` sets the target limit (except the special value `1`), `SPD`
sets one wheel target RPM, and `DRV` sets mecanum forward/left/rotate RPM.
`PIDGET` reports the four wheel gains. R16 accepts finite
`PID,<A-D>,Kp,Ki,Kd` writes in the 0..100 range, updates the selected
incremental controller and resets its state. Always query `PIDGET` after a
write and keep the output limit at +/-60%.
`SPEEDLIMIT`
returns the current target ceiling. `MOVE` and `DRV` use the documented vehicle
axes directly: positive forward moves the chassis forward and positive left
moves it to the left. R16 uses the installed order `[RF, LF, LR, RR]` directly;
no upper-layer axis swap or temporary lateral sign correction is applied.
`TEL,1` enables a 300 ms `TEL` stream for
the web console and VOFA bridge; it is off by default.

### BLE line-framing revision

`VER` returns `VER,VGT6_BM50_ATTITUDE_R17`. UART IDLE ends only a DMA
burst, never a command; the receive ring assembles BLE fragments until an
explicit CR or LF. Manual serial terminals must send a line terminator.

`LINK` reports RX bytes, complete commands, RX overflow count, TX dropped
count, UART error count and periodic-telemetry enable state. Unknown commands
are echoed as `ERR,UNKNOWN_COMMAND,<received-command>`, followed by
`RXHEX,<original-byte-hex>`.

The R3 startup reserved only 1024 bytes while Keil reported an already-known
1808-byte call path, before full indirect-call and interrupt overhead.
R4 sets `ProjectManager.StackSize=0x1000` in CubeMX and imports the verified
generated startup difference, reserving 4096 bytes. Telemetry formatting
buffers are also moved out of nested stack frames. The actual UART corruption
still requires physical verification; this fixes a confirmed memory-budget
defect, not a proven exclusive cause of every receive error.

`STACK` returns allocated/observed-used/free bytes and a guard flag.
`UART` reports configured baud, software PCLK, BRR, last HAL error flags,
and PE/NE/FE/ORE/DMA error counts. The bridge prefixes commands with LF;
after UART faults the firmware discards partial reception until a real line
terminator. It never guesses corrections to corrupted motor commands.
Stack-guard failures disarm/brake the motors and disable telemetry; only
lightweight diagnostic commands remain available. The guard does not replace
instrumented stack and control-loop validation.

Prefer `SPDLIMIT,<rpm>` for limit writes, including 1 RPM. `SPEED,0/1`
remain the loop-disable/enable commands. STOP disables the loop but preserves
ARM and the current telemetry selection; use `TEL,0` to explicitly stop a
stream. Enabling the loop again does not require a BLE reconnect.

`tests/host/run-tests.ps1` compiles the real transport, parser, telemetry and
speed-control modules with mock HAL/board I/O using GCC. It checks every
command split point, bytewise PID reception, malformed input recovery,
STOP/restart and telemetry timing. This is a protocol regression, not a
physical motor or radio test.

After a Keil build, `node tests/check-stack-budget.cjs` checks that `.ioc`,
startup, the linked map and runtime monitor agree, and compares the known
static depth with the reserved margin. Indirect/unknown paths still need
on-board STACK watermark checks.

The four wheel controllers are inner-loop incremental PID controllers. Each
cycle applies
`u(k)=u(k-1)+Kp[e(k)-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]`,
then limits the accumulated PWM command to 60%. The 10 ms encoder speed
measurement is filtered with a 0.7/0.3 first-order low-pass filter before the
PID calculation. The legacy integral-limit parameter is retained only for
command/API compatibility and does not limit the incremental update.

### R16 runtime gains and shaft-speed ceiling

All four channels start with calibrated gains at the existing 10 ms control
period. R16 accepts finite `PID,<A-D>,Kp,Ki,Kd` writes in the 0..100 range,
updates the selected controller and resets its incremental state. The live
整定器 performs guarded PID updates and readback checks.

The supplied `BM50电机规格书与使用说明书(2026.01.16).pdf`, PDF pages 4--5
(printed pages 2--3), lists a 19:1 gearbox / 380 transitions per output
revolution, motor-body rated speed 4462 RPM (+/-10%) and no-load speed
5199 RPM (+/-10%). With the project's existing 19:1 gearbox assumption,
these correspond to about 235 RPM rated and 274 RPM no-load at the output
shaft. The chosen default and maximum configurable target limit is 200 RPM,
below even the 10%-low rated reference (about 211 RPM). `SPDLIMIT` may lower
the ceiling to 1--200 RPM; higher writes are rejected, while `SPD` targets
are clamped symmetrically to the current ceiling. If the actual gearbox
differs, this limit and the encoder CPR must both be reconsidered.

This is a target clamp, not an actual-speed trip. The separate incremental
PID output cap remains +/-60% PWM. `Motor_Run` converts the duty directly
to TIM8 compare and permits up to 100%; it does not multiply by another
60%. The 30% jog limit belongs only to open-loop `JOG`. If the actual speed
lags a higher target while VOFA I9--I12 stays at +/-60, the speed loop is
output-saturated; raising Ki will not overcome this cap. Supply/load and
encoder scaling must also be checked before increasing drive capability.

### R16 attitude cascade

R16 adds `TRACKMOVE,heading_deg,field_x_rpm,field_y_rpm` for supervised motion
tests, without changing raw tuning interfaces. Its 20 ms measured-yaw frame
transform, quiet heading gate and test-side cross-track correction are explained
in [the team-code review and validation notes](RM_CHASSIS_R16_REVIEW.md).
R16 has been flashed via wireless DAP, independently read back (53356 bytes),
and confirmed running by Bluetooth version query. Ground tracking is not yet validated.

R16 uses an adaptive six-axis Mahony observer plus a zero-velocity update
(ZUPT) detector. Accelerometer correction is weighted by the confidence of the
measured gravity magnitude, so chassis acceleration does not receive the full
tilt correction. Mahony integral feedback learns residual tilt gyro bias, and
the application learns all three gyro biases only after 1.5 s of confirmed
rest. This is a self-learning stationary correction; it cannot remove
long-term yaw drift because IMU660RC is a six-axis sensor without a magnetic or
other absolute heading reference.

R16 uses an angular-velocity inner PI loop and a heading outer PD loop. Both run
at 20 ms. The angular-velocity loop uses the IMU660RC measured yaw rate and
outputs a rotation RPM command limited to +/-60 RPM. The heading loop wraps its
error to +/-180 degrees, uses measured yaw rate for damping, and limits the
generated angular-velocity target to +/-80 degrees/s. `YAW,<deg/s>` starts the
inner loop; `HEAD,<deg>` starts the heading cascade; `HEADMOVE,<heading>,<forward>,<left>`
starts the heading cascade while retaining translation; `MOVE,<forward>,<left>,<deg/s>`
combines translation with the inner loop. `YAWPID`, `HEADPID`, `YAWGET`,
`HEADGET`, `KEEP`, `ATEL,1/0`, and `ANGLESTATUS` configure and observe the
loops. A 1.5 s command watchdog stops the cascade if `KEEP` is not received.

R17 records rate PI (mode=1) and heading cascade (mode=2) in its RAM logger.
`HLOG,CLEAR`,
`HLOG,START`, `HLOG,STOP`, `HLOG,STATUS`, and `HLOG,READ,offset,count` record
the 20 ms rate/heading state without writing Flash. Reading is limited to
eight records per command and requires `STOP` and `HLOG,STOP`; the host can batch
reads and save the returned `HLOG,DATA` records as CSV.

`ATEL` fields are:

```text
ATEL,tick,mode,imu_ready,target_rate_mdps,actual_rate_mdps,
output_rpm_milli,target_heading_mdeg,actual_heading_mdeg,
heading_error_mdeg,output_dps_milli,saturated,speed_enabled,battery_mv
```

PS2 translation uses a radial deadzone and continuous X/Y mixing. Its maximum
translation wheel RPM is 30, 60 or 140, preserving direction at saturation.
The right stick requests a heading-reference rate up to 80 deg/s at every tier.
The turning reference uses 400 deg/s^2 acceleration and integrates actual
controller elapsed time. Manual control stays in mode 2:
rate feedforward plus heading PD, then rate PI. D uses reference rate minus
40 ms low-pass measured rate. Tracking error is bounded to 8 degrees while
turning. R15 release cancels reference integration and the old angle target,
clears rate integral, then uses P-only zero-rate damping with the existing
output slew limit. During braking the angle target follows measured yaw,
so no outer-loop command pulls the chassis back to its pre-stop angle.
The actual heading is latched after measured and D-filtered rate are both
within 1.5 deg/s and rotation output is below 0.5 RPM for 120 ms. If braking
does not settle within 2.5 seconds, the chassis stops. A new stick command
cancels braking without restarting wheel PIDs. Translation remains active.
After capture, ordinary hold corrects external disturbances without recapture.

Manual hold has 0.35/0.8 degree and 1.5/3 deg/s rest hysteresis. Within rest,
the rate integrator is cleared and rotation output returns to zero, avoiding
noise-driven alternating wheel targets. Hold corrections are limited to 30
deg/s; manual rotation output slews at 240 RPM/s. These are initial shaping
settings, not experimentally tuned gains. Manual integral may unwind towards
zero even while the output slew limiter is active. HEAD/YAW tuning formulas and gains
remain unchanged (HEAD 6.0/1.25, YAW 0.15/2.5).

Manual rotation gets priority within the wheel ceiling: translation is scaled
uniformly when needed. At the default 160 RPM ceiling, 60 RPM rotation leaves
100 RPM peak translation, preserving its direction. All tiers retain their
30/60/140 RPM translation maximum when enough headroom exists.
Invalid IMU data, a lost PS2 frame, STOP or SELECT stop the vehicle and require
another deliberate START.

The OLED bottom line contains the `R17` marker, receiver connection, PS2 enable
and selected speed tier. This identifies the build independently of the page.

## First bench test order

1. Leave the 9--14 V motor supply disconnected and flash the MCU.
2. Confirm the OLED, Bluetooth `PING`, IMU `WHO` and IMU calibration still
   work.
3. Compare `BAT` with a multimeter at the battery input.
4. Rotate each wheel by hand and use `ENC` to confirm the corresponding
   channel counts and determine its sign.
5. Raise all wheels, connect the BM50 supply and common ground, then send
   `ARM`.
6. Test one short command at a time: `A+`, `A-`, then B, C and D.
7. Verify that each jog stops after one second and that `STOP`/`DISARM`
   brakes immediately.
8. Record required per-wheel DIR and encoder sign corrections before adding
   any mecanum kinematics or closed-loop control.

## Build

Open `vgt6_module_test.ioc` in STM32CubeMX 6.17, keep MDK-ARM V5 selected,
and regenerate when pin/peripheral settings change. Then build
`MDK-ARM/vgt6_module_test.uvprojx` in Keil.

For the connected PUSO CMSIS-DAP, run `tools/flash-dap.ps1` after the Keil
build. It uses the verified Keil CMSIS-DAP backend when installed, with
low-speed pyOCD as a fallback. It independently compares the complete HEX
image, vector table and R16 marker, then resets and confirms the target is
running. The PUSO readback uses single-word transfers because its block-read
command can return empty packets.

The `.axf` and `.hex` outputs are under `MDK-ARM/vgt6_module_test/`.
R4 stack regeneration used the existing STM32CubeMX 6.17 / F4 V1.28.3
configuration in an isolated temporary project; only the one-line generated
startup stack-size difference was imported, preserving custom Keil groups.
The generator reported installed-plugin/schema and TIM metadata warnings;
no unrelated generated initialization changes were imported.

On October 9, 2026, R4 was compiled and linked with the active Keil
Arm Compiler 5.06 update 5 target: 0 errors, 0 warnings. The map reserves
4096 stack bytes and the known static call depth is 1108 bytes. The AXF
contains the R4 version string. The host protocol suite passed 187
assertions, the bridge passed 12 tests and the page passed 14 tests.
These results do not constitute a flash or physical UART/motor test.

R5 was subsequently built on October 9, 2026 with the same active target:
0 errors and 0 warnings. Regression results: 209 protocol checks,
18 page tests, 12 bridge tests. The 4096-byte stack budget check passed;
known static depth remains 1108 bytes. No flashing or physical motor test
was performed for R5.
