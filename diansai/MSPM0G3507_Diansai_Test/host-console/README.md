# MSPM0G3507 WHEELTEC console

The vehicle uses UART0 PA10/PA11 at 9600 8N1 with the `WHEELTEC-IOS` module.
For stable operation, the browser does not own the BLE connection directly.
The Python bridge handles FFE0 BLE notifications, writes and reconnects.

Start everything from the project root:

```powershell
powershell -ExecutionPolicy Bypass -File .\host-console\bridge\start-wheeltec.ps1
```

Keep that terminal open and visit:

```text
http://127.0.0.1:8770
```

The default browser mode is `WHEELTEC local bridge`. A direct 9600 baud Web
Serial option remains available for wired troubleshooting.

The direction pad also accepts keyboard control: `W`/`S` set forward/reverse
throttle and `A`/`D` set left/right steering. Combined keys are supported. Key
release, window blur, page hiding and disconnect all stop keyboard control.

The speed-loop panel shows the locked left/right `4000,800,0` gains. The
yaw-rate panel controls loop enable, maximum rate and `Kp/Ki/Kd/Kff`; applying
it sends `YAWPID`, `YAWRATE` and `YAW`. Firmware values are read back from `STA`
while idle and from `SPD` while driving. The telemetry panel shows wheel-speed
and yaw-rate cascade details.

The remote-heading panel displays the locked `diansai_test`-style
PD/feedforward outer loop: `Kp=4.000`, `Kd=0.300`, `Kff=1.000`, maximum
80 degrees/s. Remote straight driving selects it automatically.
The automatic test alternates positive/negative relative heading steps and
returns to the baseline. A step advances only after heading error and yaw rate
remain inside their limits for the configured settle time; timeout, focus loss,
page hiding or disconnect sends `STOP`.

The square-test button starts the firmware state machine: drive 1 m, turn left
90 degrees, repeat four times. The browser sends a start command once and then
keepalive-only commands every 200 ms, displays phase/leg/distance from `SQR`, and
stops refreshing after complete or error state.

The line-direction panel edits `Kp/Ki/Kd`, maximum differential ratio and base
speed. The browser converts parameters to x1000 integers for `LINEPID` and
`LINEDIFF`, waits for `STA` readback, and repeats `LINE,1,speed` every 200 ms
while the start button is active. V23 defaults to direction PID `200/0/350`,
differential ratio `0.650`, and runs the grayscale direction loop every 20 ms
with separate short-gap, blind-turn and confirmed-loss states.

The grayscale panel shows raw ADC and normalized values for every channel, plus
the SW1-selected track mode. K1 captures background and K2 captures target line.
SW1 down means white background/black line; SW1 up means blue background/white
line. Normalized values always use background=0 and target-line=1000. The signed
formula intentionally supports the measured blue-large/white-small polarity.

After valid calibration, hold K1 or K2 for 500 ms to start local line following
at 200 mm/s. The buzzer remains on during arming. Local running does not depend
on the browser's periodic commands; confirmed loss or another K1/K2 press stops
it. The telemetry panel also shows SW1/SW2 state and PB19-derived battery voltage.

## Motor dead-zone measurement

The independent motor sliders cover the firmware's full `-100..100` command
range. The dead-zone panel drives one motor and one direction at a time in
one-percent steps, displays its live encoder response, and stores separate
start and sustained-run values for all four motor/direction combinations.

Verify direction and stop behavior with the wheels lifted first. Measure the
actual dead zone on the ground in the intended mechanical load condition. For
each combination, increase PWM from rest until motion is reliable and record
the start value. Without stopping the motor, reduce PWM until it is just able
to keep moving and record the run value. Enter the motor-battery voltage under
load if available. Results are retained in browser local storage.

## VOFA+ tuning

Start the bridge, select FireWater in VOFA+, and connect its TCP client to
`127.0.0.1:1347`. The bridge always sends one fixed 66-value `tuning:` frame.
`TEL`, `SPD`, and `LIN` update this shared frame, so a given `I` index never
changes meaning when different firmware telemetry rows arrive.

The main heading channels are:

| Signal | VOFA channel |
| --- | ---: |
| Target heading | `I4` |
| Rate-limited reference heading | `I5` |
| Actual heading | `I6` |
| Heading error | `I7` |
| Reference yaw rate | `I8` |
| Heading-loop output / target yaw rate | `I9` |
| Actual yaw rate | `I10` |

Headings are degrees and yaw rates are degrees/s. Plot `I4 + I6` to confirm the
commanded step, `I5 + I6` to tune reference tracking, and `I9 + I10` to inspect
the heading-loop output through the already tuned yaw-rate loop.

Speed channels are `I15/I16` (left target/actual) and `I18/I19` (right
target/actual). Their unit is m/s, so 200 mm/s appears as `0.2`. Yaw-rate
channels are `I34/I35/I36` (target/actual/error). Line-control channels occupy
`I48..I65`; in particular, filtered line error is `I53`, target/actual yaw rate
is `I55/I56`, and left/right wheel targets are `I58/I59`.

For the line-direction sign check, lift the vehicle and move the selected target
line under G0 then G7. G0 must produce a positive target yaw rate and
`left target < right target`. If the sign is opposite, stop and reverse the
firmware channel weights; do not use a negative Kp.

For ground tuning, start at 200 mm/s with `Ki=0` and differential ratio 0.650.
Temporarily set `Kd=0`, increase Kp until the filtered error begins to oscillate,
then reduce Kp by 20-30%. Increase Kd in small steps to damp the oscillation.
Keep Ki at zero unless a repeatable steady offset remains. Tune the differential
ratio only after Kp/Kd track reliably.

Recovery modes are `0=waiting`, `1=tracking`, `2=gap hold`, `3=blind turn` and
`4=confirmed lost`. Gap hold lasts 150 ms. Blind turn is entered only when G0,
G7 or a large error recorded a fresh turn direction; it requests 90 degrees/s,
temporarily uses differential ratio 1.0 and times out after 1200 ms. During
blind turn, at least one and at most four target-line channels must be seen for two
consecutive 20 ms samples before normal PID resumes.
