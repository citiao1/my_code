# SERVO ONLY R4 — 2026-10-26

Servo transport moved from huart2 (PD5/PD6) to huart3 (PC10 TX / PC11 RX), 1000000 baud, 8N1. The active application remains servo/USER/OLED only. The OLED version is `SERVO ONLY R4`. Gesture timing, protocol frames and motion modes are unchanged so this is a serial-port comparison.

Changed USART3.BaudRate in the native .ioc and regenerated an isolated project copy with CubeMX 6.17.0. Imported only the generated usart.c after verifying that its sole change was huart3.Init.BaudRate 115200 -> 1000000. Other source/configuration files in the working project were preserved. CubeMX finished OK, with optional pack discovery/RIF virtual-pin loading messages; the expected generated UART change was confirmed exactly.

GCC button and USART3 protocol host suites passed with -std=c99 -Wall -Wextra -Werror. Native Keil ARMCC 5.06 update 5 build 528: 0 errors, 0 warnings. Code=17188, RO=1624, RW=72, ZI=11112 bytes.

HEX SHA256: `473ee11235c8d7a9d29b7215799c36d386573bffbebdde605fe1083309c974ad`.
The image contains `SERVO ONLY R4` at `0x08000DBC`.

Headless COM7 programming completed at 76800 baud. Confirmed chip ID 0x0413, bootloader 0x31 and 512 KiB flash. Backed up/merged and explicitly erased sectors [0, 1], then wrote/read back all 32768 bytes successfully. Readback SHA256: `b93dfa6e6c32d1cb0bd1f5a22539ddeb137f16fd7375645e863c574c425d3df3`. Reset to flash issued. Source/build backup: `80e9bdcc`. Local record: `vehicle-console/flash-records/20261026-220957-122561/result.json`. CodeGraph sync completed. Physical UART3 servo behavior is for the user to test after reconnecting the adapter.
