"""R21 single-wheel WLOG tuning. Default is read-only; --live requires a full plan."""
from __future__ import annotations
import argparse, asyncio, csv, json, math, time
from dataclasses import asdict
from pathlib import Path
import websockets
from bounded_search import guarded_step
from tuning_campaign import make_plan, run_campaign, validate_result
from wheel_log import VERSION, CAPS, INFO, HEADER, parse, validate, score
from closed_loop_tuning import STEP_SCORE_WEIGHTS

class WheelSession:
    def __init__(self, ws, wheel, output):
        self.ws, self.wheel, self.output = ws,wheel,output
        self.ident=0
        self.timeline=[]
        self.wire=[]
        self.batch=[]
        self.uart=self.link=self.all_gains=None
        self.locked_angles=None

    async def command(self, command):
        self.ident += 1
        ident=self.ident
        entry={'host_monotonic':time.monotonic(),'command':command}
        self.timeline.append(entry)
        await self.ws.send(json.dumps({'type':'command','id':ident,'command':command}))
        async with asyncio.timeout(25 if command.startswith('WLOG,READ,') else 5):
            while True:
                raw=await self.ws.recv()
                if isinstance(raw,bytes): raw=raw.decode('utf-8','replace')
                for line in raw.splitlines():
                    self.wire.append(line)
                    if line.startswith('WLOG,DATA,'): self.batch.append(line)
                if not raw.startswith('{'): continue
                result=json.loads(raw)
                if result.get('type') != 'result' or result.get('id') != ident: continue
                entry['result']=result
                if not result.get('ok'): raise RuntimeError(f'{command}: {result}')
                return result['reply']

    async def stopped(self):
        a=(await self.command('ANGLESTATUS')).split(',')
        p=(await self.command('PS2STATUS')).split(',')
        if len(a)!=14 or a[0]!='ATEL' or a[2]!='0' or a[12]!='0' or len(p)!=12 or p[0]!='PS2' or p[2]!='0':
            raise RuntimeError('stop angle/speed control and lock PS2 first')

    async def gains(self):
        p=(await self.command('PIDGET')).split(',')
        if len(p)!=13 or p[0]!='PID': raise RuntimeError('invalid PIDGET')
        v=tuple(map(float,p[1:]))
        if not all(math.isfinite(x) and 0<=x<=100 for x in v): raise RuntimeError('invalid gains')
        return v

    async def health(self):
        if await self.command('VER')!=VERSION or await self.command('CAPS')!=CAPS or await self.command('WLOGINFO')!=INFO:
            raise RuntimeError('R21 exact firmware/controller/encoder profile required')
        u=(await self.command('UART')).split(',')
        l=(await self.command('LINK')).split(',')
        k=(await self.command('STACK')).split(',')
        if len(u)!=10 or u[0]!='UART' or any(int(x) for x in u[5:]): raise RuntimeError('UART unhealthy')
        if len(l)!=7 or l[0]!='LINK' or any(int(x) for x in l[3:6]): raise RuntimeError('LINK unhealthy')
        if len(k)!=5 or k[0]!='STACK' or k[1]!='4096' or int(k[3])<32 or k[4]!='1' or int(k[2])+int(k[3])!=4096:
            raise RuntimeError('STACK unhealthy')
        if await self.command('SPEEDLIMIT')!='SPEEDLIMIT,160': raise RuntimeError('160 RPM ceiling required')
        angles=(await self.command('YAWGET'),await self.command('HEADGET'))
        if self.locked_angles is None: self.locked_angles=angles
        if angles!=self.locked_angles: raise RuntimeError('angular gains changed')
        if self.uart is not None and (u[5:]!=self.uart or l[3:6]!=self.link): raise RuntimeError('link errors changed')
        self.uart,self.link=u[5:],l[3:6]

    async def check_gains(self, expected):
        await self.health()
        actual=await self.gains()
        if actual!=tuple(expected): raise RuntimeError('external wheel gain change; refusing write/score')

    async def download(self):
        await self.stopped()
        status=await self.command('WLOG,STATUS')
        p=status.split(',')
        if len(p)!=7 or p[:2]!=['WLOG','STATUS'] or not all(x.isdigit() for x in p[2:]): raise RuntimeError('bad WLOG status')
        active,total,capacity,overflow,sequence=map(int,p[2:])
        if active or capacity!=1024 or not 0<total<=capacity or overflow or sequence!=total: raise RuntimeError('incomplete/overflowed WLOG')
        records={}
        self.download_diagnostics=[]
        for offset in range(0,total,8):
            count=min(8,total-offset)
            complete=False
            for attempt in range(3):
                self.batch=[]
                reply=None
                try: reply=await self.command(f'WLOG,READ,{offset},{count}')
                except (TimeoutError,RuntimeError) as exc:
                    self.download_diagnostics.append({'offset':offset,'attempt':attempt+1,'error':str(exc)})
                previous=-1
                for line in self.batch:
                    r=parse(line); n=r['sequence']
                    if not offset<=n<offset+count or n<=previous: raise RuntimeError('out-of-batch/reordered WLOG')
                    previous=n
                    if n in records and records[n]!=r: raise RuntimeError('conflicting WLOG retransmission')
                    records[n]=r
                if reply and reply!=f'WLOG,END,{offset},{count},{total}': raise RuntimeError('WLOG END mismatch')
                complete=bool(reply) and all(i in records for i in range(offset,offset+count))
                if complete: break
            if not complete: raise RuntimeError('WLOG missing samples after three bounded reads')
        if await self.command('WLOG,STATUS')!=status: raise RuntimeError('WLOG changed during download')
        return validate([records[i] for i in range(total)])

    async def collect(self, target, baseline_seconds, seconds):
        await self.command('STOP'); await self.stopped()
        for cmd in ('WLOG,CLEAR','WLOG,START','ARM','SPEED,1'):
            await self.command(cmd)
        async def hold(duration):
            deadline=time.monotonic()+duration
            while time.monotonic()<deadline:
                await self.command('WKEEP')
                await asyncio.sleep(min(.08,max(0,deadline-time.monotonic())))
        try:
            await self.command(f'WSPD,{self.wheel},0')
            await hold(baseline_seconds)
            await self.command(f'WSPD,{self.wheel},{int(target)}')
            await hold(seconds)
        finally:
            await self.command('STOP')
            await self.command('WLOG,STOP')
        return await self.download()

