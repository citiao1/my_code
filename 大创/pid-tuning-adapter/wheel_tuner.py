"""R21 four-wheel synchronous WLOG measurement; selected-channel gain search."""
from __future__ import annotations
import argparse, asyncio, csv, json, math, time
from dataclasses import asdict
from pathlib import Path
import websockets
from session_transport import connect_endpoint
from bounded_search import guarded_step
from tuning_campaign import make_plan, run_campaign, validate_result
from wheel_log import (VERSION, CAPS, INFO, FIRMWARE_PROFILES, HEADER,
                       parse, validate, score_four)
from closed_loop_tuning import ResponseScore, STEP_SCORE_WEIGHTS
from pid_ws_logger import distance_from_encoder, encoder_row

GROUND_RETURN_TOLERANCE_M = 0.05
GROUND_DISTANCE_MARGIN = 0.97
GROUND_MIN_DISTANCE_RATIO = 0.75
GROUND_MAX_DISTANCE_RATIO = 1.25
GROUND_FORWARD_PROBE_DISTANCE_M = 0.10
GROUND_FORWARD_PROBE_MIN_RATIO = 0.50
GROUND_FORWARD_PROBE_MAX_RATIO = 1.50
GROUND_FORWARD_PROBE_TARGET_RPM = 25
# On the current FRONT_REVERSED R23 vehicle, a physical forward DRV,+25
# probe produces negative counts on all four wheel encoders.
GROUND_FORWARD_ENCODER_SIGN = -1

class DriveRefreshOverrun(RuntimeError):
    def __init__(self, elapsed_s, max_gap_s):
        super().__init__('DRV refresh interval exceeded the watchdog guard')
        self.elapsed_s = elapsed_s
        self.max_gap_s = max_gap_s

class WlogConflictError(RuntimeError):
    pass

def timed_leg_seconds(distance_m, target_rpm):
    if not math.isfinite(distance_m) or distance_m <= 0 or target_rpm == 0:
        raise ValueError('timed ground leg requires positive distance and nonzero target')
    nominal_speed = abs(target_rpm) * math.pi * .127 / 60.0
    return distance_m / nominal_speed * GROUND_DISTANCE_MARGIN

