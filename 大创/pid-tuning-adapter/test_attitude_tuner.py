"""In-memory bridge regression; no physical serial/BLE/probe is accessed."""
import asyncio
import json
import math
import tempfile
import unittest
from contextlib import asynccontextmanager
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import AsyncMock, patch
from attitude_tuner import CAPS, FIXED, FIRMWARE_VERSION, Session, run
from hlog_experiment import score_logged
from pid_tuning import Sample


class FakeBridge:
    def __init__(self):
        self.incoming=asyncio.Queue()
        self.sent=[]
        self.gains={"YAWPID":(0.15,2.5),"HEADPID":(5,1.25)}
        self.bad_readback=False
        self.records=[]
        self.mode=0
        self.process=0.0
        self.recording=False
        self.corrupt=None

    async def recv(self):
        return "BRIDGE,BM50_ACK_R3\n"

    async def send(self,text):
        request=json.loads(text)
        command=request["command"]
        self.sent.append(command)
        replies={
            "VER":FIRMWARE_VERSION,
            "CAPS":CAPS,"STACK":"STACK,4096,1400,2696,1",
            "UART":"UART,9600,42000000,4375,0x00,0,0,0,0,0",
            "LINK":"LINK,100,5,0,0,0,1","SPEEDLIMIT":"SPEEDLIMIT,160",
            "PIDGET":"PID,"+",".join(str(x) for x in FIXED),
            "ANGLESTATUS":f"ATEL,100,{self.mode},1,0,0,0,0,0,0,0,0,{int(self.mode!=0)},12000",
            "HLOG,STATUS":f"HLOG,STATUS,{int(self.recording)},{len(self.records)},1024,0,{len(self.records)}",
        }
        name=command.split(",")[0]
        if command == "STOP":
            self.mode=0
        elif name in ("HEAD", "MOVE"):
            self.mode=2 if name=="HEAD" else 1
        elif command == "HLOG,CLEAR":
            self.records=[]
            self.process=0.0
        elif command == "HLOG,START":
            self.recording=True
        elif command == "HLOG,STOP":
            self.recording=False
        elif command.startswith("HLOG,READ,"):
            _,_,offset,count=command.split(",")
            offset,count=int(offset),int(count)
            for row in self.records[offset:offset+count]:
                await self.incoming.put(row)
            replies[command]=f"HLOG,END,{offset},{count},{len(self.records)}"
        if name in self.gains:
            if self.mode or self.recording:
                raise AssertionError("gain write occurred before STOP / HLOG,STOP")
            self.gains[name]=tuple(float(x) for x in command.split(",")[1:])
        if command in ("YAWGET","HEADGET"):
            key="YAWPID" if command=="YAWGET" else "HEADPID"
            gains=self.gains[key]
            if self.bad_readback: gains=(gains[0]+1,gains[1])
            reply=f"{key},{gains[0]:.4f},{gains[1]:.4f},"+(
                "60,80,20" if key=="YAWPID" else "80,20,D_MEAS")
        else:
            reply=replies.get(command,f"ACK,{name}")
        await self.incoming.put(json.dumps({"id":request["id"],"ok":True,"reply":reply}))

    def __aiter__(self):
        return self

    async def __anext__(self):
        return await self.incoming.get()


