# BM50 PID Tuning Data Adapter

This standalone adapter passively records one wheel's speed-loop response from
the existing BM50 bridge WebSocket. It does not open BLE/serial, command motor
targets, write PID gains, or apply tuning results. The accompanying
`pid_tuning.py` module and `tune_pid.py` CLI perform offline SISO step-response
identification and produce review-only P/PI/PD/PID candidates.

## Setup

Run the existing console and bridge first. Keep the wheels raised and use the
console to ARM, enable the speed loop, and apply a conservative step target.
The bridge must be listening on `127.0.0.1:8766`.

```powershell
py -3 -m venv pid-tuning-adapter\.venv
pid-tuning-adapter\.venv\Scripts\python.exe -m pip install -r pid-tuning-adapter\requirements.txt
pid-tuning-adapter\.venv\Scripts\python.exe pid-tuning-adapter\pid_ws_logger.py --motor A --output pid-tuning-adapter\capture-A.csv
```

Choose `A`, `B`, `C`, or `D`. Stop with Ctrl+C. The adapter restores the
previous periodic-telemetry state on normal exit. If the WebSocket disconnects
unexpectedly, check telemetry from the console and turn it off there if needed.

## CSV

The CSV contains `time_s,setpoint_rpm,process_rpm,output_percent`. Time is the
firmware millisecond tick relative to the first recorded sample. The logger
only records samples where the speed loop reports enabled. Use the captured
step response for offline identification or to prepare candidate gains.

The current firmware reports fixed gains (`Kp=1.2, Ki=0.2, Kd=0`). `PID`
commands with other values are rejected by firmware, so results from a tuning
tool must not be written to this firmware. Applying candidate gains requires a
separately reviewed firmware change and bench validation.

The logger writes a sidecar `<capture>.csv.meta.json` describing the loop,
controller form, 10 ms firmware control period and approximately 300 ms TEL
observation period. `--controller-form incremental` is the correct form for
the current STM32 speed controller; it uses
`u += Kp*Δe + Ki_step*e + Kd_step*Δ²e`. The logger's TEL samples are not the
10 ms control samples.

## Offline generic tuner

The tuner accepts the existing columns
`time_s,setpoint_rpm,process_rpm,output_percent` and generic aliases such as
`time,setpoint,process,output`. It identifies a conservative first-order plus
dead-time model and emits candidates for:

```powershell
pid-tuning-adapter\.venv\Scripts\python.exe `
  pid-tuning-adapter\tune_pid.py `
  --input pid-tuning-adapter\capture-A.csv `
  --output pid-tuning-adapter\capture-A.tuning.json `
  --sample-time 0.01
```

It supports P, PI, PD and PID modes and three explicit controller forms:

- `parallel`: continuous notation, `Kp`, `Ki` in 1/s, `Kd` in s;
- `incremental`: conversion for the current STM32 incremental update;
- `position-discrete`: per-sample integral and derivative coefficients.

This covers speed PI, angular-velocity PI, direction PD and ordinary PID
loops as long as each is a SISO loop with a meaningful step response. It does
not claim to auto-tune arbitrary nonlinear, coupled MIMO, unstable, cascaded,
feed-forward or observer-based controllers. Gimbal angle/velocity loops must
be captured with their own `setpoint/process/output` CSV; the current BM50
bridge exposes only wheel TEL and cannot pretend to capture gimbal signals.
For a cascade, tune the inner velocity loop before the outer angle/direction
loop and use the inner loop's closed response as the outer loop's plant.

Candidate gains are never sent to the MCU. Check sign, units, sample period,
output limit and saturation before applying them. In particular, converting
continuous gains directly into the current incremental controller is wrong;
use the `incremental` candidate so `Ki_step = Ki_continuous*T` and
`Kd_step = Kd_continuous/T`.

## WebSocket Contract

- Endpoint: `ws://127.0.0.1:8766`
- Bridge greeting: `BRIDGE,BM50_ACK_R3`
- Commands use JSON requests: `{"type":"command","id":N,"command":"VER"}`
- Results use JSON replies with the same `id`; telemetry remains plain CSV text.
- `TEL` has 27 comma-separated fields. For wheel index `i` in `A..D`, target is
  field `4+i`, output is field `8+i`, and actual RPM is field `12+i` divided by
  10. Firmware tick is field 1; speed-loop-enabled is field 3.

The adapter sends only `VER`, `STACK`, `LINK`, `PIDGET`, and `TEL,0/1`. It
refuses firmware-version or stack-guard mismatches. It never sends `ARM`,
`SPEED`, `SPD`, `DRV`, `PID`, `JOG`, or `STOP`.

## Handoff For Project Integration

The browser console already talks to the local bridge over WebSocket; the BLE
bridge remains the sole BLE Central and serial owner. Keep any future tuning
tool on this WebSocket boundary rather than letting it open a COM port or BLE
connection. It should consume `TEL` feedback, associate each sample with the
active target, and return candidate gains for explicit review.

Before adding parameter writes, first change the MCU protocol and PID ownership
deliberately: currently `PIDGET` reports the same fixed gains for all wheels
and `PID,<wheel>,Kp,Ki,Kd` is only a compatibility no-op for those exact fixed
values. Do not infer successful parameter application from a bridge ACK alone;
read back all four gains and require a firmware version/capability check.
Keep output limits, target RPM limits, stack checks, STOP behavior, and manual
confirmation in the safety review.

Tests:

```powershell
pid-tuning-adapter\.venv\Scripts\python.exe -m unittest discover -s pid-tuning-adapter -p "test_*.py" -v
```
