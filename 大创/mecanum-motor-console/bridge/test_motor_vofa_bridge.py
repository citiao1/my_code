from __future__ import annotations

import asyncio
import contextlib
import socket
import unittest
from dataclasses import dataclass
from unittest.mock import AsyncMock, patch

import websockets

import motor_vofa_bridge as bridge_module


@dataclass
class FakeDevice:
    name: str
    address: str


@dataclass
class FakeAdvertisement:
    service_uuids: list[str]
    local_name: str


class FakeCharacteristic:
    properties = ["notify", "write-without-response"]


class FakeServices:
    def get_characteristic(self, uuid: str):
        if uuid in (bridge_module.NOTIFY_UUID, bridge_module.WRITE_UUID_FFE2):
            return FakeCharacteristic()
        return None


class MissingFfe1Services:
    def get_characteristic(self, _uuid: str):
        return None


class ReconnectingClient:
    instances = []
    second_ready: asyncio.Event

    def __init__(self, device, disconnected_callback, **kwargs) -> None:
        self.device = device
        self.disconnected_callback = disconnected_callback
        self.options = kwargs
        self.is_connected = False
        self.services = MissingFfe1Services() if len(self.instances) == 1 else FakeServices()
        self.disconnect_count = 0
        self.__class__.instances.append(self)

    async def connect(self) -> None:
        self.is_connected = True

    async def disconnect(self) -> None:
        self.disconnect_count += 1
        self.is_connected = False

    async def start_notify(self, _characteristic, _callback) -> None:
        if len(self.instances) == 1:
            self.is_connected = False
            asyncio.get_running_loop().call_soon(self.disconnected_callback, self)
            return
        self.second_ready.set()

    async def write_gatt_char(self, *_args, **_kwargs) -> None:
        pass


class FailingWriteClient:
    is_connected = True

    async def write_gatt_char(self, *_args, **_kwargs) -> None:
        raise OSError("stale GATT handle")


class RecordingWriteClient:
    is_connected = True

    def __init__(self) -> None:
        self.writes = []

    async def write_gatt_char(self, uuid, payload, **kwargs) -> None:
        self.writes.append((uuid, bytes(payload), kwargs))


class ConfiguringClient:
    is_connected = True
    services = FakeServices()

    async def start_notify(self, _characteristic, _callback) -> None:
        pass


def unused_local_port() -> int:
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


