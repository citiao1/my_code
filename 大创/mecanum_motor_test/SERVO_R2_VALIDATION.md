# SERVO_R2 validation — 2026-10-26

Problem: main-loop-only key polling can miss a whole short press during the six 10 ms PS2 reconfiguration delays. R1 also tested double-click timing at the second release, losing some clicks when the second press started within the window but ended later.

Change: independent 1 ms SysTick GPIO/gesture capture with 10 ms debounce and a bounded single-producer/single-consumer event queue. All UART operations stay in the main loop. Overflow stops and clears pending actions. A hold released before its queued RUN is consumed is never started late. Fault recovery flushes events and requires release of a held key. The existing 300 ms double-click window and 500 ms hold threshold remain.

Validation: GCC -std=c99 -Wall -Wextra -Werror button and protocol suites passed. Button coverage includes all 80 phases of 15 ms short presses with 80 ms foreground processing intervals, a second release outside the double-click window, no stale RUN after a stalled foreground, bounded queue overflow, bounce, boot-held key, tick rollover, recovery and no UART calls from ISR sampling. Two offline Python flasher checks passed (sparse HEX byte preservation/explicit sectors and invalid-vector rejection).

Native Keil target compiled with ARMCC 5.06 update 5 build 528: 0 errors, 0 warnings. Code=25548, RO=1816, RW=240, ZI=11368 bytes. No clocks, pinmux or peripheral initialization changed, so CubeMX regeneration was not needed. Only preserved USER CODE regions contain the new SysTick hook.

HEX SHA256: `54525549f0a95e11f32b5199c6221c0bc5d75ca52be20cf4c52c453ec2903aac`.

Headless connection was verified on COM7 at 76800 baud, 8E1: ROM bootloader 0x31, chip ID 0x0413, flash 512 KiB. FlyMCU --help did not emit help/exit; only the process spawned for that test was terminated. The reusable headless programmer uses stm32loader 0.7.1 with board-matching DTR/RTS polarity, and does not automate FlyMCU windows.

Programming outcome will be recorded after the attempt. Improved physical button response still requires user observation; source and simulation tests do not establish physical behavior.
