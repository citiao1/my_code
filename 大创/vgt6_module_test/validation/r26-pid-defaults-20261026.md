# R26 PID defaults — 2026-10-26

User requested only the gains shown in two screenshots to become firmware startup defaults.

| Channel | Kp | Ki | Kd |
|---|---:|---:|---:|
| A | 1.1000 | 0.0991 | 0 |
| B | 1.1000 | 0.0998 | 0 |
| C | 1.1000 | 0.0998 | 0 |
| D | 1.1000 | 0.1008 | 0 |

Heading PD: Kp=5.5000, Kd=2.5000. Angular-rate PI remains 0.1815/1.8062.
The screenshot's 80-degree command target is not a startup motion command.

Only three source lines changed: common speed Kp and heading Kp/Kd. Per-wheel Ki and all Kd already matched. Runtime tuning remains available. No other behavior/configuration changes.

Existing Keil target built with ARMCC 5.06u5 build528: 0 errors, 0 warnings; Code=58890, RO=2534, RW=224, ZI=13240 bytes.
HEX SHA256: `8137B3A0DA42FDF0462638C949AF644F32FC502ADCE3469E34BD71AB362C69E9`.

PUSO 1234:5678 detected. Read-only preflight showed zero targets/output, angle/speed off, but chassis armed. COM12 was owned by existing compatibility bridge; STOP/DISARM sent through its localhost WebSocket, both acknowledged. PS2 locked. No ARM or motion commanded.

Programming and post-reset parameter readback are recorded separately after the source backup commit.
