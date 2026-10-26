# SERVO ONLY R3 — 2026-10-26

Temporarily reduced App_Init/App_Process to ServoTest and Display only. No Vehicle, Ps2Control, Battery, IMU, encoder acquisition or motor PWM startup paths are called. Existing generated peripheral setup and inactive sources remain for restoration; no CubeMX pin/clock configuration was changed.

The OLED permanently shows `SERVO ONLY R3`, plus KEY/PRESS/CMD/OK/UP/ERR diagnostics. Communication errors do not hide the version. Raw key state allows electrical input inspection independently of communication readiness; successful command count is based on protocol acknowledgements, not observed physical movement.

Native Keil target: ARMCC 5.06 update 5 build 528, 0 errors and 0 warnings. Code=17184, RO=1624, RW=72, ZI=11112 bytes. Existing GCC button suite passed including all 80 phases of 15 ms presses with an 80 ms foreground interval. Servo protocol and gesture timing are unchanged.

HEX SHA256: `ef05b55e106bedfd204c6577db8c6b8212727ed577409b5810683d115d2a4ae6`.

The HEX contains the literal version marker `SERVO ONLY R3` at flash address `0x08000DBC`. Flash and physical OLED observation will be recorded separately after programming.
