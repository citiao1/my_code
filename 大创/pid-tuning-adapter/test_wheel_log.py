import asyncio, copy, json, math, tempfile, unittest
from pathlib import Path
from unittest.mock import AsyncMock, patch
from wheel_log import (HEADER, VERSION, CAPS, INFO, R22_VERSION, R22_CAPS,
                       parse, validate, score, score_four)
from wheel_tuner import (DriveRefreshOverrun, WheelSession, parser, run,
                         timed_leg_seconds)

def records(target=40,tau=.25):
    target=int(target)
    rows=[]
    for i in range(501):
        t=i*.01;ref=target if i>=200 else 0
        actual=ref*(1-math.exp(-(t-2)/tau)) if ref else 0
        v=[i,i*10+1000,i*10000+1000000,1,1,0]
        v += [ref,round(actual*10),round(actual*10),int(ref*.4)*1000]*4
        v += [0,0,12000,500,1,1]
        rows.append(parse('WLOG,DATA,'+','.join(map(str,v))))
    return rows

class FakeWS:
    def __init__(self):
        self.queue=asyncio.Queue();self.queue.put_nowait('BRIDGE,BM50_ACK_R3\n')
        self.commands=[];self.requests=[];self.gains=[v for ki in (.0991,.0998,.0998,.1008) for v in (1.2,ki,0.)]
        self.version,self.caps=VERSION,CAPS
        self.data=records()[:10];self.reads=0;self.fault=None
    async def recv(self): return await self.queue.get()
    async def send(self,text):
        p=json.loads(text);cmd=p['command'];self.requests.append(p);self.commands.append(cmd)
        replies={'VER':self.version,'CAPS':self.caps,'WLOGINFO':INFO,
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
        if p['type']=='drive_lease':
            self.queue.put_nowait(json.dumps({'type':'drive_lease','event':'complete','id':p['id'],'max_gap_s':.1}))

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
    def test_single_wheel_or_staggered_targets_are_rejected(self):
        for kind in ('single','staggered'):
            rows=records()
            if kind=='single':
                for row in rows:
                    for wheel in 'BCD':
                        for key in ('target_rpm','filtered_rpm_x10','raw_rpm_x10','pwm_mpercent'):
                            row[f'{wheel}_{key}']=0
            else:
                rows[200]['D_target_rpm']=0
            with self.assertRaisesRegex(ValueError,'four-wheel synchronous'):
                score(rows,'A',40)

    def test_all_wheels_are_scored_and_a_bad_unselected_channel_fails(self):
        rows=records()
        for row in rows[400:]:
            row['B_filtered_rpm_x10']-=10
        measured,result=score_four(rows,40)
        self.assertEqual(set(measured),set('ABCD'))
        self.assertEqual(result.score,max(row[1].score for row in measured.values()))
        self.assertGreater(measured['B'][1].steady_error,measured['A'][1].steady_error)
        for row in rows[400:]:row['D_filtered_rpm_x10']=200
        with self.assertRaises(ValueError):score(rows,'A',40)
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
    def test_timed_leg_estimate_is_distance_and_speed_bounded(self):
        self.assertAlmostEqual(timed_leg_seconds(1.,40),4.134,places=2)
        with self.assertRaises(ValueError):timed_leg_seconds(0,40)
        with self.assertRaises(ValueError):timed_leg_seconds(1,0)

class WheelSessionTests(unittest.IsolatedAsyncioTestCase):
    async def test_r23_exact_profile_is_readonly_and_preserves_wlog_equations(self):
        from wheel_log import R23_VERSION, R23_CAPS
        ws=FakeWS();ws.version,ws.caps=R23_VERSION,R23_CAPS;await ws.recv()
        session=WheelSession(ws,'A',Path('.'))
        await session.health()
        self.assertEqual(session.firmware,R23_VERSION)
        self.assertEqual(session.caps,R23_CAPS)
        self.assertFalse(any(c=='ARM' or c.startswith(('PID,','DRV,')) for c in ws.commands))

    async def test_r22_exact_firmware_profile_is_accepted_read_only(self):
        ws=FakeWS();ws.version,ws.caps=R22_VERSION,R22_CAPS;await ws.recv()
        session=WheelSession(ws,'A',Path('.'))
        await session.health()
        self.assertEqual(session.firmware,R22_VERSION)
        self.assertEqual(session.caps,R22_CAPS)
        self.assertFalse(any(cmd in ('ARM','STOP','DISARM') or cmd.startswith('PID,')
                             for cmd in ws.commands))

    async def test_capture_command_order_and_watchdog_use_without_real_time_or_hardware(self):
        ws=FakeWS();await ws.recv();s=WheelSession(ws,'A',Path('.'))
        clock=[0.]
        async def advance(seconds):clock[0]+=seconds
        with patch('wheel_tuner.time.monotonic',side_effect=lambda:clock[0]),patch('wheel_tuner.asyncio.sleep',advance):
            rows=await s.collect(40,2,3)
        self.assertEqual(len(rows),10)
        start=ws.commands.index('WLOG,START')
        self.assertLess(start,ws.commands.index('ARM'))
        self.assertEqual(ws.commands.count('DRV,0,0,0'),1)
        self.assertGreater(ws.commands.count('DRV,40,0,0'),20)
        self.assertFalse(any(cmd.startswith('WSPD,') or cmd=='WKEEP' for cmd in ws.commands))
        stop=ws.commands.index('STOP',start)
        self.assertLess(stop,ws.commands.index('WLOG,STOP'))
        self.assertLess(ws.commands.index('WLOG,STOP'),ws.commands.index('WLOG,READ,0,8'))
        self.assertLess(ws.commands.index('TEL,0'),ws.commands.index('WLOG,READ,0,8'))
        self.assertGreater(ws.commands.index('TEL,1'),ws.commands.index('WLOG,READ,0,8'))
        self.assertFalse(any(cmd.startswith('PID,') for cmd in ws.commands))

    async def test_timed_ground_leg_uses_bridge_lease_without_encoder_queries(self):
        ws=FakeWS();await ws.recv();s=WheelSession(ws,'A',Path('.'))
        elapsed,max_gap=await s.drive_for_duration(40,.1)
        self.assertGreaterEqual(elapsed,0)
        self.assertEqual(max_gap,.1)
        self.assertEqual(ws.commands,['DRV,40,0,0'])
        self.assertEqual(ws.requests[0]['type'],'drive_lease')
        self.assertEqual(ws.requests[0]['duration_ms'],100)
        self.assertFalse(any(cmd=='ENC' for cmd in ws.commands))

    async def test_refresh_overrun_returns_safely_and_excludes_trial(self):
        session=WheelSession(FakeWS(),'A',Path('.'))
        commands=[]
        enc=iter((
            'ENC,1,1,380,0,0,0,0,0,0,0,0,0,0,0,0',
            'ENC,2,1,380,200,200,200,200,0,0,0,0,0,0,0,0',
            'ENC,3,1,380,5,5,5,5,0,0,0,0,0,0,0,0',
        ))
        async def command(cmd):
            commands.append(cmd)
            if cmd=='ENC': return next(enc)
            return 'ACK,'+cmd.split(',')[0]
        session.command=command
        session.stopped=AsyncMock()
        session.download=AsyncMock(return_value=[])
        drives=[]
        async def drive(target,duration):
            drives.append((target,duration))
            if len(drives)==1: raise DriveRefreshOverrun(.3,.31)
            return duration,.1
        session.drive_for_duration=drive
        clock=[0.]
        async def advance(seconds): clock[0]+=seconds
        with patch('wheel_tuner.time.monotonic',side_effect=lambda:clock[0]), \
             patch('wheel_tuner.asyncio.sleep',advance):
            await session.collect(40,2,7,1.)
        self.assertEqual([item[0] for item in drives],[40,-40])
        self.assertEqual(commands.count('DRV,0,0,0'),1)
        self.assertIn('STOP',commands)
        self.assertTrue(session.last_ground_metrics['outbound_reached'] is False)
        self.assertLessEqual(session.last_ground_metrics['return_residual_m'],.15)
        self.assertFalse(session.last_ground_metrics['outbound_reached'])

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
            self.assertEqual(report['measurement_wheels'],list('ABCD'))
            self.assertFalse(any(x in ('ARM','STOP','DISARM') or x.startswith(('PID,','WLOG,CLEAR','WSPD,')) for x in ws.commands))
    async def test_simulated_campaign_measures_four_wheels_and_changes_only_selected_gains(self):
        with tempfile.TemporaryDirectory() as temp:
            args=parser().parse_args(['--wheel','A','--rounds','2','--live','--confirm-hardware','--ground-distance','1','--output',str(Path(temp)/'run')])
            ws=FakeWS()
            async def simulated_collect(session,target,baseline,seconds,distance):
                return records(-target,tau=.35 if ws.gains[0:2]==[1.2,.0991] else .25)
            with patch('wheel_tuner.websockets.connect',return_value=Context(ws)),patch.object(WheelSession,'collect',simulated_collect):
                report=await run(args)
            self.assertEqual(report['status'],'complete')
            self.assertTrue(report['baseline_validated'])
            self.assertEqual(report['evidence'],'validated_downloaded_WLOG')
            self.assertTrue(any(row['stage']=='baseline_holdout' for row in report['history']))
            self.assertTrue(all(cmd.startswith('PID,A,') for cmd in ws.commands if cmd.startswith('PID,')))
            self.assertEqual(ws.commands[-2:],['STOP','DISARM'])
            self.assertEqual(len(list(args.output.glob('round-*-raw.json'))),len(report['history']))
            for path in args.output.glob('round-*-raw.json'):
                raw=json.loads(path.read_text())
                self.assertEqual(set(raw['wheel_metrics']),set('ABCD'))
                self.assertTrue(raw['simultaneous_measurement'])
    async def test_failed_quality_aborts_without_blind_gain_write(self):
        with tempfile.TemporaryDirectory() as temp:
            args=parser().parse_args(['--wheel','A','--live','--confirm-hardware','--ground-distance','1','--output',str(Path(temp)/'run')])
            ws=FakeWS()
            async def bad_collect(*args):return records(tau=.005)
            with patch('wheel_tuner.websockets.connect',return_value=Context(ws)),patch.object(WheelSession,'collect',bad_collect):
                with self.assertRaises(ValueError):await run(args)
            self.assertFalse(any(cmd.startswith('PID,') for cmd in ws.commands))
            report=json.loads((args.output/'session.json').read_text())
            self.assertEqual(report['status'],'failed')
            self.assertEqual(ws.commands[-2:],['STOP','DISARM'])

if __name__=='__main__':unittest.main()
