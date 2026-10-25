"""R26 read-only SWD + one safe PING; no halt/reset/ARM/motion."""
import argparse
import json
import time
from pathlib import Path
import serial
from intelhex import IntelHex
from pyocd.core.helpers import ConnectHelper
from pyocd.core.session import Session
from pyocd.probe.shared_probe_proxy import SharedDebugProbeProxy

p=argparse.ArgumentParser();p.add_argument('--port',required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
if a.output.exists():p.error('use fresh output')
image=IntelHex(str(Path(__file__).resolve().parents[1]/'MDK-ARM/vgt6_module_test/vgt6_module_test.hex'))
report={'access':'read-only SWD attach + one PING; no halt/reset/motion','port':a.port}
probe=ConnectHelper.choose_probe(unique_id='puso_daplink_lab',blocking=False)
if probe is None:raise RuntimeError('probe absent')
with Session(SharedDebugProbeProxy(probe),target_override='stm32f407vgtx',
             pack=['D:/Keil5/ARM/PACK/Keil/STM32F4xx_DFP/3.1.1'],connect_mode='attach',frequency=100000,
             options={'cmsis_dap.deferred_transfers':False,'cmsis_dap.limit_packets':True,
                      'auto_unlock':False,'resume_on_disconnect':False}) as session:
    t=session.target
    assert t.read32(0xe0042000)&0xfff==0x413
    for addr in [0x08000000,0x08000004,0x08005934]:
        assert t.read32(addr)==sum(image[addr+i]<<(8*i) for i in range(4))
    def raw(addr,size):
        base=addr&~3
        data=b''.join(t.read32(x).to_bytes(4,'little') for x in range(base,(addr+size+3)&~3,4))
        return data[addr-base:addr-base+size]
    def snap():
        return {'tick':t.read32(0x200000d8),'rxBytes':t.read32(0x20001d18),'rxLines':t.read32(0x20001d1c),
                'rxOverflow':t.read32(0x20001d20),'txDropped':t.read32(0x20001d24),'uartErrors':t.read32(0x20001d28),
                'uartDetail':[t.read32(0x20001d2c+i*4) for i in range(9)],
                'rxDmaCr':t.read32(0x40026088),'rxDmaRemaining':t.read32(0x4002608c),
                'uartCr1':t.read32(0x4000440c),'uartCr3':t.read32(0x40004414),
                'gpioAMode':t.read32(0x40020000),'gpioAAfr':t.read32(0x40020020),'gpioAInput':t.read32(0x40020010)}
    report['before']=snap()
    with serial.Serial(a.port,115200,timeout=.02,write_timeout=1) as port:
        port.write(b'\nPING\n');report['pingSent']=time.time();received=bytearray()
        end=time.monotonic()+1.0
        while time.monotonic()<end:received.extend(port.read(max(1,min(port.in_waiting,4096))))
        report['rxHex']=received.hex();report['after']=snap()
        slots=[];base=0x20000b68
        for slot in range(16):
            length=int.from_bytes(raw(base+4490+slot*2,2),'little')
            if 0<length<256:slots.append(raw(base+393+slot*256,length).decode('ascii','backslashreplace'))
        report['txSlots']=slots
a.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report),flush=True)
