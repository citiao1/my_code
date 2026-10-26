# SERVO AUTO R5 — 2026-10-26

User explicitly requested automatic +30/-30 degree reciprocation at power-up and a USER start/stop toggle. This version starts automatic motion after initialization and therefore starts the requested motion after programming/reset as well. USART3 PC10 TX / PC11 RX, 1 Mbps 8N1, ID 1, servo/USER/OLED-only runtime are retained.

The button now toggles on each debounced press (10 ms, independently sampled at 1 ms), with no double-click window or hold gesture. A held button toggles only once. Input capture continues through communication faults. OLED version is `SERVO AUTO R5`; operating labels are AUTO, PAUSE and FAULT, with the existing key/command/error/uptime counters.

Mode 3 is configured once during initialization. Each new start clears the interrupted relative goal to zero before enabling torque. Each leg sends one relative goal (+341 or -341 counts = +/-29.9707 degrees), without reconfiguring mode or toggling torque every leg. The application waits at least 1200 ms before checking status/moving feedback at 100 ms intervals, requires not-moving feedback before reversing, dwells 250 ms at the endpoint and faults if a leg remains moving at 3000 ms. Pause disables torque. Resume begins a new forward/back pair from the current position rather than finishing an interrupted leg.

A missing/invalid motion acknowledgement is ambiguous: the command may have executed. Such a motion is never automatically resent. It triggers torque-off and pauses automatic motion. Recovery attempts only torque-off/configuration/zero-goal initialization every 500 ms; successful recovery stays paused until USER is pressed. Protocol error/status failure and movement timeout also pause. Wire disconnection can prevent delivery of torque-off until communication returns.

GCC -std=c99 -Wall -Wextra -Werror button/controller and USART3 protocol suites passed. Coverage: automatic forward/reverse alternation, immediate pause/resume, held key/bounce, all 80 phases of a 15 ms press with 80 ms foreground servicing, failed ACK without relative retry, paused recovery, timeout without reverse, boot-held key, tick rollover and no ISR UART. Protocol coverage includes one frame per leg, clearing interrupted goals and moving/status feedback.

Native Keil ARMCC 5.06 update 5 build 528: 0 errors, 0 warnings. Code=17040, RO=1624, RW=68, ZI=11100 bytes. No generated hardware configuration change was required.

HEX SHA256: `4932e3a1a2185bc8adb30f7413b8500d334163d2716552f4497a3dc07edc492e`.
Version marker `SERVO AUTO R5` is present at `0x08000DA0`.

Headless COM7 programming completed at 76800 baud. Confirmed chip 0x0413, ROM bootloader 0x31 and 512 KiB flash. Sectors [0, 1] were backed up/merged and explicitly erased, then all 32768 bytes written/read back successfully. Readback SHA256: `4b75521dc830b4cf4cb868bf9cb0eb42a9d8401f6f80dcc604922c00deddf4a4`. Reset to flash issued, which starts the user-requested automatic test. Source/build backup: `3813d8d2`. Local record: `vehicle-console/flash-records/20261026-222322-883038/result.json`. CodeGraph sync completed. Physical motion has not been observed by the agent.
