"""Preserve passive UART evidence before STOP/DISARM; never arms or drives."""
import argparse
import json
import time
from pathlib import Path
import serial

p=argparse.ArgumentParser()
p.add_argument('--port',required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
if a.output.exists():p.error('use a fresh output')
report={'port':a.port,'baud':115200,'chunks':[],'commands':[],
        'note':'Opening COM may reinitialize the PC UART; no module/MCU power cycle or reset.'}
buffer=b''
with serial.Serial(a.port,115200,timeout=.02,write_timeout=1) as port:
    def receive():
        global buffer
        data=port.read(max(1,min(port.in_waiting,4096)))
        if data:
            report['chunks'].append({'time':time.monotonic(),'hex':data.hex()})
            buffer+=data
        lines=[]
        while b'\n' in buffer:
            raw,buffer=buffer.split(b'\n',1)
            lines.append(raw.decode('ascii','backslashreplace').strip())
        return lines
    def command(text,prefix):
        entry={'command':text,'start':time.monotonic(),'lines':[]}
        report['commands'].append(entry)
        port.write(('\n'+text+'\n').encode('ascii'))
        while time.monotonic()-entry['start']<2:
            for line in receive():
                entry['lines'].append(line)
                if line==prefix or line.startswith(prefix+','):
                    entry['reply']=line;entry['rttMs']=1000*(time.monotonic()-entry['start']);return
        entry['timeout']=True
    try:
        start=time.monotonic();passive=[]
        while time.monotonic()-start<2:passive.extend(receive())
        report['passiveLines']=passive;print('passive complete lines:',len(passive),flush=True)
        for text,prefix in [('STOP','ACK,STOP'),('DISARM','ACK,DISARM'),('PING','PONG'),
                            ('VER','VER'),('UART','UART'),('LINK','LINK'),('STACK','STACK'),
                            ('SPEEDSTATUS','TEL'),('ANGLESTATUS','ATEL'),('PS2STATUS','PS2')]:
            command(text,prefix);print(json.dumps(report['commands'][-1]),flush=True)
    finally:
        command('STOP','ACK,STOP');command('DISARM','ACK,DISARM')
        a.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
