#!/usr/bin/env python3
"""Bridge a BLE UART module to VOFA+ TCP and the vehicle web console."""

from __future__ import annotations

import argparse
import asyncio
from dataclasses import dataclass
from typing import Optional

from bleak import BleakClient, BleakScanner
import websockets


SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
NOTIFY_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"
WRITE_UUID_FFE1 = NOTIFY_UUID
WRITE_UUID_FFE2 = "0000ffe2-0000-1000-8000-00805f9b34fb"


@dataclass
class BridgeConfig:
    device: Optional[str]
    tcp_host: str
    tcp_port: int
    ws_host: str
    ws_port: int
    scan_timeout: float


class BleVofaBridge:
    def __init__(self, config: BridgeConfig) -> None:
        self.config = config
        self.client: Optional[BleakClient] = None
        self.write_uuid: Optional[str] = None
        self.write_response = False
        self.ble_line_buffer = ""
        self.tcp_clients: set[asyncio.StreamWriter] = set()
        self.ws_clients: set = set()
        self.ble_write_lock = asyncio.Lock()
        self.disconnected = asyncio.Event()
        self.loop = asyncio.get_running_loop()

    async def scan(self):
        print(f"[BLE] scanning for FFE0 devices ({self.config.scan_timeout:.0f}s)...")
        found = await BleakScanner.discover(timeout=self.config.scan_timeout, return_adv=True)
        matches = []
        for device, advertisement in found.values():
            services = {uuid.lower() for uuid in (advertisement.service_uuids or [])}
            name = device.name or advertisement.local_name or "Unknown"
            if SERVICE_UUID in services or (self.config.device and self.config.device.lower() in {device.address.lower(), name.lower()}):
                matches.append(device)
        return matches

    async def choose_device(self):
        while True:
            devices = await self.scan()
            if self.config.device:
                needle = self.config.device.lower()
                devices = [d for d in devices if needle in {d.address.lower(), (d.name or "").lower()}]
            if len(devices) == 1:
                return devices[0]
            if len(devices) > 1:
                print("[BLE] multiple devices found:")
                for index, device in enumerate(devices, start=1):
                    print(f"  {index}. {device.name or 'Unknown'}  {device.address}")
                answer = await asyncio.to_thread(input, "Select device number: ")
                try:
                    return devices[int(answer) - 1]
                except (ValueError, IndexError):
                    print("[BLE] invalid selection")
                    continue
            print("[BLE] no FFE0 device found; make sure the module is powered and not connected elsewhere")
            await asyncio.sleep(2)

    def on_disconnected(self, _client: BleakClient) -> None:
        self.loop.call_soon_threadsafe(self.disconnected.set)

    def on_notification(self, _characteristic, data: bytearray) -> None:
        payload = bytes(data)
        self.loop.call_soon_threadsafe(self.consume_ble_bytes, payload)

    def consume_ble_bytes(self, payload: bytes) -> None:
        self.ble_line_buffer += payload.decode("utf-8", errors="ignore")
        lines = self.ble_line_buffer.replace("\r", "").split("\n")
        self.ble_line_buffer = lines.pop()
        for line in lines:
            line = line.strip()
            if line:
                asyncio.create_task(self.publish_telemetry(line))

    async def publish_telemetry(self, line: str) -> None:
        await self.broadcast_web(f"{line}\n")
        firewater = self.to_firewater(line)
        if firewater:
            await self.broadcast_tcp(firewater)

    @staticmethod
    def to_firewater(line: str) -> Optional[bytes]:
        fields = line.split(",")
        if fields[0] != "TEL":
            return None
        try:
            values = list(map(int, fields[1:]))
        except ValueError:
            return None

        if len(values) >= 36:
            time_ms, enabled, link, yaw10 = values[:4]
            left_mm, right_mm, target_l_mm, target_r_mm = values[6:10]
            pwm_l, pwm_r = values[14:16]
            target_yaw10, yaw_rate10, yaw_error10, yaw_correction_mm = values[22:26]
            yaw_enabled, battery_mv, mpu_ok, yaw_feedforward_mm = values[26], values[27], values[29], values[30]
            target_heading10, heading_error10, heading_output10 = values[31:34]
            heading_enabled, heading_active = values[34:36]
        elif len(values) >= 23:
            (time_ms, enabled, link, yaw10, left_mm, right_mm, target_l_mm, target_r_mm,
             pwm_l, pwm_r, target_yaw10, yaw_rate10, yaw_error10, yaw_correction_mm,
             yaw_enabled, mpu_ok, yaw_feedforward_mm, target_heading10, heading_error10,
             heading_output10, heading_enabled, heading_active, battery_mv) = values[:23]
        else:
            return None

        # FireWater accepts one optional prefix, followed only by CSV numbers.
        channels = (
            time_ms, enabled, link,
            yaw10 / 10,
            left_mm / 1000, right_mm / 1000,
            target_l_mm / 1000, target_r_mm / 1000,
            pwm_l, pwm_r,
            target_yaw10 / 10, yaw_rate10 / 10, yaw_error10 / 10,
            yaw_correction_mm / 1000, yaw_enabled, mpu_ok,
            yaw_feedforward_mm / 1000,
            target_heading10 / 10, heading_error10 / 10, heading_output10 / 10,
            heading_enabled, heading_active,
            battery_mv / 1000,
        )
        text = "vehicle:" + ",".join(f"{value:g}" for value in channels) + "\n"
        return text.encode("utf-8")

    async def broadcast_tcp(self, payload: bytes) -> None:
        failed = []
        for writer in tuple(self.tcp_clients):
            try:
                writer.write(payload)
                await writer.drain()
            except (ConnectionError, RuntimeError):
                failed.append(writer)
        for writer in failed:
            self.tcp_clients.discard(writer)
            writer.close()

    async def broadcast_web(self, message: str) -> None:
        failed = []
        for websocket in tuple(self.ws_clients):
            try:
                await websocket.send(message)
            except Exception:
                failed.append(websocket)
        for websocket in failed:
            self.ws_clients.discard(websocket)

    async def send_ble(self, command: str) -> None:
        if not self.client or not self.client.is_connected or not self.write_uuid:
            await self.broadcast_web("STATUS,0,BLE disconnected\n")
            return
        payload = command.strip().encode("utf-8") + b"\n"
        async with self.ble_write_lock:
            await self.client.write_gatt_char(self.write_uuid, payload, response=self.write_response)

    async def handle_tcp(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        peer = writer.get_extra_info("peername")
        self.tcp_clients.add(writer)
        print(f"[TCP] VOFA+ connected: {peer}")
        try:
            while data := await reader.readline():
                command = data.decode("utf-8", errors="ignore").strip()
                if command:
                    await self.send_ble(command)
        except (ConnectionError, asyncio.CancelledError):
            pass
        finally:
            self.tcp_clients.discard(writer)
            writer.close()
            await writer.wait_closed()
            print(f"[TCP] VOFA+ disconnected: {peer}")

    async def handle_websocket(self, websocket) -> None:
        self.ws_clients.add(websocket)
        print("[WS] web console connected")
        await websocket.send(f"STATUS,{1 if self.client and self.client.is_connected else 0},bridge ready\n")
        try:
            async for message in websocket:
                if isinstance(message, bytes):
                    message = message.decode("utf-8", errors="ignore")
                for command in message.replace("\r", "").split("\n"):
                    if command.strip():
                        await self.send_ble(command)
        except Exception:
            pass
        finally:
            self.ws_clients.discard(websocket)
            if not self.ws_clients:
                await self.send_ble("STOP")
            print("[WS] web console disconnected")

    async def configure_characteristics(self) -> None:
        assert self.client is not None
        notify = self.client.services.get_characteristic(NOTIFY_UUID)
        if notify is None:
            raise RuntimeError("FFE1 notify characteristic not found")
        await self.client.start_notify(notify, self.on_notification)

        properties = set(notify.properties)
        if "write-without-response" in properties or "write" in properties:
            self.write_uuid = WRITE_UUID_FFE1
            self.write_response = "write-without-response" not in properties
        else:
            write = self.client.services.get_characteristic(WRITE_UUID_FFE2)
            if write is None:
                raise RuntimeError("neither FFE1 nor FFE2 is writable")
            self.write_uuid = WRITE_UUID_FFE2
            self.write_response = "write-without-response" not in set(write.properties)

    async def ble_loop(self) -> None:
        device = await self.choose_device()
        print(f"[BLE] selected {device.name or 'Unknown'}  {device.address}")
        while True:
            self.disconnected.clear()
            try:
                async with BleakClient(device, disconnected_callback=self.on_disconnected) as client:
                    self.client = client
                    await self.configure_characteristics()
                    print(f"[BLE] connected; notify=FFE1 write={self.write_uuid[4:8].upper()}")
                    await self.broadcast_web("STATUS,1,BLE connected\n")
                    await self.disconnected.wait()
            except Exception as error:
                print(f"[BLE] connection error: {error}")
            finally:
                self.client = None
                self.write_uuid = None
                await self.broadcast_web("STATUS,0,BLE disconnected\n")
            print("[BLE] reconnecting in 3 seconds...")
            await asyncio.sleep(3)

    async def run(self) -> None:
        tcp_server = await asyncio.start_server(self.handle_tcp, self.config.tcp_host, self.config.tcp_port)
        print(f"[TCP] VOFA+ endpoint: {self.config.tcp_host}:{self.config.tcp_port}")
        async with tcp_server, websockets.serve(self.handle_websocket, self.config.ws_host, self.config.ws_port):
            print(f"[WS] web console endpoint: ws://{self.config.ws_host}:{self.config.ws_port}")
            await asyncio.gather(tcp_server.serve_forever(), self.ble_loop())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="BLE FFE0 bridge for VOFA+ and C30D web console")
    parser.add_argument("--device", help="BLE device name or address; scans interactively when omitted")
    parser.add_argument("--tcp-host", default="127.0.0.1")
    parser.add_argument("--tcp-port", type=int, default=1347)
    parser.add_argument("--ws-host", default="127.0.0.1")
    parser.add_argument("--ws-port", type=int, default=8766)
    parser.add_argument("--scan-timeout", type=float, default=8.0)
    return parser.parse_args()


async def async_main() -> None:
    args = parse_args()
    config = BridgeConfig(args.device, args.tcp_host, args.tcp_port, args.ws_host, args.ws_port, args.scan_timeout)
    await BleVofaBridge(config).run()


if __name__ == "__main__":
    try:
        asyncio.run(async_main())
    except KeyboardInterrupt:
        print("\nBridge stopped")