class LiveSessionTests(unittest.IsolatedAsyncioTestCase):
    def args(self,path,loop="angular_velocity",live=True):
        return SimpleNamespace(url="ws://fake",loop=loop,live=live,
                               confirm_hardware=True,baseline=0,target=15,rounds=2,
                               baseline_seconds=4,seconds=6,output=Path(path))

    async def exercise(self,loop,fail=False,live=True,experiment="step",corrupt=None):
        bridge=FakeBridge()
        bridge.corrupt=corrupt
        @asynccontextmanager
        async def connect(*args,**kwargs):
            yield bridge
        async def collect(session,which,target,seconds,first_tick):
            await session.command(session.motion_command(which,target))
            if fail: raise RuntimeError("IMU feedback lost")
            for _ in range(round(seconds/0.02)):
                bridge.process += (target-bridge.process)*(1-math.exp(-0.02/(0.001 if corrupt=="fast" else 1)))
                i=len(bridge.records)
                sp,pv=round(target*1000),round(bridge.process*1000)
                values=(sp,pv,sp-pv,0,0,5000,0) if which=="direction" else (0,0,0,sp,pv,0,5000)
                bridge.records.append("HLOG,DATA,"+",".join(str(x) for x in
                    (i,1000+20*i,*values,0,0,0,bridge.mode)))
            if target and corrupt in ("duplicate", "gap", "mode"):
                fields=bridge.records[250].split(",")
                fields[{"duplicate":2,"gap":3,"mode":14}[corrupt]]={
                    "duplicate":"249","gap":"999999","mode":"2"}[corrupt]
                bridge.records[250]=",".join(fields)
            # Deliberately bogus low-rate values must NEVER reach the scorer.
            return [Sample(0,999,999,999)],1000
        with tempfile.TemporaryDirectory() as directory:
            with patch("attitude_tuner.websockets.connect",connect),patch.object(Session,"collect",collect):
                args=self.args(directory,loop,live)
                args.experiment=experiment
                if fail or corrupt:
                    with self.assertRaises((RuntimeError,ValueError)):
                        await run(args)
                    saved=json.loads((Path(directory)/"round-01-raw.json").read_text())
                    self.assertTrue(saved["error"])
                else:
                    await run(args)
            history=json.loads((Path(directory)/"session.json").read_text()) if live and not fail and not corrupt else None
        return bridge,history

    def test_motion_commands_match_console_control_path(self):
        self.assertEqual(
            Session.motion_command("angular_velocity", 15),
            "MOVE,0,0,15.0000",
        )
        self.assertEqual(
            Session.motion_command("angular_velocity", -15),
            "MOVE,0,0,-15.0000",
        )
        self.assertEqual(
            Session.motion_command("direction", 15),
            "HEAD,15.0000",
        )

    async def test_rate_writes_verified_gains_without_rearming_and_restores_tested_best(self):
        bridge,history=await self.exercise("angular_velocity")
        self.assertEqual(bridge.sent.count("ARM"),1)
        self.assertEqual(bridge.sent[-1],"STOP")
        self.assertTrue(any(x.startswith("YAWPID,") for x in bridge.sent))
        self.assertFalse(any(x.startswith(("HEADPID,","PID,","SPDLIMIT,")) for x in bridge.sent))
        self.assertEqual(bridge.gains["YAWPID"],tuple(history["best_gains"]))
        self.assertEqual(len(history["history"]),2)
        self.assertEqual(history["scoring_source"],"downloaded_HLOG")
        self.assertLess(history["history"][0]["metrics"]["steady_error"],0.01)
        self.assertEqual(bridge.sent.count("HLOG,CLEAR"),2)
        self.assertEqual(bridge.sent.count("HLOG,STOP"),2)

    async def test_spin_downloads_both_signs_and_no_bad_log_can_write(self):
        bridge,history=await self.exercise("angular_velocity",experiment="spin")
        self.assertIn("MOVE,0,0,-15.0000",bridge.sent)
        self.assertEqual(history["experiment"],"spin")
        for corrupt in ("fast","duplicate","gap","mode"):
            with self.subTest(corrupt=corrupt):
                failed,_=await self.exercise("angular_velocity",corrupt=corrupt)
                self.assertFalse(any(x.startswith("YAWPID,") for x in failed.sent))
                self.assertEqual(failed.sent[-1],"STOP")

    async def test_heading_never_writes_inner_rate_and_sensor_failure_stops(self):
        bridge,_=await self.exercise("direction")
        self.assertTrue(any(x.startswith("HEADPID,") for x in bridge.sent))
        self.assertFalse(any(x.startswith("YAWPID,") for x in bridge.sent))
        failed,_=await self.exercise("direction",fail=True)
        self.assertEqual(failed.sent[-1],"STOP")

    async def test_readonly_preflight_never_moves_or_writes(self):
        bridge,_=await self.exercise("angular_velocity",live=False)
        self.assertFalse(any(x.split(",")[0] in ("ARM","YAW","HEAD","STOP","YAWPID","HEADPID")
                             for x in bridge.sent))

    async def test_command_results_and_telemetry_are_demultiplexed_and_readback_checked(self):
        bridge=FakeBridge()
        session=Session(bridge)
        try:
            await bridge.incoming.put("ATEL,100,1,1,15000,12000,5000,0,0,0,0,0,1,12000")
            self.assertEqual(await session.get_gains("angular_velocity"),(0.15,2.5))
            self.assertFalse(session.frames.empty())
            bridge.bad_readback=True
            with self.assertRaises(RuntimeError):
                await session.write("angular_velocity",(0.16,2.5))
        finally:
            session.reader.cancel()
            await asyncio.gather(session.reader,return_exceptions=True)

    async def test_downloaded_rate_log_checks_status_mode_and_sequence(self):
        session = Session(FakeBridge())
        try:
            session.command = AsyncMock(side_effect=[
                "HLOG,STATUS,0,2,1024,0,2", "HLOG,END,0,2,2"])
            for sequence, tick in enumerate((100, 120)):
                await session.hlog_frames.put(
                    f"HLOG,DATA,{sequence},{tick},0,0,0,15000,12000,0,5000,0,0,0,1")
            records = await session.read_hlog(loop="angular_velocity")
            self.assertEqual([x["mode"] for x in records], [1, 1])
            for status in ("HLOG,STATUS,1,2,1024,0,2",
                           "HLOG,STATUS,0,1024,1024,1,1025",
                           "HLOG,STATUS,0,2,1024,0,3",
                           "HLOG,STATUS,0,0,1024,0,0"):
                session.command = AsyncMock(return_value=status)
                with self.assertRaises(RuntimeError):
                    await session.read_hlog(loop="angular_velocity")
                self.assertEqual(session.command.await_count, 1)
        finally:
            session.reader.cancel()
            await asyncio.gather(session.reader, return_exceptions=True)

    async def test_capacity_rejected_before_arm_and_unresolved_logs_never_write(self):
        bridge=FakeBridge()
        @asynccontextmanager
        async def connect(*a,**kw):
            yield bridge
        with tempfile.TemporaryDirectory() as directory:
            args=self.args(directory)
            args.experiment="spin"
            args.seconds=10
            with patch("attitude_tuner.websockets.connect",connect):
                with self.assertRaisesRegex(ValueError,"maximum"):
                    await run(args)
            self.assertEqual(bridge.sent,[])

    async def test_short_or_mixed_log_never_scores(self):
        rows=[]
        for i in range(500):
            target=0 if i<200 else 15
            rows.append(dict(sequence=i,tick=1000+20*i,mode=1,
                             target_rate_dps=target,actual_rate_dps=target,
                             rate_output_rpm=5,saturated=False,forward_rpm=0,left_rpm=0))
        with self.assertRaisesRegex(ValueError,"10 independent"):
            score_logged(rows,"angular_velocity",[(0,4),(15,6)])
        rows[250]["mode"]=2
        with self.assertRaises(ValueError):
            score_logged(rows,"angular_velocity",[(0,4),(15,6)])


if __name__=="__main__":
    unittest.main()