def load_resume_state(path, plan, initial, args, session):
    """Resume only the known safe case: complete baseline train, failed first holdout."""
    path=Path(path)
    if path.name!='session.json' or not path.is_file():
        raise ValueError('--resume-from must name an existing session.json')
    previous=json.loads(path.read_text(encoding='utf-8'))
    expected_plan=json.loads(json.dumps(asdict(plan)))
    if previous.get('status')!='failed' or previous.get('error')!='encoder feedback lost before return leg':
        raise ValueError('resume source is not the interrupted encoder-feedback campaign')
    if previous.get('baseline_validated') or previous.get('selection')!='pending_holdout':
        raise ValueError('resume source is not pending its first baseline holdout')
    previous_plan=previous.get('plan')
    if previous_plan!=expected_plan:
        # Holdouts are independent cases: changing their order is allowed so
        # the operator-confirmed forward case can run before the reverse case.
        # Their values and all other plan fields must remain identical.
        if not isinstance(previous_plan,dict):
            raise ValueError('resume plan does not match the prior campaign')
        previous_normalized=dict(previous_plan)
        expected_normalized=dict(expected_plan)
        previous_targets=previous_normalized.get('validation_targets')
        current_targets=expected_normalized.get('validation_targets')
        if not isinstance(previous_targets,list) or not isinstance(current_targets,list):
            raise ValueError('resume plan does not match the prior campaign')
        previous_normalized['validation_targets']=sorted(previous_targets)
        expected_normalized['validation_targets']=sorted(current_targets)
        if previous_normalized!=expected_normalized:
            raise ValueError('resume plan differs beyond validation-target order')
    if (previous.get('wheel')!=args.wheel or previous.get('training_target')!=args.target or
            previous.get('ground_distance_m')!=args.ground_distance or
            previous.get('baseline_seconds')!=args.baseline_seconds or
            previous.get('seconds')!=args.seconds):
        raise ValueError('resume motion plan differs from the prior campaign')
    if (previous.get('firmware')!=session.firmware or previous.get('caps')!=session.caps or
            previous.get('controller')!=session.controller):
        raise ValueError('firmware identity differs from the prior campaign')
    initial=list(initial)
    if previous.get('all_initial_gains')!=initial or previous.get('all_applied_gains')!=initial:
        raise ValueError('prior campaign did not preserve the exact initial RAM gains')
    history=previous.get('history')
    if not isinstance(history,list) or len(history)!=plan.repeats:
        raise ValueError('resume source lacks every initial training repeat')
    scores=[]
    for repeat,entry in enumerate(history,1):
        if (entry.get('stage')!='train' or entry.get('target') is not None or
                entry.get('repeat')!=repeat or entry.get('gains')!=list(plan.initial)):
            raise ValueError('resume history is not a complete initial training group')
        raw_path=path.parent/f'round-{repeat:03d}-raw.json'
        if not raw_path.is_file():
            raise ValueError(f'missing completed training evidence: {raw_path}')
        raw=json.loads(raw_path.read_text(encoding='utf-8'))
        if (raw.get('quality')!='passed' or raw.get('stage')!='train' or
                raw.get('target')!=args.target or raw.get('repeat')!=repeat or
                raw.get('gains')!=list(plan.initial) or raw.get('all_applied_gains')!=initial or
                raw.get('source')!='MCU_WLOG' or not raw.get('records')):
            raise ValueError(f'training evidence {repeat} is not a complete passed WLOG trial')
        scores.append(ResponseScore(**entry['metrics']))
    failed_path=path.parent/f'round-{len(history)+1:03d}-raw.json'
    if not failed_path.is_file():
        raise ValueError(f'missing failed-round diagnostics: {failed_path}')
    failed=json.loads(failed_path.read_text(encoding='utf-8'))
    if (failed.get('quality')!='failed' or
            failed.get('error')!='encoder feedback lost before return leg' or
            not any(line.startswith('ENC,') and len(line.split(','))!=16
                    for line in failed.get('wire',[]))):
        raise ValueError('failed round does not contain the expected malformed ENC packet')
    return history,scores

