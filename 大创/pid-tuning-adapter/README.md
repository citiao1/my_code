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

The current R10 firmware reports per-wheel gains (`Kp=1.2`, `Ki` is
0.0991/0.0998/0.0998/0.1008 by default, `Kd=0`). `PID`
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

The R10 firmware also provides a RAM-only direction-loop logger. It records
the 20 ms controller state without writing Flash; `attitude_tuner.py` can
retrieve the stopped log with `--experiment direction_identify`, and
`direction_model.py` fits the closed angular-rate plant and ranks bounded
`HEADPID(Kp,Kd)` candidates. Candidate gains are review-only until an
independent validation trajectory accepts them.

Candidate gains are never sent to the MCU. Check sign, units, sample period,
output limit and saturation before applying them. In particular, converting
continuous gains directly into the current incremental controller is wrong;
use the `incremental` candidate so `Ki_step = Ki_continuous*T` and
`Kd_step = Kd_continuous/T`.

## R10 physical experiment modes

The online adapter can operate the vehicle only with `--live` and one
explicit hardware confirmation. All motion modes keep one ARM and the
watchdog heartbeat for the whole session, save raw samples, check limits and
send STOP in a `finally` path.

Angular velocity uses a matched spin experiment:

```powershell
python pid-tuning-adapter/attitude_tuner.py --live --confirm-hardware `
  --loop angular_velocity --experiment spin --target 15 --rounds 4 `
  --output pid-tuning-adapter/runs/rate-spin
```

The vehicle holds zero, turns clockwise, returns to zero, turns
counter-clockwise, and returns to zero. Both signs must pass before a rate
candidate is accepted.

Heading uses a measured-distance round trip. R10's `HEADMOVE` command keeps
translation active while the outer heading target changes:

```powershell
python pid-tuning-adapter/attitude_tuner.py --live --confirm-hardware `
  --loop direction --experiment heading_round_trip --target 15 `
  --forward-rpm 30 --distance-m 1.0 --rounds 3 `
  --output pid-tuning-adapter/runs/heading
```

To collect high-rate direction data, use the same continuous ground trajectory
with the RAM HLOG enabled:

```powershell
pid-tuning-adapter\.venv\Scripts\python.exe `
  pid-tuning-adapter\attitude_tuner.py `
  --loop direction --experiment direction_identify --target 15 `
  --distance-m 0.5 --forward-rpm 20 --seconds 30 --rounds 1 `
  --live --confirm-hardware `
  --output pid-tuning-adapter\runs\direction-identify-a15-v20
```

The resulting `round-01-hlog.csv` can be analyzed without touching the
controller:

```powershell
pid-tuning-adapter\.venv\Scripts\python.exe `
  pid-tuning-adapter\direction_model.py `
  pid-tuning-adapter\runs\direction-identify-a15-v20\round-01-hlog.csv `
  --kp 5 --kd 1.25
```

The adapter zeros the encoder, drives the declared distance, changes the
heading reference during the leg, reverses automatically, and stops after
returning the same distance. The distance uses a 127 mm wheel and 380
transitions per output revolution (`1.05 mm/count`); time is only a timeout.

For a ground speed validation of the currently applied wheel gains:

```powershell
python pid-tuning-adapter/attitude_tuner.py --live --confirm-hardware `
  --loop wheel_speed --experiment ground_speed --speed-rpm 30 `
  --distance-m 1.0 --output pid-tuning-adapter/runs/ground-speed
```

This mode drives forward and back, records average wheel RPM/PWM and scores
tracking. Wheel PID writes remain guarded by the live tuning session and are
verified with `PIDGET` readback.

## WebSocket Contract

- Endpoint: `ws://127.0.0.1:8766`
- Bridge greeting: `BRIDGE,BM50_ACK_R3`
- Commands use JSON requests: `{"type":"command","id":N,"command":"VER"}`
- Results use JSON replies with the same `id`; telemetry remains plain CSV text.
- `TEL` has 27 comma-separated fields. For wheel index `i` in `A..D`, target is
  field `4+i`, output is field `8+i`, and actual RPM is field `12+i` divided by
  10. Firmware tick is field 1; speed-loop-enabled is field 3.

The passive logger sends only diagnostics and telemetry selection. The live
attitude adapter additionally sends the declared experiment commands
(`ARM`, `MOVE`, `HEAD`, `HEADMOVE`, `ENC`, `ENCZERO`, `KEEP`, `STOP`)
inside the confirmed live session. `MOVE` is the same R10 cascade entry used by
the console's W/A/S/D and Q/E controls. It refuses firmware-version or stack-guard
mismatches and verifies any attitude PID write by readback.

## Handoff For Project Integration

The browser console already talks to the local bridge over WebSocket; the BLE
bridge remains the sole BLE Central and serial owner. Keep any future tuning
tool on this WebSocket boundary rather than letting it open a COM port or BLE
connection. It should consume `TEL` feedback, associate each sample with the
active target, and return candidate gains for explicit review.

Before adding parameter writes, first change the MCU protocol and PID ownership
deliberately: R10 accepts finite 0..100 wheel PID writes, and `PIDGET` is the
required readback. Do not infer successful parameter application from a bridge ACK alone;
read back all four gains and require a firmware version/capability check.
Keep output limits, target RPM limits, stack checks, STOP behavior, and manual
confirmation in the safety review.

Tests:

```powershell
pid-tuning-adapter\.venv\Scripts\python.exe -m unittest discover -s pid-tuning-adapter -p "test_*.py" -v
```