async def run(args):
    if args.output.exists(): raise ValueError('use a new output directory; never overwrite a session')
    if not all(math.isfinite(x) for x in (args.baseline_seconds,args.seconds)) or not 10<=abs(args.target)<=60 or args.baseline!=0 or args.baseline_seconds<2 or args.seconds<2 or args.baseline_seconds+args.seconds>9:
        raise ValueError('wheel plan requires baseline 0, target 10..60 RPM, phases >=2 s, total <=9 s')
    if args.search not in ('bo','de'): raise ValueError('wheel search supports BO/DE only')
    if any(not math.isfinite(x) or x != int(x) or not 10 <= abs(x) <= 60 for x in args.validation_targets):
        raise ValueError('wheel holdouts must be signed integer 10..60 RPM targets')
    args.output.mkdir(parents=True)
    async with websockets.connect(args.url,open_timeout=5) as ws:
        if (await ws.recv()).strip()!='BRIDGE,BM50_ACK_R3': raise RuntimeError('R3 bridge required')
        session=WheelSession(ws,args.wheel,args.output)
        report={'status':'preflight','motion_started':False,'selection':'pending','history':[]}
        applied=None
        initial=None
        active_raw=None
        def save():
            report['commands']=session.timeline
            (args.output/'session.json').write_text(json.dumps(report,indent=2,allow_nan=False),encoding='utf-8')
        try:
            await session.stopped(); await session.health()
            initial=applied=await session.gains()
            index='ABCD'.index(args.wheel)*3
            if initial[index+2]!=0: raise RuntimeError('wheel PI profile requires Kd_step=0; no silent conversion')
            for name in ('HLOG','WLOG'):
                p=(await session.command(f'{name},STATUS')).split(',')
                if len(p)!=7 or p[2]!='0': raise RuntimeError('another recorder is active')
            plan=make_plan(args,initial[index:index+2])
            report.update(plan=asdict(plan),firmware=VERSION,caps=CAPS,controller=INFO,
                          wheel=args.wheel,all_initial_gains=initial,ram_only=True,
                          form='incremental',period_s=.01,ki_unit='PWM_percent/RPM/update',
                          kd_step=0,scoring_source='validated_downloaded_WLOG',
                          training_target=args.target,baseline_seconds=args.baseline_seconds,seconds=args.seconds,
                          objective_weights=STEP_SCORE_WEIGHTS,
                          profile='one physical wheel; wheels lifted, free to spin; no ground acceptance')
            if not args.live:
                save(); print(json.dumps(report,indent=2)); return report
            if not args.confirm_hardware:
                answer=input(f'确认整车可靠悬空、四轮自由、急停有效：{args.wheel} 轮，训练{args.target} RPM，留出{plan.validation_targets} RPM，每项{plan.repeats}次，{plan.rounds}组训练，PWM≤60%，只改该轮Kp/Ki_step且每次≤20%。输入YES：')
                if answer!='YES': raise RuntimeError('physical plan not confirmed')
            report['motion_started']=True
            counter=0
            async def apply(expected,target):
                nonlocal applied
                await session.command('STOP'); await session.stopped()
                if tuple(applied[index:index+2])!=tuple(expected): raise RuntimeError('campaign state mismatch')
                for _ in range(256):
                    await session.check_gains(applied)
                    before=applied[index:index+2]
                    if tuple(before)==tuple(target): return
                    step=guarded_step(before,target)
                    if step==tuple(before): raise RuntimeError('bounded write cannot progress')
                    await session.command(f'PID,{args.wheel},{step[0]:.4f},{step[1]:.4f},0')
                    requested=list(applied);requested[index:index+2]=step
                    actual=await session.gains()
                    if actual!=tuple(requested): raise RuntimeError('gain write/readback mismatch')
                    applied=actual
                raise RuntimeError('restoration step limit')
            async def evaluate(gains,stage,target,repeat):
                nonlocal counter,active_raw
                counter+=1
                target=args.target if target is None else target
                active_raw=args.output/f'round-{counter:03d}-raw.json'
                wire_start=len(session.wire)
                raw={'stage':stage,'target':target,'repeat':repeat+1,'gains':gains,'quality':'pending',
                     'wheel':args.wheel,'firmware':VERSION,'caps':CAPS,'controller':INFO,
                     'baseline_seconds':args.baseline_seconds,'seconds':args.seconds,
                     'all_applied_gains':applied,'angular_gains':session.locked_angles,
                     'source':'MCU_WLOG','ram_only':True}
                try:
                    await session.check_gains(applied)
                    records=await session.collect(target,args.baseline_seconds,args.seconds)
                    raw['records']=records
                    samples,result=score(records,args.wheel,target,args.baseline_seconds,args.seconds)
                    validate_result(result,plan.max_overshoot)
                    await session.check_gains(applied)
                    raw.update(quality='passed',metrics=asdict(result))
                    with (args.output/f'round-{counter:03d}.csv').open('w',newline='',encoding='utf-8') as f:
                        writer=csv.DictWriter(f,fieldnames=HEADER);writer.writeheader();writer.writerows(records)
                    return result
                except BaseException as exc:
                    raw.update(quality='failed',error=str(exc) or type(exc).__name__)
                    raise
                finally:
                    raw['wire']=session.wire[wire_start:]
                    raw['download_diagnostics']=getattr(session,'download_diagnostics',[])
                    active_raw.write_text(json.dumps(raw,indent=2,allow_nan=False),encoding='utf-8')
            def checkpoint(update):
                report.update(update);report['all_applied_gains']=applied;save()
            result=await run_campaign(plan,evaluate,apply,checkpoint)
            report.update(result);save();return report
        except BaseException as exc:
            report.update(status='failed',error=str(exc) or type(exc).__name__,restored_readback=None)
            if report['motion_started']:
                try:
                    await session.command('STOP');await session.command('WLOG,STOP');await session.stopped()
                    # Restore only a holdout-validated initial after health and exact current gains.
                    if report.get('baseline_validated'):
                        await apply(applied[index:index+2],initial[index:index+2])
                        report['restored_readback']=await session.gains()
                except BaseException as restore:
                    report['restoration_error']=str(restore) or type(restore).__name__
            save();raise
        finally:
            if report['motion_started']:
                try: await session.command('STOP');await session.command('DISARM')
                except Exception as exc: report['cleanup_error']=str(exc);save()

def parser():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--url',default='ws://127.0.0.1:8766')
    p.add_argument('--wheel',choices=list('ABCD'),required=True)
    p.add_argument('--target',type=int,default=40)
    p.add_argument('--baseline',type=int,default=0)
    p.add_argument('--baseline-seconds',type=float,default=2)
    p.add_argument('--seconds',type=float,default=3)
    p.add_argument('--validation-targets',type=float,nargs='+',default=[-40,20])
    p.add_argument('--search',choices=['bo','de'],default='bo')
    p.add_argument('--rounds',type=int,default=5)
    p.add_argument('--repeats',type=int,default=2)
    p.add_argument('--seed',type=int,default=0)
    p.add_argument('--min-improvement',type=float,default=.01)
    p.add_argument('--max-overshoot',type=float,default=.3)
    p.add_argument('--kp-bounds',type=float,nargs=2)
    p.add_argument('--second-bounds',type=float,nargs=2)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--live',action='store_true')
    p.add_argument('--confirm-hardware',action='store_true')
    p.set_defaults(loop='wheel_speed',experiment='step')
    return p

if __name__=='__main__':
    asyncio.run(run(parser().parse_args()))
