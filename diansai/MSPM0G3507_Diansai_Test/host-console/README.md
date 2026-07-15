# MSPM0G3507 WHEELTEC console

The vehicle uses UART0 PA10/PA11 at 9600 8N1 with the `WHEELTEC-IOS` module.
For stable operation, the browser does not own the BLE connection directly.
The Python bridge handles FFE0 BLE notifications, writes and reconnects.

Start everything from the project root:

```powershell
powershell -ExecutionPolicy Bypass -File .\host-console\bridge\start-wheeltec.ps1
```

Keep that terminal open and visit:

```text
http://127.0.0.1:8770
```

The default browser mode is `WHEELTEC local bridge`. A direct 9600 baud Web
Serial option remains available for wired troubleshooting.
