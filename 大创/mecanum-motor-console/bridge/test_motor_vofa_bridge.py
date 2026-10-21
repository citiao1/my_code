from __future__ import annotations

import asyncio
import contextlib
import json
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
    async def test_r20_wheel_log_routes_full_rows_and_exact_batch_end(self):
        cls = bridge_module.MotorVofaBridge
        self.assertEqual(cls.expected_reply("WLOGINFO"), "WLOG,INFO")
        self.assertEqual(cls.expected_reply("WLOG,STATUS"), "WLOG,STATUS")
        self.assertEqual(cls.expected_reply("WLOG,READ,8,2"), "WLOG,END,8")
        self.assertEqual(cls.expected_reply("WSPD,A,30"), "ACK,WSPD")
        self.assertEqual(cls.expected_reply("WKEEP"), "ACK,WKEEP")
        bridge = cls(self.make_config())
        bridge.send_hardware = AsyncMock(return_value=True)
        bridge.broadcast_web = AsyncMock()
        task = asyncio.create_task(bridge.execute_command("WLOG,READ,8,2"))
        await asyncio.sleep(0)
        packet = b"WLOG,DATA,8,100,100000,1,1,0,40,350,316,15000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12000,500,1,1\r\n"
        for i in range(0,len(packet),7): bridge.consume_bytes(packet[i:i+7])
        bridge.consume_bytes(b"WLOG,END,0,2,10\r\n")
        await asyncio.sleep(0)
        self.assertFalse(task.done())
        bridge.consume_bytes(b"WLOG,END,8,2,10\r\n")
        self.assertEqual(await task,"WLOG,END,8,2,10")
        bridge.broadcast_web.assert_any_await(packet.decode().replace('\r',''))

    async def test_state_is_published_before_command_completes(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        entered, release = asyncio.Event(), asyncio.Event()

        async def broadcast(message):
            self.assertEqual(message, "STACK,4096,1992,2104,1\n")
            entered.set()
            await release.wait()

        async def write(command):
            bridge.consume_bytes(b"STACK,4096,1992,2104,1\r\n")
            return True

        bridge.broadcast_web = broadcast
        bridge.send_hardware = write
        task = asyncio.create_task(bridge.execute_command("STACK"))
        await entered.wait()
        self.assertFalse(task.done())
        release.set()
        self.assertEqual(await task, "STACK,4096,1992,2104,1")

    async def test_tracked_drive_requires_its_own_mcu_ack(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        started = asyncio.Event()

        async def write(command):
            self.assertEqual(command, "TRACKMOVE,90,25.50,-17.25")
            started.set()
            return True

        bridge.send_hardware = write
        task = asyncio.create_task(bridge.execute_command("TRACKMOVE,90,25.50,-17.25"))
        await started.wait()
        bridge.consume_bytes(b"ACK,HEADMOVE\r\n")
        self.assertFalse(task.done())
        bridge.consume_bytes(b"ACK,TRACKMOVE\r\n")
        self.assertEqual(await task, "ACK,TRACKMOVE")

    def make_config(self, tcp_port: int = 0, ws_port: int = 0):
        return bridge_module.BridgeConfig(
            "ble", None, 9600, None,
            "127.0.0.1", tcp_port, "127.0.0.1", ws_port, 0.01,
        )

    def test_old_vehicle_packet_is_not_misinterpreted_as_bm50(self) -> None:
        line = (
            "TEL,1234,W,1,100,200,300,400,11,12,13,14,"
            "21,22,23,24,30,30,30,30,12540,1000,200,0,60,30,1,"
            "1,1,0,28750,30125,45678,30000,18400,500,100,0,1,"
            "45000,45678,-678,-2712,4000,300,80000,1,1"
        )

        payload = bridge_module.MotorVofaBridge.to_firewater(line)

        self.assertIsNone(payload)

    def test_compact_telemetry_has_fixed_firewater_channels(self) -> None:
        fields = ["TEL", "1234", "0", "1", "30", "-40", "50", "60",
                  "11", "-12", "13", "14", "215", "-225", "235", "245",
                  "30", "-40", "50", "60", "12540", "0", "0", "1", "0", "0", "1"]
        self.assertEqual(
            bridge_module.MotorVofaBridge.to_firewater(",".join(fields)),
            b"vehicle:1234,21.5,-22.5,23.5,24.5,30,-40,50,60,11,-12,13,14,12540\r\n",
        )
        self.assertIsNone(bridge_module.MotorVofaBridge.to_firewater("TEL,1,2"))
        fields[12] = "bad"
        self.assertIsNone(bridge_module.MotorVofaBridge.to_firewater(",".join(fields)))

    def test_speed_limit_query_matches_readback_response(self) -> None:
        self.assertEqual(
            bridge_module.MotorVofaBridge.expected_reply("SPEEDLIMIT"),
            "SPEEDLIMIT",
        )

    async def test_tof_query_stream_acks_and_fragmented_frames(self):
        cls = bridge_module.MotorVofaBridge
        self.assertEqual(cls.expected_reply("TOF"), "TOF")
        self.assertEqual(cls.expected_reply("TOF,1"), "ACK,TOF,1")
        self.assertEqual(cls.expected_reply("TOF,0"), "ACK,TOF,0")
        bridge = cls(self.make_config())
        bridge.broadcast_web = AsyncMock()
        bridge.broadcast_tcp = AsyncMock()
        bridge.send_hardware = AsyncMock(return_value=True)
        task = asyncio.create_task(bridge.execute_command("TOF,1"))
        await asyncio.sleep(0)
        bridge.consume_bytes(b"TOF,100,1,1,1234,9,0,0\r\n")
        await asyncio.sleep(0)
        self.assertFalse(task.done())  # A data frame cannot acknowledge stream enable.
        bridge.consume_bytes(b"ACK,TOF,1\r\n")
        self.assertEqual(await task, "ACK,TOF,1")
        packet = b"TOF,200,1,0,-1,4,0,0\r\n"
        for offset in range(0, len(packet), 5):
            bridge.consume_bytes(packet[offset:offset+5])
        await asyncio.sleep(0)
        bridge.broadcast_web.assert_any_await("TOF,200,1,0,-1,4,0,0\n")
        bridge.broadcast_tcp.assert_not_awaited()

    def test_angular_channels_and_query_routing(self):
        self.assertEqual(bridge_module.MotorVofaBridge.to_firewater(
            "ATEL,1234,2,1,15000,14250,5400,170000,-175000,15000,15000,0,1,12000"),
            b"attitude:1234.0,15.0,14.25,5.4,170.0,-175.0,15.0,15.0,0.0,1.0,12.0\r\n")
        self.assertIsNone(bridge_module.MotorVofaBridge.to_firewater(
            "ATEL,1234,2,1,NaN,14250,5400,170000,-175000,15000,15000,0,1,12000"))
        for command,reply in (("YAWGET","YAWPID"),("HEADGET","HEADPID"),
                              ("CAPS","CAPS"),("ANGLESTATUS","ATEL"),
                              ("PS2STATUS","PS2"),
                              ("PS2RAW","PS2RAW"),
                              ("HLOG,STATUS","HLOG,STATUS"),
                              ("HLOG,START","HLOG,START"),
                              ("HLOG,STOP","HLOG,STOP"),
                              ("HLOG,CLEAR","HLOG,CLEAR"),
                              ("HLOG,READ,0,8","HLOG,END,0")):
            self.assertEqual(bridge_module.MotorVofaBridge.expected_reply(command),reply)

    async def test_r17_hlog_fragmented_rows_forward_unchanged_and_need_matching_ack(self):
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        bridge.broadcast_web = AsyncMock()
        bridge.broadcast_tcp = AsyncMock()
        bridge.send_hardware = AsyncMock(return_value=True)
        task = asyncio.create_task(bridge.execute_command('HLOG,STOP'))
        await asyncio.sleep(0)
        bridge.consume_bytes(b'HLOG,STATUS,1,2,1024,0,2\r\n')
        await asyncio.sleep(0)
        self.assertFalse(task.done())
        bridge.consume_bytes(b'HLOG,STOP,OK\r\n')
        self.assertEqual(await task, 'HLOG,STOP,OK')
        task = asyncio.create_task(bridge.execute_command('HLOG,READ,0,2'))
        await asyncio.sleep(0)
        bridge.consume_bytes(b'HLOG,END,8,2,10\r\n')
        await asyncio.sleep(0)
        self.assertFalse(task.done())  # A delayed END cannot acknowledge another offset.
        rows = ['HLOG,DATA,0,100,0,0,0,15000,12000,0,5000,0,0,0,1',
                'HLOG,DATA,1,120,15000,12000,3000,8000,7600,40000,12000,0,20,0,2,120123']
        payload = ('\r\n'.join(rows) + '\r\n').encode('ascii')
        for offset in range(0, len(payload), 13):
            bridge.consume_bytes(payload[offset:offset+13])
        await asyncio.sleep(0)
        self.assertFalse(task.done())
        bridge.consume_bytes(b'HLOG,END,0,2,2\r\n')
        self.assertEqual(await task, 'HLOG,END,0,2,2')
        await asyncio.sleep(0)
        for row in rows:
            bridge.broadcast_web.assert_any_await(row+'\n')
        bridge.broadcast_tcp.assert_not_awaited()  # Downloads are not live VOFA frames.

    async def test_power_cycle_rescans_and_keeps_local_servers_alive(self) -> None:
        device_a = FakeDevice("BT05", "AA:BB:CC:DD:EE:FF")
        device_not_ready = FakeDevice("BT05", "AA:BB:CC:DD:EE:FF")
        device_missing_ffe1 = FakeDevice("BT05", "AA:BB:CC:DD:EE:FF")
        ready_advertisement = FakeAdvertisement([bridge_module.SERVICE_UUID], "BT05")
        booting_advertisement = FakeAdvertisement([], "BT05")
        scans = [
            {"first": (device_a, ready_advertisement)},
            {"booting": (device_not_ready, booting_advertisement)},
            {"missing-ffe1": (device_missing_ffe1, ready_advertisement)},
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
                    self.assertEqual(await websocket.recv(), "BRIDGE,BM50_ACK_R3\n")
                    status = await asyncio.wait_for(websocket.recv(), 1.0)
                    self.assertIn("STATUS,1", status)

                # A named UART module is a valid candidate even while its
                # advertisement temporarily omits FFE0 after power-up.
                self.assertEqual(discover.await_count, 3)
                self.assertIs(ReconnectingClient.instances[0].device, device_a)
                self.assertEqual(ReconnectingClient.instances[0].disconnect_count, 1)
                self.assertIs(
                    ReconnectingClient.instances[1].device, device_not_ready
                )
                self.assertEqual(ReconnectingClient.instances[1].disconnect_count, 1)
                self.assertIs(ReconnectingClient.instances[2].device, device_missing_ffe1)
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
        self.assertEqual(b"".join(payloads), b"\nYAWPID,20000,20000,20000\n")
        self.assertEqual(len(payloads), 2)
        self.assertTrue(all(len(payload) <= 16 for payload in payloads))
        self.assertTrue(all(write[0] == bridge_module.WRITE_UUID_FFE1 for write in client.writes))

        client.writes.clear()
        await bridge.send_hardware("YAWPID,500,100,0")
        self.assertEqual(b"".join(write[1] for write in client.writes), b"\nYAWPID,500,100,0\n")
        self.assertTrue(all(len(write[1]) <= 16 for write in client.writes))

        client.writes.clear()
        await bridge.send_hardware("W")
        self.assertEqual([write[1] for write in client.writes], [b"\nW\n"])

    async def test_ble_configuration_prefers_ffe1_uart_write(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        bridge.ble_client = ConfiguringClient()

        await bridge.configure_ble()

        self.assertEqual(bridge.ble_write_uuid, bridge_module.WRITE_UUID_FFE1)

    async def test_commands_wait_for_fragmented_ack_before_next_write(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        writes = []
        first_written = asyncio.Event()
        second_written = asyncio.Event()

        async def write(command):
            writes.append(command)
            (first_written if len(writes) == 1 else second_written).set()
            return True

        bridge.send_hardware = write
        first = asyncio.create_task(bridge.execute_command("PID,A,0.8,0.1,0"))
        await first_written.wait()
        second = asyncio.create_task(bridge.execute_command("SPD,A,30"))
        await asyncio.sleep(0)
        self.assertEqual(writes, ["PID,A,0.8,0.1,0"])
        bridge.consume_bytes(b"ACK,PI")
        self.assertFalse(first.done())
        bridge.consume_bytes(b"D\r\n")
        self.assertEqual(await first, "ACK,PID")
        await second_written.wait()
        bridge.consume_bytes(b"ACK,SPD\r\n")
        self.assertEqual(await second, "ACK,SPD")
        self.assertEqual(writes, ["PID,A,0.8,0.1,0", "SPD,A,30"])

    async def test_stop_cancels_inflight_and_queued_targets(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        writes = []
        started = asyncio.Event()

        async def write(command):
            writes.append(command)
            started.set()
            if command == "STOP":
                bridge.consume_bytes(b"ACK,STOP\r\n")
            return True

        bridge.send_hardware = write
        first = asyncio.create_task(bridge.execute_command("PID,A,0.8,0.1,0"))
        await started.wait()
        queued = asyncio.create_task(bridge.execute_command("SPD,A,30"))
        await asyncio.sleep(0)
        stop = asyncio.create_task(bridge.execute_command("STOP"))
        results = await asyncio.gather(first, queued, stop, return_exceptions=True)
        self.assertIsInstance(results[0], ConnectionError)
        self.assertIsInstance(results[1], RuntimeError)
        self.assertEqual(results[2], "ACK,STOP")
        self.assertEqual(writes, ["PID,A,0.8,0.1,0", "STOP"])

    async def test_normal_stop_cancels_keep_without_disconnect_or_second_stop(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        bridge.hardware_connected = True
        writes = []
        keep_written = asyncio.Event()

        async def write(command):
            writes.append(command)
            if command == "KEEP":
                keep_written.set()  # ACK is delayed when STOP arrives.
            else:
                bridge.consume_bytes(("ACK," + command.split(",")[0] + "\r\n").encode())
            return True

        bridge.send_hardware = write
        keep = asyncio.create_task(bridge.execute_command("KEEP"))
        await keep_written.wait()
        stop = asyncio.create_task(bridge.execute_command("STOP"))
        results = await asyncio.gather(keep, stop, return_exceptions=True)
        self.assertIsInstance(results[0], ConnectionError)
        self.assertIn("cancelled by STOP", str(results[0]))
        self.assertEqual(results[1], "ACK,STOP")
        self.assertTrue(bridge.hardware_connected)
        self.assertFalse(bridge.disconnected.is_set())
        self.assertEqual(await bridge.execute_command("HEADMOVE,90,0,0"), "ACK,HEADMOVE")
        self.assertEqual(writes, ["KEEP", "STOP", "HEADMOVE,90,0,0"])

    async def test_timeout_does_not_retry_and_next_command_recovers(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        bridge.send_hardware = AsyncMock(return_value=True)
        with patch.object(bridge_module, "COMMAND_TIMEOUT_SECONDS", 0.01):
            with self.assertRaisesRegex(TimeoutError, "MCU reply timeout"):
                await bridge.execute_command("SPEED,1")
        self.assertEqual(bridge.send_hardware.await_count, 1)
        self.assertIsNone(bridge.pending_reply)

        async def write(command):
            bridge.consume_bytes(b"ACK,ARM,WHEELS_OFF_GROUND\r\n")
            return True

        bridge.send_hardware = write
        self.assertEqual(await bridge.execute_command("ARM"), "ACK,ARM,WHEELS_OFF_GROUND")

    async def test_error_echo_is_reported_not_success(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())

        async def write(command):
            bridge.consume_bytes(b"ERR,UNKNOWN_COMMAND,SPE\r\n")
            return True

        bridge.send_hardware = write
        with self.assertRaisesRegex(RuntimeError, "UNKNOWN_COMMAND,SPE"):
            await bridge.execute_command("SPEED,1")

    async def test_speed_limit_ack_cannot_complete_speed_enable(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        started = asyncio.Event()

        async def write(command):
            started.set()
            return True

        bridge.send_hardware = write
        task = asyncio.create_task(bridge.execute_command("SPEED,1"))
        await started.wait()
        bridge.consume_bytes(b"ACK,SPEED,LIMIT\r\n")
        self.assertFalse(task.done())
        bridge.consume_bytes(b"ACK,SPEED,1\r\n")
        self.assertEqual(await task, "ACK,SPEED,1")

    async def test_web_json_result_and_vofa_command_forwarding(self) -> None:
        bridge = bridge_module.MotorVofaBridge(self.make_config())
        bridge.hardware_connected = True

        async def write(command):
            reply = "PONG" if command == "PING" else "ACK,STOP"
            bridge.consume_bytes((reply + "\r\n").encode("ascii"))
            return True

        bridge.send_hardware = AsyncMock(side_effect=write)
        async with websockets.serve(bridge.handle_websocket, "127.0.0.1", 0) as server:
            port = server.sockets[0].getsockname()[1]
            async with websockets.connect(f"ws://127.0.0.1:{port}") as websocket:
                self.assertEqual(await websocket.recv(), "BRIDGE,BM50_ACK_R3\n")
                self.assertIn("STATUS,1", await websocket.recv())
                await websocket.send(json.dumps({"type": "command", "id": 7, "command": "PING"}))
                messages = [await asyncio.wait_for(websocket.recv(), 1.0) for _ in range(2)]
                result = next(json.loads(message) for message in messages if message.startswith("{"))
                self.assertEqual(result, {"type": "result", "id": 7, "ok": True, "reply": "PONG"})

        async with await asyncio.start_server(bridge.handle_tcp, "127.0.0.1", 0) as server:
            port = server.sockets[0].getsockname()[1]
            reader, writer = await asyncio.open_connection("127.0.0.1", port)
            try:
                await asyncio.sleep(0)
                writer.write(b"PING\n")
                await writer.drain()
                # A sample must traverse the actual TCP output path.
                sample = b"vehicle:1,2,3\r\n"
                await bridge.broadcast_tcp(sample)
                self.assertEqual(await asyncio.wait_for(reader.readline(), 1.0), sample)
                for _ in range(10):
                    if bridge.send_hardware.await_count >= 3:
                        break
                    await asyncio.sleep(0.01)
                self.assertGreaterEqual(bridge.send_hardware.await_count, 3)
                self.assertEqual(bridge.send_hardware.await_args.args, ("PING",))
            finally:
                writer.close()
                await writer.wait_closed()


if __name__ == "__main__":
    unittest.main()
