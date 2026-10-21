import asyncio, copy, json, math, tempfile, unittest
from pathlib import Path
from unittest.mock import patch
from wheel_log import HEADER, VERSION, CAPS, INFO, parse, validate, score
from wheel_tuner import WheelSession, parser, run

def records(target=40,tau=.25):
    rows=[]
    for i in range(501):
        t=i*.01;ref=target if i>=200 else 0
        actual=ref*(1-math.exp(-(t-2)/tau)) if ref else 0
        v=[i,i*10+1000,i*10000+1000000,1,1,0]
        v += [ref,round(actual*10),round(actual*10),int(ref*.4)*1000]+[0]*12
        v += [0,0,12000,500,1,1]
        rows.append(parse('WLOG,DATA,'+','.join(map(str,v))))
    return rows

class FakeWS:
    def __init__(self):
        self.queue=asyncio.Queue();self.queue.put_nowait('BRIDGE,BM50_ACK_R3\n')
        self.commands=[];self.gains=[v for ki in (.0991,.0998,.0998,.1008) for v in (1.2,ki,0.)]
        self.data=records()[:10];self.reads=0;self.fault=None
    async def recv(self): return await self.queue.get()
    async def send(self,text):
        p=json.loads(text);cmd=p['command'];self.commands.append(cmd)
        replies={'VER':VERSION,'CAPS':CAPS,'WLOGINFO':INFO,
                 'ANGLESTATUS':'ATEL,1,0,1,0,0,0,0,0,0,0,0,0,12000',
                 'PS2STATUS':'PS2,1,0,0x73,0x0000,128,128,128,128,30,0,1',
                 'STACK':'STACK,4096,2048,2048,1','UART':'UART,9600,42000000,4375,0x00,0,0,0,0,0',
                 'LINK':'LINK,12,1,0,0,0,0','SPEEDLIMIT':'SPEEDLIMIT,160',
                 'YAWGET':'YAWPID,0.1815,1.8062,60,80,20', 'HEADGET':'HEADPID,6.0000,1.2500,80,20,D_MEAS',
                 'HLOG,STATUS':'HLOG,STATUS,0,0,1024,0,0'}
        if cmd.startswith('PID,'):
            fields=cmd.split(',');i='ABCD'.index(fields[1])*3
            self.gains[i:i+3]=map(float,fields[2:]);reply='ACK,PID'
        elif cmd=='PIDGET': reply='PID,'+','.join(f'{v:.4f}' for v in self.gains)
        elif cmd=='WLOG,STATUS': reply=f'WLOG,STATUS,0,{len(self.data)},1024,0,{len(self.data)}'
        elif cmd.startswith('WLOG,READ,'):
            _,_,offset,count=cmd.split(',');offset,count=int(offset),int(count);self.reads+=1
            for row in self.data[offset:offset+count]:
                if self.fault in ('retry','conflict') and self.reads==1 and row['sequence']==1: continue
                row=dict(row)
                if self.fault=='conflict' and self.reads==2: row['A_target_rpm']+=1
                self.queue.put_nowait('WLOG,DATA,'+','.join(str(row[k]) for k in HEADER)+'\n')
            reply=f'WLOG,END,{offset},{count},{len(self.data)}'
            if self.fault=='end':reply=f'WLOG,END,{offset},{count-1},{len(self.data)}'
        else: reply=replies.get(cmd,'ACK,'+cmd.split(',')[0])
        self.queue.put_nowait(json.dumps({'type':'result','id':p['id'],'ok':True,'reply':reply}))

class Context:
    def __init__(self,ws):self.ws=ws
    async def __aenter__(self):return self.ws
    async def __aexit__(self,*args):pass

class WheelQualityTests(unittest.TestCase):
    def test_quality_and_score_both_signs(self):
        for target in (40,-40,20):
            samples,result=score(records(target), 'A',target)
            self.assertEqual(len(samples),501)
            self.assertTrue(result.resolved_transient)
            self.assertLess(result.overshoot,.01)
    def test_reject_missing_cadence_phase_overflow_flags_and_bad_quality(self):
        for change in ('missing','cadence','phase','mode','isolation','limited','baseline','fast','duration','battery','reversed','final'):
            r=records(tau=.01 if change=='fast' else .25)
            if change=='missing': del r[20]
            elif change=='cadence':r[20]['tick_us']+=9000
            elif change=='phase':r[20]['tick_ms']+=2
            elif change=='mode':r[250]['angle_mode']=1
            elif change=='isolation':r[250]['B_target_rpm']=10
            elif change=='limited':r[250]['tof_limited']=1
            elif change=='baseline':r[150]['A_filtered_rpm_x10']=500
            elif change=='duration':r=r[:250]
            elif change=='battery':r[250]['battery_mv']=9000
            elif change=='reversed':
                for row in r:row['A_filtered_rpm_x10']*=-1
            elif change=='final':
                for row in r[400:]:row['A_filtered_rpm_x10']=200
            with self.assertRaises((ValueError,RuntimeError),msg=change):score(r,'A',40)
    def test_timestamp_wrap(self):
        r=records()[:2]
        for i,row in enumerate(r):
            absolute=4294967200+i*10000
            row['tick_us']=absolute&0xffffffff;row['tick_ms']=absolute//1000
        self.assertEqual(len(validate(r)),2)

