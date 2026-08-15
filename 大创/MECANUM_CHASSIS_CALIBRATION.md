# Mecanum chassis calibration record

Date: 2026-08-06

## Wheel mapping

| Channel | Position |
| --- | --- |
| A | Right rear |
| B | Left rear |
| C | Right front |
| D | Left front |

## Encoder counts for 10 wheel turns

These are the original measured totals. The older exported CSV labels them as
`one_turn`, but the measurement procedure was 10 turns.

| Wheel | Measured counts / 10 turns | Derived counts / turn |
| --- | ---: | ---: |
| A | 15603 | 1560.3 |
| B | 15574 | 1557.4 |
| C | 15626 | 1562.6 |
| D | 15576 | 1557.6 |
| Mean | 15594.75 | 1559.475 |

## Encoder counts for one meter

Only the rear wheels were measured over one meter. The front motors, encoders,
and wheels are the same model, so the front values below temporarily reuse the
corresponding rear value on the same side.

| Wheel | Counts / meter | Source |
| --- | ---: | --- |
| A | 7870 | Measured, right rear |
| B | 7732 | Measured, left rear |
| C | 7870 | Preliminary reuse of A, right side |
| D | 7732 | Preliminary reuse of B, left side |
| Four-wheel mean | 7801 | Derived |

## Speed-loop preparation

- Use each wheel's own derived counts-per-turn value for speed feedback.
- Use `7870 count/m` on the right and `7732 count/m` on the left as preliminary
  distance conversion values.
- C and D are estimates until the front wheels are physically checked over one
  meter. Keep measured and reused values distinguishable during tuning.
- Start speed-loop tuning with the wheels raised, low target speed, output
  limiting, and a fixed control period.
