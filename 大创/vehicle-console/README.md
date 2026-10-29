# C30D Vehicle Console

1. Serve this directory from localhost and open it with desktop Chrome or Edge.
2. Keep the default BLE mode for modules exposing service FFE0. The console detects FFE1 or FFE2 for writes.
3. Use serial COM mode only when the module appears as a Windows COM port.

The firmware uses USART2 at 9600 baud. Motion commands are refreshed every 100 ms and the firmware stops the motors if no command arrives for 400 ms.

## VOFA+ bridge

Run `bridge/start-bridge.ps1`. The bridge owns the BLE connection and exposes:

- TCP `127.0.0.1:1347` with FireWater telemetry for VOFA+.
- WebSocket `ws://127.0.0.1:8766` for the web console.

Select `Local bridge` in the web console when the bridge is running. A BLE module generally cannot be connected directly by the web page and bridge at the same time.

## Headless UART firmware download

Run from the workspace root:

```powershell
./vehicle-console/flash-servo.ps1
./vehicle-console/flash-servo.ps1 -ProbeOnly
```

The default image is the mecanum_motor_test HEX, using COM7 at 76800 baud. The first invocation creates an isolated `.venv-flash` environment and installs the pinned dependencies. `-Port`, `-Baud`, and `-Image` override defaults. Close any serial session that owns COM7 before programming.

FlyMcu2188.exe did not provide CLI help/exit when tested with `--help`; no documented CLI was found. This script uses [stm32loader](https://github.com/florisla/stm32loader) and the STM32 AN3155 ROM bootloader protocol with the board's existing DTR-low reset / RTS-high BootLoader wiring. It operates without window control. A read-only probe confirms chip ID 0x0413 and 512 KiB flash before any write. It backs up affected sectors, merges the HEX into that backup, erases only affected sectors, writes and reads back all bytes, then resets to flash. Bytes outside the HEX are preserved, including tails within erased sectors. No mass erase, unprotect, or option-byte write is used. Records and backups are saved locally under `flash-records/` (ignored by Git). A failure after erase/write leaves ROM bootloader available for retry; a reset command is not physical boot confirmation.

Offline checks: `.venv-flash/Scripts/python.exe -m unittest discover -s tools -p test_flash_stm32_uart.py` from this directory. These checks simulate flash and do not touch COM7.
