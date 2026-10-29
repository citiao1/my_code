"""Stopped-only FP/FireWater validation. Never ARM, START or move motors."""
import argparse
import json
import time
from pathlib import Path

import serial

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--port', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
if a.output.exists():
    p.error('use a fresh output path')
report = {'port': a.port, 'baud': 115200, 'commands': [], 'frames': []}
buffer = b''
with serial.Serial(a.port, 115200, timeout=.02, write_timeout=1) as port:
    def receive():
        global buffer
        buffer += port.read(max(1, min(port.in_waiting, 4096)))
        lines = []
        while b'\n' in buffer:
            raw, buffer = buffer.split(b'\n', 1)
            line = raw.decode('ascii', 'strict').strip()
            lines.append(line)
            if line.startswith('follow:'):
                values = list(map(int, line[7:].split(',')))
                assert len(values) == 8 and values[0] in (500, 600)
                assert -80 <= values[4] <= 80 and values[5] in (0, 1)
                assert values[6] == 0, 'must remain stopped; no tracking trial authorized'
                report['frames'].append({'hostTime': time.monotonic(), 'values': values})
        return lines

    def command(text, prefix):
        item = {'command': text, 'lines': []}
        report['commands'].append(item)
        start = time.monotonic()
        port.write(('\n' + text + '\n').encode('ascii'))
        while time.monotonic() - start < 2:
            for line in receive():
                item['lines'].append(line)
                if line == prefix or line.startswith(prefix + ','):
                    item.update(reply=line, rttMs=round(1000 * (time.monotonic()-start), 2))
                    print(json.dumps(item), flush=True)
                    return line
        raise TimeoutError(text)

    try:
        command('STOP', 'ACK,STOP')
        command('DISARM', 'ACK,DISARM')
        assert command('FD', 'FD') == 'FD,500'
        command('FD,99', 'ERR,FD,RANGE_100_2000_MM')
        command('FD,4294967796', 'ERR,FD,RANGE_100_2000_MM')
        assert command('FD', 'FD') == 'FD,500'
        assert command('FP', 'FP') == 'FP,0.0600,0.1800'
        assert command('FP,0.06,0.18', 'FP') == 'FP,0.0600,0.1800'
        assert command('FP,4,10', 'FP') == 'FP,4.0000,10.0000'
        assert command('FP', 'FP') == 'FP,4.0000,10.0000'
        command('FP,-1,10', 'ERR,FP,USE_NONNEGATIVE_FINITE')
        assert command('FP,0.06,0.18', 'FP') == 'FP,0.0600,0.1800'
        assert command('FP', 'FP') == 'FP,0.0600,0.1800'
        command('UART', 'UART')
        assert command('FD,600', 'FD') == 'FD,600'
        command('FDBG,1', 'ACK,FDBG,1')
        start = time.monotonic()
        while time.monotonic() - start < 3:
            receive()
        assert len(report['frames']) >= 8, 'insufficient FireWater samples'
        for _ in range(5):
            command('PING', 'PONG')
            until = time.monotonic() + .15
            while time.monotonic() < until:
                receive()
        command('FDBG,0', 'ACK,FDBG,0')
        assert command('FD,500', 'FD') == 'FD,500'
        report['streamSummary'] = {
            'count': len(report['frames']),
            'first': report['frames'][0]['values'],
            'last': report['frames'][-1]['values'],
        }
        command('UART', 'UART')
        assert command('STACK', 'STACK').split(',')[-1] == '1'
        angle = command('ANGLESTATUS', 'ATEL').split(',')
        assert angle[2] == angle[12] == '0'
        assert command('PS2STATUS', 'PS2').split(',')[2] == '0'
        print(json.dumps(report['streamSummary']), flush=True)
    finally:
        for text, prefix in [('FDBG,0', 'ACK,FDBG,0'), ('STOP', 'ACK,STOP'),
                             ('DISARM', 'ACK,DISARM'), ('FD,500', 'FD,500')]:
            try:
                command(text, prefix)
            except Exception as error:
                report.setdefault('cleanupErrors', []).append(repr(error))
        a.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
