# SERVO ONLY R3 — 2026-10-26

Temporarily reduced App_Init/App_Process to ServoTest and Display only. No Vehicle, Ps2Control, Battery, IMU, encoder acquisition or motor PWM startup paths are called. Existing generated peripheral setup and inactive sources remain for restoration; no CubeMX pin/clock configuration was changed.

The OLED permanently shows `SERVO ONLY R3`, plus KEY/PRESS/CMD/OK/UP/ERR diagnostics. Communication errors do not hide the version. Raw key state allows electrical input inspection independently of communication readiness; successful command count is based on protocol acknowledgements, not observed physical movement.

Native Keil target: ARMCC 5.06 update 5 build 528, 0 errors and 0 warnings. Code=17184, RO=1624, RW=72, ZI=11112 bytes. Existing GCC button suite passed including all 80 phases of 15 ms presses with an 80 ms foreground interval. Servo protocol and gesture timing are unchanged.

HEX SHA256: `ef05b55e106bedfd204c6577db8c6b8212727ed577409b5810683d115d2a4ae6`.

The HEX contains the literal version marker `SERVO ONLY R3` at flash address `0x08000DBC`. Headless COM7 programming completed at 76800 baud. Chip ID 0x0413, bootloader 0x31 and 512 KiB flash were confirmed. Explicit sectors [0, 1] were backed up, merged, erased, written and fully read back. All 32768 bytes matched, including the displayed-version literal. Readback SHA256: `2eaa2869275a45536bb7b5a915b9bafe1ca5f9f657fe4549f03472d62df43916`. Reset to flash was issued. Local record: `vehicle-console/flash-records/20261026-214751-735035/result.json`.

Source/build backup commit: `a5ad984a`. A transient concurrent Git index lock delayed this commit until just after programming; no user changes or lock files were removed. Physical OLED display and servo response remain unobserved by the agent; the user should see `SERVO ONLY R3` and an increasing UP counter on the board.