class WheelSession:
    def __init__(self, ws, wheel, output):
        self.ws, self.wheel, self.output = ws,wheel,output
        self.ident=0
        self.timeline=[]
        self.wire=[]
        self.batch=[]
        self.uart=self.link=self.all_gains=None
        self.locked_angles=None
        self.firmware=self.caps=self.controller=None
        self.last_ground_metrics=None
        self.encoder_diagnostics=[]

    async def command(self, command, *, drive_lease_ms=None):
        self.ident += 1
        ident=self.ident
        entry={'host_monotonic':time.monotonic(),'command':command}
        if drive_lease_ms is not None: entry['drive_lease_ms']=drive_lease_ms
        self.timeline.append(entry)
        wire_start=len(self.wire)
        request={'type':'command','id':ident,'command':command}
        if drive_lease_ms is not None:
            request.update(type='drive_lease',duration_ms=drive_lease_ms)
        try:
            await self.ws.send(json.dumps(request))
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
        except BaseException as exc:
            # Preserve only frames received while this command was in flight;
            # a missing/malformed status reply then remains diagnosable in the
            # preflight session without duplicating the entire WLOG wire log.
            entry['wire']=self.wire[wire_start:]
            entry.setdefault('error',f'{type(exc).__name__}: {exc}')
            raise

    async def read_encoder(self):
        """Drop malformed diagnostic packets and retry a bounded number of reads."""
        for attempt in range(1,4):
            reply=await self.command('ENC')
            packet=encoder_row(reply)
            if packet is None:
                self.encoder_diagnostics.append({
                    'attempt':attempt, 'kind':'discarded_malformed_packet',
                    'wire':reply,
                })
                continue
            if packet[1]!=1 or packet[2]!=380:
                self.encoder_diagnostics.append({
                    'attempt':attempt, 'kind':'discarded_unready_or_wrong_cpr',
                    'ready':packet[1], 'cpr':packet[2], 'wire':reply,
                })
                continue
            return packet
        raise RuntimeError('no valid ready 380-CPR encoder packet after three reads')

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
        identity=(await self.command('VER'),await self.command('CAPS'),await self.command('WLOGINFO'))
        if identity not in FIRMWARE_PROFILES:
            raise RuntimeError('unsupported exact R21/R22/R23 wheel firmware/controller profile')
        self.firmware,self.caps,self.controller=identity
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
        def accept(line, expected_sequence):
            try:
                row=parse(line)
            except ValueError as exc:
                self.download_diagnostics.append({
                    'sequence':expected_sequence, 'kind':'discarded_malformed_row',
                    'error':str(exc),
                })
                return False
            n=row['sequence']
            if n!=expected_sequence:
                self.download_diagnostics.append({
                    'sequence':expected_sequence, 'kind':'discarded_wrong_sequence',
                    'observed_sequence':n,
                })
                return False
            if n in records and records[n]!=row:
                raise WlogConflictError(f'conflicting valid WLOG retransmission {n}')
            records[n]=row
            return True

        # Keep every individually valid row. A damaged or missing row does not
        # invalidate its neighbors; only unresolved sequence numbers are read
        # again, one at a time, after a bounded small-batch pass.
        batch_size=4
        for offset in range(0,total,batch_size):
            count=min(batch_size,total-offset)
            end=f'WLOG,END,{offset},{count},{total}'
            last_error='missing samples'
            ended=False
            for attempt in range(3):
                self.batch=[]
                try:
                    reply=await self.command(f'WLOG,READ,{offset},{count}')
                except (TimeoutError,RuntimeError) as exc:
                    last_error=str(exc)
                    self.download_diagnostics.append({
                        'offset':offset, 'count':count, 'attempt':attempt+1,
                        'kind':'batch_read_failed', 'error':last_error,
                    })
                    continue
                if reply!=end:
                    last_error=f'WLOG END mismatch: {reply!r}'
                    self.download_diagnostics.append({
                        'offset':offset, 'count':count, 'attempt':attempt+1,
                        'kind':'batch_end_mismatch', 'reply':reply,
                    })
                    continue
                ended=True
                for line in self.batch:
                    try:
                        row=parse(line)
                    except ValueError as exc:
                        self.download_diagnostics.append({
                            'offset':offset, 'count':count,
                            'attempt':attempt+1, 'kind':'discarded_malformed_row',
                            'error':str(exc), 'wire':line,
                        })
                        continue
                    n=row['sequence']
                    if not offset<=n<offset+count:
                        self.download_diagnostics.append({
                            'offset':offset, 'count':count, 'attempt':attempt+1,
                            'kind':'discarded_out_of_batch_row', 'sequence':n,
                        })
                        continue
                    if n in records and records[n]!=row:
                        raise WlogConflictError(f'conflicting valid WLOG retransmission {n}')
                    records[n]=row
                break

            if not ended:
                raise RuntimeError(
                    f'WLOG batch {offset} did not return a matching END after three reads: {last_error}'
                )

            for n in range(offset,offset+count):
                if n in records:
                    continue
                recovered=False
                for attempt in range(3):
                    self.batch=[]
                    try:
                        reply=await self.command(f'WLOG,READ,{n},1')
                    except (TimeoutError,RuntimeError) as exc:
                        self.download_diagnostics.append({
                            'sequence':n, 'attempt':attempt+1,
                            'kind':'single_read_failed', 'error':str(exc),
                        })
                        continue
                    if reply!=f'WLOG,END,{n},1,{total}':
                        self.download_diagnostics.append({
                            'sequence':n, 'attempt':attempt+1,
                            'kind':'single_end_mismatch', 'reply':reply,
                        })
                        continue
                    for line in self.batch:
                        if accept(line,n):
                            recovered=True
                        else:
                            # Keep scanning the returned lines; a malformed
                            # fragment is diagnostic, not a batch-wide failure.
                            continue
                    if recovered:
                        self.download_diagnostics.append({
                            'sequence':n, 'attempt':attempt+1,
                            'kind':'recovered_single_row',
                        })
                        break
                if not recovered:
                    raise RuntimeError(f'WLOG sequence {n} remained unavailable after three single-row reads')
        if await self.command('WLOG,STATUS')!=status: raise RuntimeError('WLOG changed during download')
        return validate([records[i] for i in range(total)])

    async def drive_for_duration(self,target,duration):
        if not math.isfinite(duration) or duration<=0:
            raise ValueError('timed ground leg requires a positive duration')
        duration_ms=round(duration*1000)
        if not 1<=duration_ms<=12000:
            raise ValueError('timed ground leg exceeds the bridge drive-lease limit')
        command=f'DRV,{int(target)},0,0'
        if await self.command(command,drive_lease_ms=duration_ms)!='ACK,DRV':
            raise RuntimeError('four-wheel timed DRV lease not acknowledged')
        # The bridge starts the lease before returning the command ACK. Anchor
        # the completion wait after that ACK so BLE command latency does not
        # consume the watchdog/event grace interval.
        start=time.monotonic()
        lease_id=self.ident
        deadline=start+duration+1.0
        while time.monotonic()<deadline:
            try:
                raw=await asyncio.wait_for(self.ws.recv(),timeout=min(.1,deadline-time.monotonic()))
            except asyncio.TimeoutError:
                continue
            if isinstance(raw,bytes): raw=raw.decode('utf-8','replace')
            for line in raw.splitlines():
                self.wire.append(line)
                if not line.startswith('{'):
                    continue
                event=json.loads(line)
                if (event.get('type')!='drive_lease' or event.get('id')!=lease_id):
                    continue
                if event.get('event')=='failed':
                    raise DriveRefreshOverrun(time.monotonic()-start,
                                              float(event.get('max_gap_s') or 0.0))
                if event.get('event')=='complete':
                    return time.monotonic()-start,float(event.get('max_gap_s') or 0.0)
        raise TimeoutError('bridge drive lease did not complete before deadline')

    async def collect(self, target, baseline_seconds, seconds, ground_distance_m=None):
        self.encoder_diagnostics=[]
        self.download_diagnostics=[]
        self.last_ground_metrics=None
        self.direction_probe=None
        async def expect(command, expected):
            reply=await self.command(command)
            if reply!=expected:
                raise RuntimeError(f'{command} returned unexpected reply: {reply!r}')
            return reply

        await expect('STOP','ACK,STOP')
        await self.stopped()
        origin=None
        log_started=False
        log_stopped=False
        async def hold(target_rpm,duration):
            command=f'DRV,{int(target_rpm)},0,0'
            if target_rpm == 0:
                await expect(command,'ACK,DRV')
                await asyncio.sleep(duration)
                return
            deadline=time.monotonic()+duration
            while time.monotonic()<deadline:
                await expect(command,'ACK,DRV')
                await asyncio.sleep(min(.08,max(0,deadline-time.monotonic())))

        try:
            if ground_distance_m is not None:
                await expect('ENCZERO','ACK,ENCZERO')
                origin_packet=await self.read_encoder()
                origin=origin_packet[3]

                # The last live probe showed that -25 RPM moved backward.
                # Use +25 as the forward candidate and require all four live
                # encoder deltas to confirm it before scoring or going farther.
                await expect('ARM','ACK,ARM,WHEELS_OFF_GROUND')
                await expect('SPEED,1','ACK,SPEED,1')
                probe_duration=timed_leg_seconds(
                    GROUND_FORWARD_PROBE_DISTANCE_M,
                    GROUND_FORWARD_PROBE_TARGET_RPM,
                )
                self.direction_probe={
                    'command_rpm':GROUND_FORWARD_PROBE_TARGET_RPM,
                    'expected_direction':'forward',
                    'expected_encoder_sign':GROUND_FORWARD_ENCODER_SIGN,
                    'requested_distance_m':GROUND_FORWARD_PROBE_DISTANCE_M,
                    'elapsed_s':None,
                    'max_drv_gap_s':None,
                    'passed':False,
                }
                probe_elapsed,probe_gap=await self.drive_for_duration(
                    GROUND_FORWARD_PROBE_TARGET_RPM,probe_duration)
                await asyncio.sleep(.12)
                await expect('STOP','ACK,STOP')
                probe_packet=await self.read_encoder()
                probe_deltas=tuple(probe_packet[3][i]-origin[i] for i in range(4))
                probe_distance=distance_from_encoder(probe_packet[3],origin)
                self.direction_probe.update({
                    'encoder_wheel_deltas':list(probe_deltas),
                    'encoder_distance_m':probe_distance,
                    'elapsed_s':probe_elapsed,
                    'max_drv_gap_s':probe_gap,
                })
                if any(delta*GROUND_FORWARD_ENCODER_SIGN<=0 for delta in probe_deltas):
                    raise RuntimeError(
                        'forward direction probe found a wheel encoder delta with the wrong sign; '
                        'stopped without issuing reverse motion')
                if not (GROUND_FORWARD_PROBE_DISTANCE_M*GROUND_FORWARD_PROBE_MIN_RATIO <=
                        probe_distance <=
                        GROUND_FORWARD_PROBE_DISTANCE_M*GROUND_FORWARD_PROBE_MAX_RATIO):
                    raise RuntimeError(
                        f'forward direction probe displacement {probe_distance:.3f} m '
                        'is outside its safe verification range; stopped without reverse motion')
                self.direction_probe['passed']=True

            # ARM alone does not enable wheel motion. Move its BLE round trip
            # outside the 1024-record WLOG window; firmware still rejects
            # CLEAR/START whenever attitude or speed control is active.
            await expect('ARM','ACK,ARM,WHEELS_OFF_GROUND')
            await expect('WLOG,CLEAR','WLOG,CLEAR,OK')
            await expect('WLOG,START','WLOG,START,OK')
            log_started=True
            await expect('SPEED,1','ACK,SPEED,1')
            # WLOG setup and BLE ACK latency already provide part of the
            # baseline. Keep at least 1.5 s after the zero DRV ACK so the
            # enabled, settled baseline remains >=2 s in the MCU timeline.
            await hold(0,max(1.5,baseline_seconds-.5))
            if ground_distance_m is None:
                await hold(target,seconds)
            else:
                probe_distance=self.direction_probe['encoder_distance_m']
                remaining_distance=ground_distance_m-probe_distance
                if remaining_distance<=0:
                    raise RuntimeError('forward probe already reached the full outbound distance')
                outbound_duration=timed_leg_seconds(remaining_distance,target)
                if outbound_duration>seconds:
                    raise RuntimeError('timed outbound leg exceeds its declared timeout')
                outbound_reached=True
                outbound_refresh_gap=0.0
                try:
                    outbound_elapsed,outbound_refresh_gap=await self.drive_for_duration(target,outbound_duration)
                except DriveRefreshOverrun as exc:
                    outbound_elapsed=exc.elapsed_s
                    outbound_refresh_gap=exc.max_gap_s
                    outbound_reached=False
                await asyncio.sleep(.12)
                await expect('STOP','ACK,STOP')
                await expect('WLOG,STOP','WLOG,STOP,OK')
                log_stopped=True
                departure=await self.read_encoder()
                outbound_deltas=tuple(departure[3][i]-origin[i] for i in range(4))
                if any(delta*GROUND_FORWARD_ENCODER_SIGN<=0 for delta in outbound_deltas):
                    raise RuntimeError(
                        'scored outbound encoder sign disagrees with forward probe; '
                        'stopped without issuing reverse motion')
                outbound=distance_from_encoder(departure[3],origin)
                outbound_reached = outbound_reached and (
                    ground_distance_m*GROUND_MIN_DISTANCE_RATIO <= outbound <=
                    ground_distance_m*GROUND_MAX_DISTANCE_RATIO)
                await expect('ARM','ACK,ARM,WHEELS_OFF_GROUND')
                await expect('SPEED,1','ACK,SPEED,1')
                nominal_speed=abs(target)*math.pi*.127/60.0
                return_duration=outbound/max(nominal_speed,.02)*GROUND_DISTANCE_MARGIN
                return_refresh_gap=0.0
                try:
                    _,return_refresh_gap=await self.drive_for_duration(-target,return_duration)
                except DriveRefreshOverrun as exc:
                    return_refresh_gap=exc.max_gap_s
                    await self.command('DRV,0,0,0')
                await expect('STOP','ACK,STOP')
                returned=await self.read_encoder()
                residual=distance_from_encoder(returned[3],origin)
                if residual>GROUND_RETURN_TOLERANCE_M:
                    raise RuntimeError(f'ground return residual {residual:.3f} m exceeds tolerance')
                self.last_ground_metrics={'requested_distance_m':ground_distance_m,
                    'outbound_distance_m':outbound,'return_residual_m':residual,
                    'distance_margin':GROUND_DISTANCE_MARGIN,
                    'return_tolerance_m':GROUND_RETURN_TOLERANCE_M,
                    'direction_probe':self.direction_probe,
                    'encoder_cpr':origin_packet[2],
                    'outbound_duration_s':outbound_elapsed,
                    'return_duration_s':return_duration,
                    'outbound_max_drv_gap_s':outbound_refresh_gap,
                    'return_max_drv_gap_s':return_refresh_gap,
                    'outbound_reached':outbound_reached,
                    'return_within_tolerance':True}
        finally:
            await expect('STOP','ACK,STOP')
            if log_started and not log_stopped:
                await expect('WLOG,STOP','WLOG,STOP,OK')
        await self.command('TEL,0')
        try:
            return await self.download()
        finally:
            await self.command('TEL,1')

