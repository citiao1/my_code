#!/usr/bin/env python3
"""R7 PI angular-velocity / D-on-measurement PD heading live experiment."""
from __future__ import annotations
import argparse
import asyncio
import csv
import json
import math
import time
from dataclasses import asdict
from pathlib import Path
import websockets
from closed_loop_tuning import candidate, score_step, wrap_deg
from pid_tuning import Sample
from pid_ws_logger import BRIDGE_GREETING, FIRMWARE_VERSION, telemetry_row

CAPS = "CAPS,ATTITUDE_R7,20,10,200,60,60,80,SPEED_FIXED"
FIXED = [x for ki in (0.0991,0.0998,0.0998,0.1008) for x in (1.2,ki,0)]


class Session:
    def __init__(self, ws):
        self.ws, self.serial = ws, 0
        self.pending, self.frames = {}, asyncio.Queue(maxsize=256)
        self.fault = None
        self.armed = False
        self.reader = asyncio.create_task(self.receive())

    async def receive(self):
        try:
            async for text in self.ws:
                if not isinstance(text, str):
                    continue
                if text.startswith("{"):
                    reply = json.loads(text)
                    future = self.pending.get(reply.get("id"))
                    if future and not future.done():
                        if reply.get("ok"):
                            future.set_result(reply.get("reply", ""))
                        else:
                            future.set_exception(RuntimeError(reply.get("error")))
                elif text.startswith("ATEL,"):
                    self.frames.put_nowait((time.monotonic(), text.strip()))
                elif text.startswith(("STATUS,0,", "ERR,")):
                    self.fault = RuntimeError(text.strip())
        except Exception as exc:
            self.fault = exc
        finally:
            for future in self.pending.values():
                if not future.done():
                    future.set_exception(ConnectionError("bridge receiver stopped"))

    async def command(self, text):
        self.serial += 1
        number = self.serial
        future = asyncio.get_running_loop().create_future()
        self.pending[number] = future
        try:
            await self.ws.send(json.dumps({"type":"command","id":number,"command":text}))
            return await asyncio.wait_for(future, 5)
        finally:
            self.pending.pop(number, None)

    async def heartbeat(self):
        try:
            while True:
                await self.command("KEEP")
                await asyncio.sleep(0.4)
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            self.fault = exc

    async def health(self):
        if self.fault:
            raise self.fault
        if await self.command("CAPS") != CAPS:
            raise RuntimeError("firmware capability or fixed limits changed")
        if await self.command("SPEEDLIMIT") != "SPEEDLIMIT,200":
            raise RuntimeError("wheel ceiling changed")
        stack = (await self.command("STACK")).split(",")
        if len(stack) != 5 or stack[4] != "1" or int(stack[3]) < 32:
            raise RuntimeError("stack health failure")
        uart = await self.command("UART")
        if uart != self.uart_baseline:
            raise RuntimeError("UART diagnostics changed")
        link = (await self.command("LINK")).split(",")
        if len(link) != 7 or link[3:6] != self.link_baseline:
            raise RuntimeError("serial drops/overflows/errors changed")
        wheel = (await self.command("PIDGET")).split(",")
        if len(wheel) != 13 or any(abs(float(a)-b)>0.0001 for a,b in zip(wheel[1:],FIXED)):
            raise RuntimeError("fixed wheel gains mismatch")

    async def get_gains(self, loop):
        fields = (await self.command("YAWGET" if loop=="angular_velocity" else "HEADGET")).split(",")
        if loop == "angular_velocity":
            if len(fields)!=6 or fields[0]!="YAWPID" or fields[3:]!=["60","80","20"]:
                raise RuntimeError("unexpected rate form/limits/period")
        elif len(fields)!=6 or fields[0]!="HEADPID" or fields[3:]!=["80","20","D_MEAS"]:
            raise RuntimeError("unexpected heading derivative/limits/period")
        gains = tuple(float(x) for x in fields[1:3])
        if not all(math.isfinite(x) and 0<=x<=100 for x in gains):
            raise RuntimeError("invalid gains")
        return gains

    async def write(self, loop, gains):
        old = await self.get_gains(loop)
        if any(abs(new-prev)>max(abs(prev)*0.2001,0.0001) for new,prev in zip(gains,old)):
            raise RuntimeError("per-write gain change exceeds locked 20% bound")
        name = "YAWPID" if loop=="angular_velocity" else "HEADPID"
        reply = await self.command(f"{name},{gains[0]:.4f},{gains[1]:.4f}")
        if reply != f"ACK,{name}" or await self.get_gains(loop)!=gains:
            raise RuntimeError("PID write/readback mismatch")

    async def collect(self, loop, target, seconds, first_tick):
        name = "YAW" if loop=="angular_velocity" else "HEAD"
        while not self.frames.empty():
            self.frames.get_nowait()
        await self.command(f"{name},{target:.4f}")
        deadline = time.monotonic()+seconds
        samples=[]
        last_time = None
        while time.monotonic()<deadline:
            if self.fault:
                raise self.fault
            received, text = await asyncio.wait_for(self.frames.get(), 1.0)
            if time.monotonic()-received > 0.8:
                raise RuntimeError("stale telemetry")
            parsed = telemetry_row(text, None, first_tick, loop)
            if parsed is None:
                raise RuntimeError("IMU/control disabled or invalid ATEL")
            row, first_tick = parsed
            if abs(wrap_deg(row[1]-target) if loop=="direction" else row[1]-target) > 0.001:
                continue # one pre-command frame may already be queued in the UART
            fields = text.split(",")
            if abs(float(fields[5])/1000)>120:
                raise RuntimeError("rate outside session guard")
            if last_time is not None and not 0 < row[0]-last_time <= 0.8:
                raise RuntimeError("telemetry gap or non-monotonic tick")
            last_time = row[0]
            samples.append(Sample(*row))
        return samples, first_tick


