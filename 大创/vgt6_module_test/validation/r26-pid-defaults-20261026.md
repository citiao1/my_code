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

Source/build backup commit: `ab6e29c9`.

## Programming status

Initial attempt: after STOP/DISARM acknowledgement, two read-only attach attempts at 100 kHz failed in DebugPortSetup with SWD/JTAG No ACK. No flash/reset/unlock command was issued in that attempt. Build log and DAP failure log remain local ignored artifacts.

## User-requested retry: successfully programmed

The same image SHA256 above was rechecked, source backup verified, and the single PUSO 1234:5678 probe detected again. Read-only preflight succeeded: RUNNING, chassis disarmed, angle/speed disabled, all four targets/output zero, no CPU fault registers set.

Existing `tools/flash-dap.ps1` used the native Keil target. Keil reported `Erase Done.Programming Done.Verify OK.Application running` and `Flash Load finished`. Independent readback confirmed STM32F4 device `0x101f6413`, 1024 KiB flash, vectors and R26 marker at `0x08005934`. Reset completed; post-reset critical readback passed with core RUNNING. No mass erase, unlock or option-byte change.

Post-reset serial readback through the already-running compatibility bridge:

```text
PID,1.1000,0.0991,0.0000,1.1000,0.0998,0.0000,1.1000,0.0998,0.0000,1.1000,0.1008,0.0000
HEADPID,5.5000,2.5000,80,20,D_MEAS
YAWPID,0.1815,1.8062,60,80,20
STACK,4096,2456,1640,1
```

STOP/DISARM acknowledged; angle/speed off, PS2 locked. Readback assertions confirmed all requested startup gains and unchanged rate PI. No ARM, motor command, live tuning or closed-loop motion test performed.
Evidence: `r26-pid-defaults-retry-preflight-20261026.json`, `r26-pid-defaults-postflash-20261026.json`; flash helper/native Keil logs remain local ignored artifacts.
