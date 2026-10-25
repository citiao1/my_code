"""Read-only R25 fault capture: no halt, reset, writes or serial commands."""
import argparse
import json
import re
import struct
import time
from pathlib import Path

from intelhex import IntelHex
from pyocd.core.helpers import ConnectHelper
from pyocd.core.session import Session
from pyocd.probe.shared_probe_proxy import SharedDebugProbeProxy

p=argparse.ArgumentParser()
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
if a.output.exists():p.error('use a fresh output file')
root=Path(__file__).resolve().parents[1]
map_text=(root/'MDK-ARM/vgt6_module_test/vgt6_module_test.map').read_text()
image=IntelHex(str(root/'MDK-ARM/vgt6_module_test/vgt6_module_test.hex'))
def symbol(name,obj):
    matches=re.findall(r'^\s*'+re.escape(name)+r'\s+(0x[0-9a-f]+)\s+Data\s+\d+\s+'+re.escape(obj)+r'\(',map_text,re.M)
    if len(matches)!=1:raise RuntimeError(f'ambiguous symbol {name}/{obj}')
    return int(matches[0],16)

report={'access':'read-only attach; no halt/reset/write/serial commands','snapshots':[]}
probe=ConnectHelper.choose_probe(unique_id='puso_daplink_lab',blocking=False)
if probe is None:raise RuntimeError('PUSO probe unavailable')
with Session(SharedDebugProbeProxy(probe),target_override='stm32f407vgtx',
             pack=['D:/Keil5/ARM/PACK/Keil/STM32F4xx_DFP/3.1.1'],
             connect_mode='attach',frequency=100000,
             options={'cmsis_dap.deferred_transfers':False,'cmsis_dap.limit_packets':True,
                      'auto_unlock':False,'resume_on_disconnect':False}) as session:
    t=session.target
    assert t.read32(0xe0042000)&0xfff==0x413 and t.read16(0x1fff7a22)==1024
    for addr in [0x08000000,0x08000004,0x08005934]:
        assert t.read32(addr)==sum(image[addr+i]<<(8*i) for i in range(4)), 'R25 identity mismatch'
    def raw(addr,size):
        aligned=addr&~3
        data=b''.join(t.read32(i).to_bytes(4,'little') for i in range(aligned,(addr+size+3)&~3,4))
        return data[addr-aligned:addr-aligned+size]
    for _ in range(3):
        speed=raw(symbol('speed_state','speed_control.o'),136)
        serial_addr=symbol('serial_port','serial_dma.o')
        ring=raw(serial_addr+320,8)
        tx=raw(serial_addr+4522,4)
        snap={'time':time.time(),'core':t.get_state().name,'tick':t.read32(symbol('uwTick','stm32f4xx_hal.o')),
              'faultRegisters':{'cfsr':t.read32(0xe000ed28),'hfsr':t.read32(0xe000ed2c)},
              'chassisBytes':list(raw(symbol('chassis_state','chassis_test.o'),8)),
              'angleMode':raw(symbol('state','attitude_control.o'),1)[0],
              'speedEnabled':speed[130],'targets':list(struct.unpack('<4i',speed[:16])),
              'outputPercent':list(struct.unpack('<4i',speed[48:64])),
              'serialStats':list(struct.unpack('<5I',raw(symbol('serial_stats','serial_dma.o'),20))),
              'uartStats':list(struct.unpack('<9I',raw(symbol('uart_stats','serial_dma.o'),36))),
              'uartBrr':t.read32(0x40004408),'dmaTxRemaining':t.read32(0x400260a4),
              'rxHeadTailFlags':ring.hex(),'txHeadTailBusyReady':list(tx)}
        tail=(tx[1]+15)%16
        length=int.from_bytes(raw(serial_addr+4490+2*tail,2),'little')
        if 0<length<256:
            snap['lastCompletedTxSlot']=raw(serial_addr+393+256*tail,length).hex()
        report['snapshots'].append(snap)
        print(json.dumps(snap),flush=True)
        time.sleep(.1)
a.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