async def run(args):
    args.output.mkdir(parents=True, exist_ok=True)
    async with websockets.connect(args.url, open_timeout=5) as ws:
        if (await asyncio.wait_for(ws.recv(),5)).strip()!=BRIDGE_GREETING:
            raise RuntimeError("R3 bridge required")
        s = Session(ws)
        heartbeat = None
        try:
            if await s.command("VER")!=FIRMWARE_VERSION:
                raise RuntimeError("R7 firmware required")
            s.uart_baseline = await s.command("UART")
            u = s.uart_baseline.split(",")
            if len(u)!=10 or u[0]!="UART" or any(int(x) for x in u[5:]):
                raise RuntimeError("UART fault counters nonzero before experiment")
            link=(await s.command("LINK")).split(",")
            if len(link)!=7: raise RuntimeError("invalid LINK")
            s.link_baseline=link[3:6]
            await s.health()
            current=await s.get_gains(args.loop)
            inner=await s.get_gains("angular_velocity")
            if not args.live:
                print(json.dumps({"capabilities":CAPS,"gains":current,"inner":inner,
                                  "motion_started":False},indent=2))
                return
            if not args.confirm_hardware:
                answer=input("首次确认：车体可实际旋转、场地清空、急停有效，"
                             f"目标 {args.baseline}→{args.target}，固定限幅 60 RPM / 80°/s。输入 YES：")
                if answer!="YES": raise RuntimeError("hardware not confirmed")
            # Only one ARM; rounds keep controllers running. Targets are a
            # predeclared two-level experiment, never chosen by the optimizer.
            await s.command("ARM")
            s.armed=True
            name="YAW" if args.loop=="angular_velocity" else "HEAD"
            await s.command(f"{name},{args.baseline}")
            await s.command("ATEL,1")
            heartbeat=asyncio.create_task(s.heartbeat())
            best_gains=current
            best_score=float("inf")
            history=[]
            for round_index in range(args.rounds):
                await s.health()
                if await s.get_gains(args.loop)!=current:
                    raise RuntimeError("external PID change")
                if args.loop=="direction" and await s.get_gains("angular_velocity")!=inner:
                    raise RuntimeError("inner rate gains changed while tuning outer loop")
                baseline, first_tick=await s.collect(args.loop,args.baseline,args.baseline_seconds,None)
                step, _=await s.collect(args.loop,args.target,args.seconds,first_tick)
                await s.health()
                if await s.get_gains(args.loop)!=current:
                    raise RuntimeError("external PID change during measurement")
                if args.loop=="direction" and await s.get_gains("angular_velocity")!=inner:
                    raise RuntimeError("inner PID changed during outer-loop measurement")
                result=score_step(baseline+step,angle=args.loop=="direction",
                                  output_limit=60 if args.loop=="angular_velocity" else 80)
                csv_path=args.output/f"round-{round_index+1:02d}.csv"
                with csv_path.open("w",newline="",encoding="utf-8") as stream:
                    writer=csv.writer(stream)
                    writer.writerow(("time_s","setpoint","process","output"))
                    writer.writerows((x.time_s,x.setpoint,x.process,x.output) for x in baseline+step)
                if result.score<best_score:
                    best_score,best_gains=result.score,current
                proposal=candidate(best_gains,result,args.loop)
                # Each actual write, including rollback, is bounded relative
                # to the currently applied coefficients.
                proposal=tuple(round(max(old*0.8,min(old*1.2,new)),4)
                               for old,new in zip(current,proposal))
                record={"round":round_index+1,"gains":current,"metrics":asdict(result),
                        "next_gains":proposal,"csv":str(csv_path)}
                history.append(record)
                (args.output/"session.json").write_text(json.dumps({
                    "loop":args.loop,"form":"parallel","period_s":0.02,
                    "derivative":"measurement" if args.loop=="direction" else "none",
                    "telemetry_period_s":0.2,"limits":{"rpm":60,"dps":80,"wheel_rpm":200},
                    "baseline":args.baseline,"target":args.target,"history":history,
                    "best_gains":best_gains,"best_score":best_score},indent=2),encoding="utf-8")
                print(json.dumps(record))
                if round_index+1<args.rounds:
                    await s.write(args.loop,proposal)
                    current=proposal
            # Finish on the best measured parameters; do not apply an untested last candidate.
            while current != best_gains:
                rollback = tuple(round(max(old*0.8,min(old*1.2,new)),4)
                                 for old,new in zip(current,best_gains))
                if rollback == current:
                    raise RuntimeError("cannot restore measured best within locked gain guard")
                await s.write(args.loop,rollback)
                current=rollback
        finally:
            if heartbeat:
                heartbeat.cancel()
                await asyncio.gather(heartbeat,return_exceptions=True)
            if s.armed:
                try: await s.command("STOP")
                except Exception as exc: print(f"STOP unavailable: {exc}; MCU heartbeat watchdog expires in 1.5s")
            s.reader.cancel()
            await asyncio.gather(s.reader,return_exceptions=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--loop",required=True,choices=("angular_velocity","direction"))
    p.add_argument("--url",default="ws://127.0.0.1:8766")
    p.add_argument("--live",action="store_true")
    p.add_argument("--confirm-hardware",action="store_true")
    p.add_argument("--baseline",type=float,default=0)
    p.add_argument("--target",type=float,required=True)
    p.add_argument("--rounds",type=int,default=5)
    p.add_argument("--seconds",type=float,default=6)
    p.add_argument("--baseline-seconds",type=float,default=4)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    bounds=80 if a.loop=="angular_velocity" else 180
    if not all(math.isfinite(x) for x in (a.baseline,a.target,a.seconds,a.baseline_seconds)):
        p.error("all numeric arguments must be finite")
    if max(abs(a.baseline),abs(a.target))>bounds or not 1<=a.rounds<=20:
        p.error("invalid target or rounds")
    step=abs(wrap_deg(a.target-a.baseline) if a.loop=="direction" else a.target-a.baseline)
    if not 5<=step<=30 or not 3<=a.seconds<=20 or not 3<=a.baseline_seconds<=20:
        p.error("step must be 5..30 in loop units; durations 3..20s")
    try: asyncio.run(run(a))
    except KeyboardInterrupt: return 130
    except Exception as exc: print(f"Tuning stopped: {exc}"); return 1
    return 0


if __name__=="__main__":
    raise SystemExit(main())