class MotorVofaBridgeTests(unittest.IsolatedAsyncioTestCase):
    def make_config(self, tcp_port: int = 0, ws_port: int = 0):
        return bridge_module.BridgeConfig(
            "ble", None, 9600, None,
            "127.0.0.1", tcp_port, "127.0.0.1", ws_port, 0.01,
        )

    def test_extended_telemetry_maps_to_control_loops_firewater(self) -> None:
        line = (
            "TEL,1234,W,1,100,200,300,400,11,12,13,14,"
            "21,22,23,24,30,30,30,30,12540,1000,200,0,60,30,1,"
            "1,1,0,28750,30125,45678,30000,18400,500,100,0,1,"
            "45000,45678,-678,-2712,4000,300,80000,1,1"
        )

        payload = bridge_module.MotorVofaBridge.to_firewater(line)

        self.assertEqual(
            payload,
            b"1234,21,22,23,24,30,30,30,30,11,12,13,14,12540,30.125,30.0,18.4,45.678,28.75,45.0,45.678,-2.712,-0.678\r\n",
        )

    async def test_power_cycle_rescans_and_keeps_local_servers_alive(self) -> None:
        device_a = FakeDevice("BT05", "AA:BB:CC:DD:EE:FF")
        device_not_ready = FakeDevice("BT05", "AA:BB:CC:DD:EE:FF")
        device_missing_ffe1 = FakeDevice("BT05", "AA:BB:CC:DD:EE:FF")
        device_b = FakeDevice("BT05", "AA:BB:CC:DD:EE:FF")
        ready_advertisement = FakeAdvertisement([bridge_module.SERVICE_UUID], "BT05")
        booting_advertisement = FakeAdvertisement([], "BT05")
        scans = [
            {"first": (device_a, ready_advertisement)},
            {"booting": (device_not_ready, booting_advertisement)},
            {"missing-ffe1": (device_missing_ffe1, ready_advertisement)},
            {"recovered": (device_b, ready_advertisement)},
        ]
        tcp_port = unused_local_port()
        ws_port = unused_local_port()
        bridge = bridge_module.MotorVofaBridge(self.make_config(tcp_port, ws_port))
        ReconnectingClient.instances = []
        ReconnectingClient.second_ready = asyncio.Event()

        with (
            patch.object(bridge_module.BleakScanner, "discover", new=AsyncMock(side_effect=scans)) as discover,
            patch.object(bridge_module, "BleakClient", ReconnectingClient),
            patch.object(bridge_module, "BLE_RECONNECT_DELAY_SECONDS", 0.0),
            patch.object(bridge_module, "BLE_SCAN_RETRY_DELAY_SECONDS", 0.0),
        ):
            run_task = asyncio.create_task(bridge.run())
            try:
                await asyncio.wait_for(ReconnectingClient.second_ready.wait(), 2.0)
                self.assertFalse(run_task.done())

                reader, writer = await asyncio.open_connection("127.0.0.1", tcp_port)
                writer.close()
                await writer.wait_closed()
                del reader

                async with websockets.connect(f"ws://127.0.0.1:{ws_port}") as websocket:
                    status = await asyncio.wait_for(websocket.recv(), 1.0)
                    self.assertIn("STATUS,1", status)

                self.assertEqual(discover.await_count, 4)
                self.assertIs(ReconnectingClient.instances[0].device, device_a)
                self.assertEqual(ReconnectingClient.instances[0].disconnect_count, 1)
                self.assertIs(
                    ReconnectingClient.instances[1].device, device_missing_ffe1
                )
                self.assertEqual(ReconnectingClient.instances[1].disconnect_count, 1)
                self.assertIs(ReconnectingClient.instances[2].device, device_b)
                self.assertEqual(
                    ReconnectingClient.instances[2].options["timeout"],
                    bridge_module.BLE_CONNECT_TIMEOUT_SECONDS,
                )
            finally:
                run_task.cancel()
                with contextlib.suppress(asyncio.CancelledError):
                    await run_task

    async def test_ble_write_failure_requests_reconnect(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        bridge.ble_client = FailingWriteClient()
        bridge.ble_write_uuid = bridge_module.WRITE_UUID_FFE1
        bridge.hardware_connected = True
        bridge.broadcast_web = AsyncMock()

        await bridge.send_hardware("W")

        self.assertTrue(bridge.disconnected.is_set())
        self.assertFalse(bridge.hardware_connected)
        bridge.broadcast_web.assert_awaited_with(
            "STATUS,0,BLE link lost; reconnecting\n"
        )

    async def test_ble_command_is_written_in_16_byte_chunks(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        client = RecordingWriteClient()
        bridge.ble_client = client
        bridge.ble_write_uuid = bridge_module.WRITE_UUID_FFE1

        with patch.object(bridge_module, "BLE_WRITE_CHUNK_DELAY_SECONDS", 0.0):
            await bridge.send_hardware("YAWPID,20000,20000,20000")

        payloads = [write[1] for write in client.writes]
        self.assertEqual(b"".join(payloads), b"YAWPID,20000,20000,20000\n")
        self.assertEqual(len(payloads), 2)
        self.assertTrue(all(len(payload) <= 16 for payload in payloads))
        self.assertTrue(all(write[0] == bridge_module.WRITE_UUID_FFE1 for write in client.writes))

        client.writes.clear()
        await bridge.send_hardware("YAWPID,500,100,0")
        self.assertEqual(
            [write[1] for write in client.writes],
            [b"YAWPID,500,100,0", b"\n"],
        )

        client.writes.clear()
        await bridge.send_hardware("W")
        self.assertEqual([write[1] for write in client.writes], [b"W\n"])

    async def test_ble_configuration_prefers_ffe1_uart_write(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        bridge.ble_client = ConfiguringClient()

        await bridge.configure_ble()

        self.assertEqual(bridge.ble_write_uuid, bridge_module.WRITE_UUID_FFE1)


if __name__ == "__main__":
    unittest.main()