class WheelSessionTests(unittest.IsolatedAsyncioTestCase):
    async def test_capture_command_order_and_watchdog_use_without_real_time_or_hardware(self):
        ws=FakeWS();await ws.recv();s=WheelSession(ws,'A',Path('.'))
        clock=[0.]
        async def advance(seconds):clock[0]+=seconds
        with patch('wheel_tuner.time.monotonic',side_effect=lambda:clock[0]),patch('wheel_tuner.asyncio.sleep',advance):
            rows=await s.collect(40,2,3)
        self.assertEqual(len(rows),10)
        start=ws.commands.index('WLOG,START')
        self.assertLess(start,ws.commands.index('ARM'))
        self.assertGreater(ws.commands.count('WKEEP'),30)
        stop=ws.commands.index('STOP',start)
        self.assertLess(stop,ws.commands.index('WLOG,STOP'))
        self.assertLess(ws.commands.index('WLOG,STOP'),ws.commands.index('WLOG,READ,0,8'))
        self.assertFalse(any(cmd.startswith('PID,') for cmd in ws.commands))

    async def test_download_and_identical_retry(self):
        for fault in (None,'retry','conflict','end'):
            ws=FakeWS();ws.fault=fault;await ws.recv()
            s=WheelSession(ws,'A',Path('.'))
            if fault in ('conflict','end'):
                with self.assertRaises(RuntimeError):await s.download()
            else:self.assertEqual(len(await s.download()),10)
            self.assertFalse(any(x=='ARM' or x.startswith('PID,') for x in ws.commands))
    async def test_default_preflight_never_moves_or_writes(self):
        with tempfile.TemporaryDirectory() as temp:
            args=parser().parse_args(['--wheel','A','--output',str(Path(temp)/'run')])
            ws=FakeWS()
            with patch('wheel_tuner.websockets.connect',return_value=Context(ws)),patch('builtins.print'):
                report=await run(args)
            self.assertFalse(report['motion_started'])
            self.assertFalse(any(x in ('ARM','STOP','DISARM') or x.startswith(('PID,','WLOG,CLEAR','WSPD,')) for x in ws.commands))
    async def test_simulated_campaign_uses_holdout_and_only_selected_wheel(self):
        with tempfile.TemporaryDirectory() as temp:
            args=parser().parse_args(['--wheel','A','--rounds','2','--live','--confirm-hardware','--output',str(Path(temp)/'run')])
            ws=FakeWS()
            async def simulated_collect(session,target,baseline,seconds):
                return records(target,tau=.35 if ws.gains[0:2]==[1.2,.0991] else .25)
            with patch('wheel_tuner.websockets.connect',return_value=Context(ws)),patch.object(WheelSession,'collect',simulated_collect):
                report=await run(args)
            self.assertEqual(report['status'],'complete')
            self.assertTrue(report['baseline_validated'])
            self.assertEqual(report['evidence'],'validated_downloaded_WLOG')
            self.assertTrue(any(row['stage']=='baseline_holdout' for row in report['history']))
            self.assertTrue(all(cmd.startswith('PID,A,') for cmd in ws.commands if cmd.startswith('PID,')))
            self.assertEqual(ws.commands[-2:],['STOP','DISARM'])
            self.assertEqual(len(list(args.output.glob('round-*-raw.json'))),len(report['history']))
    async def test_failed_quality_aborts_without_blind_gain_write(self):
        with tempfile.TemporaryDirectory() as temp:
            args=parser().parse_args(['--wheel','A','--live','--confirm-hardware','--output',str(Path(temp)/'run')])
            ws=FakeWS()
            async def bad_collect(*args):return records(tau=.005)
            with patch('wheel_tuner.websockets.connect',return_value=Context(ws)),patch.object(WheelSession,'collect',bad_collect):
                with self.assertRaises(ValueError):await run(args)
            self.assertFalse(any(cmd.startswith('PID,') for cmd in ws.commands))
            report=json.loads((args.output/'session.json').read_text())
            self.assertEqual(report['status'],'failed')
            self.assertEqual(ws.commands[-2:],['STOP','DISARM'])

if __name__=='__main__':unittest.main()
