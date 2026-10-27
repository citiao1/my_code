import asyncio
import json
import unittest
from unittest.mock import AsyncMock, patch

from session_transport import SerialEndpoint


class SerialEndpointTests(unittest.IsolatedAsyncioTestCase):
    async def test_raw_log_is_before_end_and_result(self):
        endpoint = SerialEndpoint('COM9')
        async def write(_):
            endpoint.engine.consume_bytes(b'WLOG,DATA,0,raw\r\nWLOG,END,0,1,1\r\n')
            return True
        endpoint.engine.send_hardware = write
        await endpoint.send(json.dumps({'type': 'command', 'id': 1, 'command': 'WLOG,READ,0,1'}))
        self.assertEqual(await endpoint.recv(), 'WLOG,DATA,0,raw\n')
        self.assertEqual(await endpoint.recv(), 'WLOG,END,0,1,1\n')
        self.assertEqual(json.loads(await endpoint.recv())['reply'], 'WLOG,END,0,1,1')
        await endpoint.close()

    async def test_stop_preempts_waiting_command_without_replay(self):
        endpoint = SerialEndpoint('COM9')
        sent = []
        entered = asyncio.Event()
        async def write(command):
            sent.append(command)
            if command == 'STOP':
                endpoint.engine.consume_bytes(b'ACK,STOP\r\n')
            entered.set()
            return True
        endpoint.engine.send_hardware = write
        await endpoint.send(json.dumps({'type': 'command', 'id': 1, 'command': 'PING'}))
        await entered.wait()
        await endpoint.send(json.dumps({'type': 'command', 'id': 2, 'command': 'STOP'}))
        results = []
        while len(results) < 2:
            text = await asyncio.wait_for(endpoint.recv(), 1)
            if text.startswith('{'):
                results.append(json.loads(text))
        self.assertEqual(sent, ['PING', 'STOP'])
        self.assertFalse(next(r for r in results if r['id'] == 1)['ok'])
        self.assertTrue(next(r for r in results if r['id'] == 2)['ok'])
        await endpoint.close()

    async def test_lease_completion_is_output_not_another_request(self):
        endpoint = SerialEndpoint('COM9')
        sent = []
        async def write(command):
            sent.append(command)
            endpoint.engine.consume_bytes(b'ACK,DRV,0,0,0\r\n')
            return True
        endpoint.engine.send_hardware = write
        await endpoint.send(json.dumps({'type': 'drive_lease', 'id': 7,
                                        'command': 'DRV,0,0,0', 'duration_ms': 20}))
        event = None
        while event is None:
            text = await asyncio.wait_for(endpoint.recv(), 1)
            if text.startswith('{'):
                item = json.loads(text)
                if item.get('event'):
                    event = item
        self.assertEqual(event['event'], 'complete')
        self.assertEqual(sent, ['DRV,0,0,0', 'DRV,0,0,0'])
        self.assertFalse(endpoint.requests)
        await endpoint.close()

    async def test_open_failure_is_not_retried(self):
        endpoint = SerialEndpoint('COM9')
        with patch.object(endpoint.module.serial, 'Serial', side_effect=OSError('port busy')) as open_port:
            with self.assertRaisesRegex(OSError, 'port busy'):
                await endpoint.__aenter__()
        self.assertEqual(open_port.call_count, 1)
        self.assertTrue(endpoint.closed)

    async def test_overflow_fails_instead_of_dropping_log(self):
        endpoint = SerialEndpoint('COM9')
        endpoint.messages = asyncio.Queue(maxsize=1)
        await endpoint._emit('first')
        with self.assertRaisesRegex(RuntimeError, 'incomplete'):
            await endpoint._emit('second')
        with self.assertRaises(ConnectionError):
            await endpoint.recv()
        await endpoint.close()


if __name__ == '__main__':
    unittest.main()
