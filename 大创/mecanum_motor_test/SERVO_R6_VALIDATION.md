# SERVO BLE R6 — 2026-10-26

Bluetooth USART2 PD5 TX / PD6 RX restored to 9600 baud 8N1 from native .ioc regeneration; generated usart.c was compared and imported with exactly this one-line baud change. Servo retains USART3 PC10 TX / PC11 RX at 1 Mbps. SerialDma_InitBluetooth enables only USART2; USART3 remains exclusively owned by the servo binary protocol. Starts in standby; local USER toggles reciprocation.

Custom signed relative angles 0.1–720 degrees, speed 100–1500 counts/s, signed continuous rotation and configurable reciprocation are supported. Firmware command parser requires bounded ASCII frames with sequence ID and XOR checksum, validates ranges before motion, remembers eight motion request/results to suppress duplicate relative moves, and stops on UART receive faults. Remote operation requires a heartbeat within 3000 ms; loss stops and clears remote operation. No automatic relative-motion retry after a missing servo acknowledgement.

Native Keil ARMCC 5.06u5 b528: 0 errors, 0 warnings. Code 20718, RO 1938, RW 104, ZI 12960 bytes. Controller, servo protocol, command parser and six browser-protocol Node tests passed. Physical Bluetooth I/O remains to be checked.

HEX SHA256: 8ff8bd0ead4dbd39884ed47f384e1a4f997838d1fa5015d914c63d851a981b71.

Independent host UI is being built in servo-bluetooth-console. Programming outcome will be recorded separately.
