from __future__ import annotations

import argparse
import asyncio
from dataclasses import dataclass
from typing import Optional

import serial
from serial.tools import list_ports
import websockets
from bleak import BleakClient, BleakScanner


SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
NOTIFY_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"
WRITE_UUID_FFE1 = NOTIFY_UUID
WRITE_UUID_FFE2 = "0000ffe2-0000-1000-8000-00805f9b34fb"
BLE_CONNECT_TIMEOUT_SECONDS = 20.0
BLE_CONFIG_TIMEOUT_SECONDS = 10.0
BLE_DISCONNECT_TIMEOUT_SECONDS = 5.0
BLE_RECONNECT_DELAY_SECONDS = 3.0
BLE_SCAN_RETRY_DELAY_SECONDS = 2.0
# The WHEELTEC-IOS FFE0 module drops the link on a 17-byte write even though
# WinRT reports an ATT payload size of 20 bytes.
BLE_WRITE_CHUNK_SIZE = 16
BLE_WRITE_CHUNK_DELAY_SECONDS = 0.02


@dataclass
class BridgeConfig:
    transport: str
    serial_port: Optional[str]
    baud: int
    ble_device: Optional[str]
    tcp_host: str
    tcp_port: int
    ws_host: str
    ws_port: int
    scan_timeout: float


