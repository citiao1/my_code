# WHEELTEC-IOS bridge

The WHEELTEC module uses a 9600 baud transparent UART on the vehicle side and
an FFE0 BLE service on the PC side. The browser does not connect to BLE
directly. This bridge owns the BLE connection, reconnects after link loss and
exposes local WebSocket and VOFA+ TCP endpoints. It subscribes on FFE1 and
uses the dedicated FFE2 characteristic for commands. A partial GATT discovery
is rejected so the console cannot report a receive-only link as connected.

Run from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\host-console\bridge\start-wheeltec.ps1
```

Keep the PowerShell window open, then open `http://127.0.0.1:8770` and choose
`WHEELTEC bridge`. The default device name is `WHEELTEC-IOS`.

Connections:

- Browser WebSocket: `ws://127.0.0.1:8766`
- VOFA+ TCPClient: `127.0.0.1:1347`
- BLE service/notify/write: FFE0/FFE1/FFE2
