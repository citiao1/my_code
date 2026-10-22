"""R21 WLOG provenance and single-wheel step quality; no hardware writes."""
from __future__ import annotations
import math
from statistics import mean
from closed_loop_tuning import score_step
from pid_tuning import Sample

VERSION = 'VER,VGT6_BM50_ATTITUDE_R21'
CAPS = 'CAPS,ATTITUDE_R21,20,10,160,60,60,80,SPEED_TUNABLE,HLOG,PS2,TRACKMOVE,HLOG_MODE,HLOG_US,TOF_DL1B,WLOG_US,TOF_GUARD,FRONT_REVERSED,TOF_FAST'
INFO = 'WLOG,INFO,10000,1024,INCREMENTAL,KI_STEP,KD_STEP,70,30,60,160,380,FRONT_REVERSED'
HEADER = ['sequence','tick_ms','tick_us','enabled','armed','angle_mode']
for wheel in 'ABCD':
    HEADER += [f'{wheel}_target_rpm', f'{wheel}_filtered_rpm_x10', f'{wheel}_raw_rpm_x10', f'{wheel}_pwm_mpercent']
HEADER += ['saturated_mask','tof_limited','battery_mv','tof_mm','tof_valid','encoder_ready']

def parse(line):
    p = line.strip().split(',')
    if len(p) != 30 or p[:2] != ['WLOG','DATA']:
        raise ValueError('invalid WLOG field count/type')
    if any(not x or not x.lstrip('-').isdigit() or x.startswith('--') for x in p[2:]):
        raise ValueError('noninteger WLOG field')
    v = tuple(map(int, p[2:]))
    if any(not 0 <= x <= 0xffffffff for x in v[:3]):
        raise ValueError('WLOG sequence/tick outside uint32')
    if any(v[i] not in (0,1) for i in (3,4,23,26,27)) or v[5] not in (0,1,2):
        raise ValueError('WLOG flags/mode invalid')
    if not 0 <= v[22] <= 15 or not 0 <= v[24] <= 65535:
        raise ValueError('WLOG saturation/battery invalid')
    if (v[26] and not 1 <= v[25] <= 4000) or (not v[26] and v[25] != -1):
        raise ValueError('WLOG distance invalid')
    for i in range(6,22,4):
        if abs(v[i]) > 200 or abs(v[i+1]) > 2147483647 or abs(v[i+2]) > 2147483647 or abs(v[i+3]) > 60000:
            raise ValueError('WLOG wheel/output range invalid')
    return dict(zip(HEADER,v))

def validate(records):
    if not records or len(records) > 1024:
        raise ValueError('WLOG empty or oversized')
    for i,r in enumerate(records):
        # Re-parse even offline JSON so numeric/flag validation is identical.
        canonical = parse('WLOG,DATA,'+','.join(str(r[k]) for k in HEADER))
        if canonical != r or r['sequence'] != i:
            raise ValueError('WLOG missing/reordered sequence')
        if ((r['tick_us']-r['tick_ms']*1000) & 0xffffffff) >= 1000:
            raise ValueError('WLOG ms/us phase mismatch')
        if i and not 9900 <= ((r['tick_us']-records[i-1]['tick_us']) & 0xffffffff) <= 15000:
            raise ValueError('WLOG cadence outside 9900..15000 us')
    return records

def score(records, wheel, target, baseline_seconds=2., seconds=3.):
    validate(records)
    if wheel not in 'ABCD' or len(wheel) != 1 or not 10 <= abs(target) <= 60:
        raise ValueError('declare one wheel and a signed 10..60 RPM target')
    if not all(math.isfinite(x) and x >= 2 for x in (baseline_seconds,seconds)):
        raise ValueError('baseline/target duration must be >=2 s')
    times=[0.]
    for a,b in zip(records,records[1:]):
        times.append(times[-1]+((b['tick_us']-a['tick_us']) & 0xffffffff)/1e6)
    for r in records:
        if r['angle_mode'] or not r['encoder_ready'] or r['tof_limited'] or not 10000 <= r['battery_mv'] <= 14000:
            raise ValueError('WLOG mode/encoder/battery/TOF intervention unhealthy')
        if r[f'{wheel}_target_rpm'] and (not r['enabled'] or not r['armed']):
            raise ValueError('WLOG control not enabled/armed')
        for other in 'ABCD':
            if abs(r[f'{other}_raw_rpm_x10']) > 3000 or abs(r[f'{other}_filtered_rpm_x10']) > 3000:
                raise ValueError('wheel overspeed guard')
            if other != wheel and (r[f'{other}_target_rpm'] or r[f'{other}_pwm_mpercent'] or abs(r[f'{other}_raw_rpm_x10']) > 30):
                raise ValueError('single-wheel isolation violated')
    targets=[r[f'{wheel}_target_rpm'] for r in records]
    edge=next((i for i,v in enumerate(targets) if v != 0),None)
    if edge is None:
        raise ValueError('WLOG has no reference edge')
    end=next((i for i in range(edge,len(targets)) if targets[i] == 0),len(targets))
    if any(targets[:edge]) or any(v != target for v in targets[edge:end]) or any(targets[end:]):
        raise ValueError('WLOG does not match one locked baseline->target step')
    if times[-1]-times[end-1] > .3 or abs(times[edge]-baseline_seconds) > .25 or times[end-1]-times[edge] < seconds-.15 or times[-1] > baseline_seconds+seconds+.5:
        raise ValueError('WLOG phase duration/coverage failed')
    base=[r[f'{wheel}_filtered_rpm_x10']/10 for r,t in zip(records,times) if times[edge]-1 <= t < times[edge]]
    if len(base) < 60 or math.sqrt(mean(x*x for x in base)) > 1:
        raise ValueError('one-second baseline not settled')
    samples=[Sample(t,r[f'{wheel}_target_rpm'],r[f'{wheel}_filtered_rpm_x10']/10,r[f'{wheel}_pwm_mpercent']/1000) for t,r in zip(times[:end],records[:end])]
    result=score_step(samples,angle=False,output_limit=60)
    progress=[s.process/target for s in samples[edge:]]
    if sum(.1 < p < .9 for p in progress) < 10:
        raise ValueError('fewer than ten independent 10..90% transient samples')
    final=[s.process-target for s in samples if s.time_s >= samples[-1].time_s-1]
    if len(final) < 60 or math.sqrt(mean(x*x for x in final)) > max(2,abs(target)*.08):
        raise ValueError('final one-second RMS/coverage failed')
    n=max(1,len(final)//4)
    if abs(mean(final[-n:])-mean(final[:n])) > max(1,abs(target)*.03):
        raise ValueError('final-window drift failed')
    return samples,result