class MotorVofaBridge:
    def __init__(self, config: BridgeConfig) -> None:
        self.config = config
        self.serial_handle: Optional[serial.Serial] = None
        self.ble_client: Optional[BleakClient] = None
        self.ble_write_uuid: Optional[str] = None
        self.ble_write_response = False
        self.ble_notify_started = False
        self.ble_target_address: Optional[str] = None
        self.ble_target_name: Optional[str] = None
        self.hardware_connected = False
        self.rx_buffer = ""
        self.tcp_clients: set[asyncio.StreamWriter] = set()
        self.ws_clients: set = set()
        self.write_lock = asyncio.Lock()
        self.disconnected = asyncio.Event()
        self.loop = asyncio.get_running_loop()

    async def set_hardware_status(self, connected: bool, detail: str) -> None:
        self.hardware_connected = connected
        await self.broadcast_web(f"STATUS,{1 if connected else 0},{detail}\n")

    async def broadcast_web(self, message: str) -> None:
        failed = []
        for websocket in tuple(self.ws_clients):
            try:
                await websocket.send(message)
            except Exception:
                failed.append(websocket)
        for websocket in failed:
            self.ws_clients.discard(websocket)

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

    @staticmethod
    def to_firewater(line: str) -> Optional[bytes]:
        fields = line.split(",")

        if len(fields) >= 39 and fields[0] == "TEL":
            try:
                values = [
                    int(fields[1]),
                    int(fields[12]), int(fields[13]), int(fields[14]), int(fields[15]),
                    int(fields[16]), int(fields[17]), int(fields[18]), int(fields[19]),
                    int(fields[8]), int(fields[9]), int(fields[10]), int(fields[11]),
                    int(fields[20]),
                    int(fields[31]) / 1000.0,
                    int(fields[33]) / 1000.0,
                    int(fields[34]) / 1000.0,
                    int(fields[32]) / 1000.0,
                    int(fields[30]) / 1000.0,
                ]
                if len(fields) >= 48:
                    values.extend([
                        int(fields[39]) / 1000.0,
                        int(fields[40]) / 1000.0,
                        int(fields[42]) / 1000.0,
                        int(fields[41]) / 1000.0,
                    ])
            except ValueError:
                return None
            return (",".join(str(value) for value in values) + "\r\n").encode("ascii")

        if len(fields) == 27 and fields[0] == "TEL":
            try:
                values = [
                    int(fields[1]),
                    int(fields[12]), int(fields[13]), int(fields[14]), int(fields[15]),
                    int(fields[16]), int(fields[17]), int(fields[18]), int(fields[19]),
                    int(fields[8]), int(fields[9]), int(fields[10]), int(fields[11]),
                    int(fields[20]),
                ]
            except ValueError:
                return None
            return (",".join(str(value) for value in values) + "\r\n").encode("ascii")

        if len(fields) == 12 and fields[0] == "TEL":
            try:
                values = [
                    int(fields[1]),
                    int(fields[4]), int(fields[5]), int(fields[6]), int(fields[7]),
                    int(fields[8]), int(fields[9]), int(fields[10]), int(fields[11]),
                    int(fields[3]),
                ]
            except ValueError:
                return None
            return (",".join(str(value) for value in values) + "\r\n").encode("ascii")

        if len(fields) == 6 and fields[0] == "TEL":
            try:
                values = [int(fields[1]), int(fields[4]), int(fields[5])]
            except ValueError:
                return None
            return (",".join(str(value) for value in values) + "\r\n").encode("ascii")

        return None

    async def publish_line(self, line: str) -> None:
        await self.broadcast_web(f"{line}\n")
        firewater = self.to_firewater(line)
        if firewater is not None:
            await self.broadcast_tcp(firewater)

    def consume_bytes(self, payload: bytes) -> None:
        self.rx_buffer += payload.decode("utf-8", errors="ignore")
        lines = self.rx_buffer.replace("\r", "").split("\n")
        self.rx_buffer = lines.pop()
        for line in lines:
            line = line.strip()
            if line:
                asyncio.create_task(self.publish_line(line))

    async def send_hardware(self, command: str) -> None:
        command = command.strip()
        if not command:
            return

        payload = command.encode("utf-8") + b"\n"
        async with self.write_lock:
            if self.config.transport == "serial":
                if not self.serial_handle or not self.serial_handle.is_open:
                    await self.broadcast_web("STATUS,0,serial disconnected\n")
                    return
                await asyncio.to_thread(self.serial_handle.write, payload)
                return

            if not self.ble_client or not self.ble_client.is_connected or not self.ble_write_uuid:
                await self.broadcast_web("STATUS,0,BLE disconnected\n")
                return
            try:
                for offset in range(0, len(payload), BLE_WRITE_CHUNK_SIZE):
                    chunk = payload[offset:offset + BLE_WRITE_CHUNK_SIZE]
                    await self.ble_client.write_gatt_char(
                        self.ble_write_uuid, chunk, response=self.ble_write_response
                    )
                    if offset + BLE_WRITE_CHUNK_SIZE < len(payload):
                        await asyncio.sleep(BLE_WRITE_CHUNK_DELAY_SECONDS)
            except asyncio.CancelledError:
                raise
            except Exception as error:
                print(f"[BLE] write error: {error}")
                self.disconnected.set()
                await self.set_hardware_status(False, "BLE link lost; reconnecting")

    async def handle_tcp(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        peer = writer.get_extra_info("peername")
        self.tcp_clients.add(writer)
        print(f"[VOFA] connected: {peer}")
        try:
            while await reader.read(256):
                pass
        except (ConnectionError, asyncio.CancelledError):
            pass
        finally:
            self.tcp_clients.discard(writer)
            writer.close()
            await writer.wait_closed()
            print(f"[VOFA] disconnected: {peer}")

    async def handle_websocket(self, websocket) -> None:
        self.ws_clients.add(websocket)
        detail = f"{self.config.transport} connected" if self.hardware_connected else "hardware disconnected"
        await websocket.send(f"STATUS,{1 if self.hardware_connected else 0},{detail}\n")
        print("[WEB] console connected")
        try:
            async for message in websocket:
                if isinstance(message, bytes):
                    message = message.decode("utf-8", errors="ignore")
                for command in message.replace("\r", "").split("\n"):
                    if command.strip():
                        await self.send_hardware(command)
        except Exception:
            pass
        finally:
            self.ws_clients.discard(websocket)
            if not self.ws_clients:
                await self.send_hardware("STOP")
            print("[WEB] console disconnected")

    async def choose_serial_port(self) -> str:
        if self.config.serial_port:
            return self.config.serial_port

        while True:
            ports = list(list_ports.comports())
            preferred = [
                port for port in ports
                if any(name in (port.description or "").lower()
                       for name in ("ch910", "ch340", "usb serial", "virtual com"))
            ]
            if len(preferred) == 1:
                return preferred[0].device
            if len(ports) == 1:
                return ports[0].device
            if ports:
                print("[SERIAL] available ports:")
                for index, port in enumerate(ports, start=1):
                    print(f"  {index}. {port.device}  {port.description}")
                answer = await asyncio.to_thread(input, "Select port number: ")
                try:
                    return ports[int(answer) - 1].device
                except (ValueError, IndexError):
                    print("[SERIAL] invalid selection")
            else:
                print("[SERIAL] no COM port found; retrying in 2 seconds...")
                await asyncio.sleep(2)

    async def serial_loop(self) -> None:
        port = await self.choose_serial_port()
        while True:
            try:
                self.serial_handle = serial.Serial(
                    port=port,
                    baudrate=self.config.baud,
                    timeout=0.1,
                    write_timeout=1.0,
                )
                print(f"[SERIAL] connected: {port} @ {self.config.baud}")
                await self.set_hardware_status(True, f"serial {port} connected")
                while self.serial_handle.is_open:
                    payload = await asyncio.to_thread(self.serial_handle.read, 256)
                    if payload:
                        self.consume_bytes(payload)
            except (serial.SerialException, OSError) as error:
                print(f"[SERIAL] connection error: {error}")
            finally:
                if self.serial_handle:
                    try:
                        self.serial_handle.close()
                    except serial.SerialException:
                        pass
                self.serial_handle = None
                await self.set_hardware_status(False, "serial disconnected")
            print("[SERIAL] reconnecting in 2 seconds...")
            await asyncio.sleep(2)

    def ble_device_matches_target(self, device, name: str) -> bool:
        address = device.address.lower()
        name = name.lower()

        if self.ble_target_address or self.ble_target_name:
            return address == self.ble_target_address or name == self.ble_target_name
        if self.config.ble_device:
            return self.config.ble_device.lower() in {address, name}
        return True

    def remember_ble_device(self, device) -> None:
        self.ble_target_address = device.address.lower()
        self.ble_target_name = (device.name or "").lower() or None

    async def scan_ble_devices(self):
        print(f"[BLE] scanning for FFE0 devices ({self.config.scan_timeout:.0f}s)...")
        found = await BleakScanner.discover(timeout=self.config.scan_timeout, return_adv=True)
        matches = []
        for device, advertisement in found.values():
            services = {uuid.lower() for uuid in (advertisement.service_uuids or [])}
            name = device.name or advertisement.local_name or "Unknown"
            if SERVICE_UUID in services and self.ble_device_matches_target(device, name):
                matches.append(device)
        return matches

    async def choose_ble_device(self):
        while True:
            matches = await self.scan_ble_devices()
            if len(matches) == 1:
                selected = matches[0]
                self.remember_ble_device(selected)
                return selected
            if len(matches) > 1:
                print("[BLE] multiple devices found:")
                for index, device in enumerate(matches, start=1):
                    print(f"  {index}. {device.name or 'Unknown'}  {device.address}")
                answer = await asyncio.to_thread(input, "Select device number: ")
                try:
                    selected = matches[int(answer) - 1]
                    self.remember_ble_device(selected)
                    return selected
                except (ValueError, IndexError):
                    print("[BLE] invalid selection")
            else:
                print("[BLE] no ready FFE0 link; retrying in 2 seconds...")
                await asyncio.sleep(BLE_SCAN_RETRY_DELAY_SECONDS)

    def on_ble_disconnected(self, _client: BleakClient) -> None:
        self.loop.call_soon_threadsafe(self.mark_ble_disconnected, _client)

    def mark_ble_disconnected(self, client: BleakClient) -> None:
        if client is self.ble_client:
            self.disconnected.set()

    def on_ble_notification(self, _characteristic, data: bytearray) -> None:
        if not self.ble_notify_started:
            return
        self.loop.call_soon_threadsafe(self.consume_bytes, bytes(data))

    async def configure_ble(self) -> None:
        assert self.ble_client is not None
        notify = self.ble_client.services.get_characteristic(NOTIFY_UUID)
        if notify is None:
            raise RuntimeError("FFE1 notify characteristic not found")
        await self.ble_client.start_notify(notify, self.on_ble_notification)
        self.ble_notify_started = True

        properties = set(notify.properties)
        if "write-without-response" in properties or "write" in properties:
            self.ble_write_uuid = WRITE_UUID_FFE1
            self.ble_write_response = "write-without-response" not in properties
            return

        write = self.ble_client.services.get_characteristic(WRITE_UUID_FFE2)
        if write is not None:
            properties = set(write.properties)
            if "write-without-response" in properties or "write" in properties:
                self.ble_write_uuid = WRITE_UUID_FFE2
                self.ble_write_response = "write-without-response" not in properties
                return
        raise RuntimeError("neither FFE1 nor FFE2 is writable")

    def reset_ble_state(self) -> None:
        self.ble_client = None
        self.ble_write_uuid = None
        self.ble_write_response = False
        self.ble_notify_started = False
        self.rx_buffer = ""
        self.disconnected.clear()

    async def disconnect_ble(self) -> None:
        client = self.ble_client
        self.ble_notify_started = False
        if not client:
            return
        try:
            # Bleak also releases WinRT GATT objects here after physical link loss.
            await asyncio.wait_for(client.disconnect(), BLE_DISCONNECT_TIMEOUT_SECONDS)
        except Exception as error:
            print(f"[BLE] disconnect cleanup error: {error}")
        finally:
            self.ble_client = None
            self.ble_write_uuid = None
            self.ble_write_response = False

    async def run_ble_session(self, device) -> None:
        client = BleakClient(
            device,
            disconnected_callback=self.on_ble_disconnected,
            timeout=BLE_CONNECT_TIMEOUT_SECONDS,
        )
        self.ble_client = client
        await asyncio.wait_for(client.connect(), BLE_CONNECT_TIMEOUT_SECONDS)
        if not client.is_connected:
            raise ConnectionError("BLE client did not enter connected state")
        await asyncio.wait_for(self.configure_ble(), BLE_CONFIG_TIMEOUT_SECONDS)
        print(f"[BLE] connected; write={self.ble_write_uuid[4:8].upper()}")
        await self.set_hardware_status(True, "BLE connected")
        await self.disconnected.wait()

    async def ble_loop(self) -> None:
        while True:
            self.reset_ble_state()
            await self.set_hardware_status(False, "BLE reconnecting")
            try:
                device = await self.choose_ble_device()
                print(f"[BLE] selected: {device.name or 'Unknown'}  {device.address}")
                await self.run_ble_session(device)
            except asyncio.CancelledError:
                raise
            except Exception as error:
                print(f"[BLE] connection error: {error}")
            finally:
                await self.disconnect_ble()
                await self.set_hardware_status(False, "BLE disconnected")
            print(f"[BLE] reconnecting in {BLE_RECONNECT_DELAY_SECONDS:.0f} seconds...")
            await asyncio.sleep(BLE_RECONNECT_DELAY_SECONDS)

    async def run(self) -> None:
        tcp_server = await asyncio.start_server(
            self.handle_tcp, self.config.tcp_host, self.config.tcp_port
        )
        hardware_loop = self.serial_loop if self.config.transport == "serial" else self.ble_loop
        print(f"[VOFA] TCP endpoint: {self.config.tcp_host}:{self.config.tcp_port}")
        print(f"[WEB] WebSocket endpoint: ws://{self.config.ws_host}:{self.config.ws_port}")
        async with tcp_server, websockets.serve(
            self.handle_websocket, self.config.ws_host, self.config.ws_port
        ):
            await asyncio.gather(tcp_server.serve_forever(), hardware_loop())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="C30D web and VOFA bridge")
    parser.add_argument("--transport", choices=("serial", "ble"), required=True)
    parser.add_argument("--port", help="serial COM port, for example COM5")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--device", help="BLE device name or address")
    parser.add_argument("--tcp-host", default="127.0.0.1")
    parser.add_argument("--tcp-port", type=int, default=1347)
    parser.add_argument("--ws-host", default="127.0.0.1")
    parser.add_argument("--ws-port", type=int, default=8766)
    parser.add_argument("--scan-timeout", type=float, default=8.0)
    return parser.parse_args()


async def async_main() -> None:
    args = parse_args()
    config = BridgeConfig(
        args.transport, args.port, args.baud, args.device,
        args.tcp_host, args.tcp_port, args.ws_host, args.ws_port, args.scan_timeout,
    )
    await MotorVofaBridge(config).run()


if __name__ == "__main__":
    try:
        asyncio.run(async_main())
    except KeyboardInterrupt:
        print("\nBridge stopped.")
