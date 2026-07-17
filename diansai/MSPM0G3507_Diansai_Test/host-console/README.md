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

The line-direction panel edits `Kp/Ki/Kd`, maximum differential ratio and base
speed. The browser converts parameters to x1000 integers for `LINEPID` and
`LINEDIFF`, waits for `STA` readback, and repeats `LINE,1,speed` every 100 ms
while the start button is active. V21 runs the grayscale direction loop every
20 ms and uses separate short-gap, blind-turn and confirmed-loss states.

The grayscale panel shows raw ADC and normalized values for every channel.
Normalized values come from `NRM,time,valid,n0..n7`; white is 0 and black is
1000. The status reads `归一化有效` only when firmware reports `valid=1`.

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

## VOFA+ speed-loop tuning

Start the same bridge, select FireWater in VOFA+, and connect its TCP client to
`127.0.0.1:1347`. Firmware emits `SPD` every 250 ms while a `DRV` command is
active. The bridge exposes these `speed:` channels in order:

```text
time, active,
left target, actual, error, feedforward, PID correction, PWM,
right target, actual, error, feedforward, PID correction, PWM,
left Kp, Ki, Kd, right Kp, Ki, Kd
```

Speeds are m/s in VOFA+, percentages remain percent, and speed gains are locked
at the verified `4000,800,0`. VOFA+ can send newline-terminated commands back
through the same TCP link.

VOFA+ numbers the `speed:` values from `CH0`. The speed channels are:

| Signal | VOFA channel | Position in frame |
| --- | ---: | ---: |
| Left target speed | `CH2` | 3rd value |
| Left actual speed | `CH3` | 4th value |
| Right target speed | `CH8` | 9th value |
| Right actual speed | `CH9` | 10th value |

For a basic speed-loop plot, enable `CH2`, `CH3`, `CH8` and `CH9`. Their unit
is m/s, so a 120 mm/s target appears as `0.12`.

The same `SPD` frame also creates a `yaw:` group:

| Signal | VOFA channel |
| --- | ---: |
| Target yaw rate | `CH3` |
| Actual yaw rate | `CH4` |
| Yaw-rate error | `CH5` |
| Feedforward correction | `CH6` |
| PID correction | `CH7` |
| Final wheel correction | `CH8` |

Yaw rates are degrees/s. Correction channels are m/s. `CH9` is the configured
maximum yaw rate and `CH10..CH13` are `Kp/Ki/Kd/Kff` in the reference micro-unit
integers. For a basic yaw-loop plot, enable `CH3`, `CH4` and `CH5`.

## VOFA+ line-direction tuning

During line following the bridge publishes a `line:` group every 200 ms:

| Signal | VOFA channel |
| --- | ---: |
| Raw / filtered line error | `CH5` / `CH6` |
| Direction PID output | `CH7` |
| Target / actual yaw rate | `CH8` / `CH9` |
| Final wheel correction | `CH10` |
| Left / right wheel target | `CH11` / `CH12` |
| Base speed / differential ratio | `CH13` / `CH14` |
| Normalized grayscale sum | `CH15` |
| Recovery mode / elapsed time | `CH16` / `CH17` |
| Active black-channel count | `CH18` |

Error is percent, yaw rate is degrees/s, and wheel values are m/s. First lift
the vehicle and move a black line under G0 then G7: G0 must produce a positive
target yaw rate and `left target < right target`. If the sign is opposite, stop
and reverse the firmware channel weights; do not use a negative Kp.

For ground tuning, start at 100 mm/s with `Ki=0` and differential ratio 0.650.
Temporarily set `Kd=0`, increase Kp until the filtered error begins to oscillate,
then reduce Kp by 20-30%. Increase Kd in small steps to damp the oscillation.
Keep Ki at zero unless a repeatable steady offset remains. Tune the differential
ratio only after Kp/Kd track reliably.

Recovery modes are `0=waiting`, `1=tracking`, `2=gap hold`, `3=blind turn` and
`4=confirmed lost`. Gap hold lasts 150 ms. Blind turn is entered only when G0,
G7 or a large error recorded a fresh turn direction; it requests 90 degrees/s,
temporarily uses differential ratio 1.0 and times out after 1200 ms. During
blind turn, at least one and at most four black channels must be seen for two
consecutive 20 ms samples before normal PID resumes.