async def run(args):
    if args.output.exists(): raise ValueError('use a new output directory; never overwrite a session')
    if not all(math.isfinite(x) for x in (args.baseline_seconds,args.seconds)) or not 10<=abs(args.target)<=60 or args.baseline!=0 or args.baseline_seconds<2 or args.seconds<2 or args.baseline_seconds+args.seconds>9:
        raise ValueError('wheel plan requires baseline 0, target 10..60 RPM, phases >=2 s, total <=9 s')
    if args.search not in ('bo','de'): raise ValueError('wheel search supports BO/DE only')
    if args.live and (args.ground_distance is None or not math.isfinite(args.ground_distance) or args.ground_distance<=0):
        raise ValueError('live ground test requires a positive encoder-measured --ground-distance')
    if args.live and (abs(args.target)<25 or any(abs(x)<25 for x in args.validation_targets)):
        raise ValueError('1 m encoder-limited trials require target/holdout magnitudes >=25 RPM')
    if any(not math.isfinite(x) or x != int(x) or not 10 <= abs(x) <= 60 for x in args.validation_targets):
        raise ValueError('wheel holdouts must be signed integer 10..60 RPM targets')
    if (args.live and args.ground_distance is not None and
            args.validation_targets[0]*GROUND_FORWARD_PROBE_TARGET_RPM<=0):
        raise ValueError('the first ground holdout must use the verified forward-command sign')
    args.output.mkdir(parents=True)
    async with connect_endpoint(args) as ws:
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
            resume_history=resume_results=None
            if args.resume_from is not None:
                resume_history,resume_results=load_resume_state(
                    args.resume_from,plan,initial,args,session)
            report.update(plan=asdict(plan),firmware=session.firmware,caps=session.caps,controller=session.controller,
                          wheel=args.wheel,all_initial_gains=initial,ram_only=True,
                          measurement_wheels=list('ABCD'),simultaneous_measurement=True,
                          form='incremental',period_s=.01,ki_unit='PWM_percent/RPM/update',
                          kd_step=0,scoring_source='validated_downloaded_WLOG',
                          training_target=args.target,baseline_seconds=args.baseline_seconds,seconds=args.seconds,
                          objective_weights=STEP_SCORE_WEIGHTS,
                          profile='four wheels on ground under normal load; WLOG scores outbound encoder-distance leg; every test returns to origin',
                          ground_distance_m=args.ground_distance,
                          ground_distance_margin=GROUND_DISTANCE_MARGIN,
                          ground_return_tolerance_m=GROUND_RETURN_TOLERANCE_M,
                          forward_encoder_sign=GROUND_FORWARD_ENCODER_SIGN,
                          required_test_setup='four_wheels_on_ground_not_lifted',
                          wheel_aggregation='worst wheel for score and each metric')
            if args.resume_from is not None:
                report.update(resumed_from=str(Path(args.resume_from).resolve()),
                              reused_training_repeats=len(resume_history))
            if not args.live:
                save(); print(json.dumps(report,indent=2)); return report
            if not args.confirm_hardware:
                answer=input(f'确认整车不架空、前进{args.ground_distance:.2f}m后按编码器倒车回原点、前后场地清空且急停有效：四轮训练{args.target} RPM，留出{plan.validation_targets} RPM，每项{plan.repeats}次，{plan.rounds}组训练，PWM≤60%，本轮只改{args.wheel}轮Kp/Ki_step且每次≤20%。输入YES：')
                if answer!='YES': raise RuntimeError('physical plan not confirmed')
            report['motion_started']=True
            counter=len(resume_history or [])
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
                     'wheel':args.wheel,'firmware':session.firmware,'caps':session.caps,'controller':session.controller,
                     'measurement_wheels':list('ABCD'),'simultaneous_measurement':True,
                     'required_test_setup':'four_wheels_on_ground_not_lifted',
                     'baseline_seconds':args.baseline_seconds,'seconds':args.seconds,
                     'ground_distance_m':args.ground_distance,
                     'ground_distance_margin':GROUND_DISTANCE_MARGIN,
                     'ground_return_tolerance_m':GROUND_RETURN_TOLERANCE_M,
                     'all_applied_gains':applied,'angular_gains':session.locked_angles,
                     'source':'MCU_WLOG','ram_only':True}
                try:
                    await session.check_gains(applied)
                    records=await session.collect(target,args.baseline_seconds,args.seconds,args.ground_distance)
                    raw['records']=records
                    raw['ground_metrics']=session.last_ground_metrics
                    if (args.ground_distance is not None and session.last_ground_metrics and
                            not session.last_ground_metrics['outbound_reached']):
                        raise RuntimeError('outbound distance target not reached; returned safely, round excluded')
                    edge=next(i for i,row in enumerate(records) if row['A_target_rpm']!=0)
                    end=next((i for i in range(edge+1,len(records)) if records[i]['A_target_rpm']==0),len(records))
                    measured_baseline_seconds=((records[edge]['tick_us']-records[0]['tick_us'])&0xffffffff)/1e6
                    measured_seconds=((records[end-1]['tick_us']-records[edge]['tick_us'])&0xffffffff)/1e6
                    measured,result=score_four(records,-target,measured_baseline_seconds,measured_seconds)
                    raw['wheel_metrics']={wheel:asdict(row[1]) for wheel,row in measured.items()}
                    raw['measured_baseline_seconds']=measured_baseline_seconds
                    raw['measured_target_seconds']=measured_seconds
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
                    raw['encoder_diagnostics']=getattr(session,'encoder_diagnostics',[])
                    raw['direction_probe']=getattr(session,'direction_probe',None)
                    active_raw.write_text(json.dumps(raw,indent=2,allow_nan=False),encoding='utf-8')
            def checkpoint(update):
                report.update(update);report['all_applied_gains']=applied;save()
            result=await run_campaign(
                plan,evaluate,apply,checkpoint,
                initial_results=resume_results,initial_history=resume_history,
            )
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
    p.add_argument('--serial-port',help='exclusive direct COM port, 115200/8N1; no bridge service')
    p.add_argument('--wheel',choices=list('ABCD'),required=True,
                   help='gain channel to optimize; A/B/C/D always run and are measured together')
    p.add_argument('--target',type=int,default=40)
    p.add_argument('--baseline',type=int,default=0)
    p.add_argument('--baseline-seconds',type=float,default=2)
    p.add_argument('--seconds',type=float,default=7)
    p.add_argument('--ground-distance',type=float,
                   help='live ground mode: encoder-measured outbound distance; each test returns to origin')
    p.add_argument('--validation-targets',type=float,nargs='+',default=[-40,30])
    p.add_argument('--search',choices=['bo','de'],default='bo')
    p.add_argument('--rounds',type=int,default=5)
    p.add_argument('--repeats',type=int,default=2)
    p.add_argument('--seed',type=int,default=0)
    p.add_argument('--min-improvement',type=float,default=.01)
    p.add_argument('--max-overshoot',type=float,default=.3)
    p.add_argument('--kp-bounds',type=float,nargs=2)
    p.add_argument('--second-bounds',type=float,nargs=2)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--resume-from',type=Path,
                   help='resume the initial holdout after an encoder-read failure, reusing verified training WLOGs')
    p.add_argument('--live',action='store_true')
    p.add_argument('--confirm-hardware',action='store_true')
    p.set_defaults(loop='wheel_speed',experiment='step')
    return p

if __name__=='__main__':
    asyncio.run(run(parser().parse_args()))
