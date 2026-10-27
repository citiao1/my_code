"""PID endpoint selection: WebSocket service or exclusive local serial connection.

The serial endpoint reuses the command/STOP/drive-lease engine, but starts no
HTTP, WebSocket or TCP server and adds no diagnostic heartbeat traffic.
"""
from __future__ import annotations

import asyncio
import importlib.util
import json
import sys
from contextlib import asynccontextmanager
from pathlib import Path
from types import SimpleNamespace

import websockets


def load_serial_engine():
    name = "bm50_serial_command_engine"
    if name not in sys.modules:
        path = Path(__file__).resolve().parents[1] / "mecanum-motor-console/bridge/motor_vofa_bridge.py"
        spec = importlib.util.spec_from_file_location(name, path)
        module = importlib.util.module_from_spec(spec)
        sys.modules[name] = module
        try:
            spec.loader.exec_module(module)
        except BaseException:
            sys.modules.pop(name, None)
            raise
    return sys.modules[name]


class SerialEndpoint:
    def __init__(self, port):
        module = load_serial_engine()
        self.module = module
        self.port = port
        self.engine = module.MotorVofaBridge(module.BridgeConfig(
            "serial", port, 115200, None, "127.0.0.1", 0, "127.0.0.1", 0, 0))
        self.messages = asyncio.Queue(maxsize=4096)
        self.engine.broadcast_web = self._emit
        self.lease_output = SimpleNamespace(send=self._emit)
        self.requests = set()
        self.reader = None
        self.closed = False
        self.fault = None
        self.cleanup_errors = []

    async def _emit(self, message):
        try:
            self.messages.put_nowait(message)
        except asyncio.QueueFull:
            self.fault = RuntimeError("serial receive queue overflow; log is incomplete")
            self.engine.fail_pending(str(self.fault))
            raise self.fault from None

    async def __aenter__(self):
        try:
            # An occupied COM port fails here; never kill another owner or share it.
            self.engine.serial_handle = await asyncio.to_thread(
                self.module.serial.Serial, port=self.port, baudrate=115200,
                timeout=.02, write_timeout=1.0, xonxoff=False, rtscts=False, dsrdtr=False)
            self.reader = asyncio.create_task(self._read())
            # Existing PID sessions consume the same verified command API greeting.
            await self._emit("BRIDGE,BM50_ACK_R3\n")
            for command in ("STOP", "DISARM", "TEL,0", "TOF,0", "PING"):
                await self.engine.execute_command(command)
            await self.engine.set_hardware_status(True, f"direct serial {self.port} @ 115200 verified")
            return self
        except BaseException:
            await self.close()
            raise

    async def _read(self):
        try:
            handle = self.engine.serial_handle
            while True:
                count = max(1, min(handle.in_waiting, 4096))
                data = await asyncio.to_thread(handle.read, count)
                if data:
                    self.engine.consume_bytes(data)
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            self.fault = exc
            await self.engine.set_hardware_status(False, f"direct serial read failed: {exc}")

    async def send(self, message):
        if self.closed or self.fault:
            raise ConnectionError(f"direct serial unavailable: {self.fault}")
        request = json.loads(message)
        if request.get("type") not in ("command", "drive_lease"):
            raise ValueError("unsupported serial request")
        if (len(self.requests) >= 64 and
                str(request.get("command", "")).strip().upper() not in ("STOP", "DISARM")):
            raise RuntimeError("too many queued serial requests")
        task = asyncio.create_task(self._dispatch(request))
        self.requests.add(task)
        task.add_done_callback(self.requests.discard)

    async def _dispatch(self, request):
        result = {"type": "result", "id": request.get("id")}
        try:
            if request["type"] == "drive_lease":
                reply = await self.engine.start_drive_lease(
                    self.lease_output, request.get("id"), request["command"], request.get("duration_ms"))
            else:
                reply = await self.engine.execute_command(request["command"])
            result.update(ok=True, reply=reply)
        except Exception as exc:
            result.update(ok=False, error=str(exc))
        await self._emit(json.dumps(result))

    async def recv(self):
        if self.closed or self.fault:
            raise ConnectionError(f"direct serial unavailable: {self.fault}")
        return await self.messages.get()

    def __aiter__(self):
        return self

    async def __anext__(self):
        return await self.recv()

    async def close(self):
        if self.closed:
            return
        try:
            if self.reader is not None and not self.reader.done():
                for command in ("STOP", "DISARM", "TEL,0", "TOF,0"):
                    try:
                        await self.engine.execute_command(command)
                    except Exception as exc:
                        self.cleanup_errors.append(f"{command}: {exc}")
        finally:
            self.closed = True
            await self.engine.cancel_drive_lease()
            for task in tuple(self.requests):
                task.cancel()
            await asyncio.gather(*tuple(self.requests), return_exceptions=True)
            if self.reader is not None:
                self.reader.cancel()
                await asyncio.gather(self.reader, return_exceptions=True)
            if self.engine.rx_publish_task is not None:
                self.engine.rx_publish_task.cancel()
                await asyncio.gather(self.engine.rx_publish_task, return_exceptions=True)
            if self.engine.serial_handle is not None:
                self.engine.serial_handle.close()
            if self.cleanup_errors:
                print("Serial cleanup unverified: " + "; ".join(self.cleanup_errors), file=sys.stderr)

    async def __aexit__(self, *args):
        await self.close()


@asynccontextmanager
async def connect_endpoint(args):
    port = getattr(args, "serial_port", None)
    if port:
        async with SerialEndpoint(port) as endpoint:
            yield endpoint
    else:
        async with websockets.connect(args.url, open_timeout=5) as endpoint:
            yield endpoint
