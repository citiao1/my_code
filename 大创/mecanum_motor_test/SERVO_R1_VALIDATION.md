# SERVO_R1 validation

2026-10-26: CubeMX 6.17.0 regeneration completed (pack discovery warnings for unrelated optional packs). Active native Keil target compiled with ARMCC 5.06u5 b528: 0 errors, 0 warnings. Code 25368, RO 1816, RW 232, ZI 11352 bytes.

GCC button and protocol host suites both passed. No physical movement was triggered by host tests.

HEX SHA256: `5b2271d3cb97c8e5892234ec465a37e5994ca1699339741016eca0a8ac0b4b13`

FlyMCU attempt: COM7, existing 76800 baud, verify/run enabled, option-byte programming disabled. Outcome recorded after the attempt.

## FlyMCU result

COM7 @ 76800 bps connected successfully; bootloader 3.1, PID 0x0413 (STM32F40xx/41xx). Existing default programming flow reported full erase followed by 26 KB write, 100% completion in 11922 ms, successful start at 0x08000000, and command completed normally. Verify was checked; no separate verification pass message was shown. Option-byte programming stayed unchecked. No independent SWD/readback or physical servo observation was performed.

Firmware source/build backup: `4d53977a`. CodeGraph sync completed.
