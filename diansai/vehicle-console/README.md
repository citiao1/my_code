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
