"""In-memory bridge regression; no physical serial/BLE/probe is accessed."""
import asyncio
import json
import math
import tempfile
import unittest
from contextlib import asynccontextmanager
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
from attitude_tuner import CAPS, FIXED, Session, run
from pid_tuning import Sample


class FakeBridge:
    def __init__(self):
        self.incoming=asyncio.Queue()
        self.sent=[]
        self.gains={"YAWPID":(0.15,2.5),"HEADPID":(5,1.25)}
        self.bad_readback=False

    async def recv(self):
        return "BRIDGE,BM50_ACK_R3\n"

    async def send(self,text):
        request=json.loads(text)
        command=request["command"]
        self.sent.append(command)
        replies={
            "VER":"VER,VGT6_BM50_ATTITUDE_R10",
            "CAPS":CAPS,"STACK":"STACK,4096,1400,2696,1",
            "UART":"UART,9600,42000000,4375,0x00,0,0,0,0,0",
            "LINK":"LINK,100,5,0,0,0,1","SPEEDLIMIT":"SPEEDLIMIT,160",
            "PIDGET":"PID,"+",".join(str(x) for x in FIXED),
            "ANGLESTATUS":"ATEL,100,2,1,0,0,0,0,0,0,0,0,1,12000",
        }
        name=command.split(",")[0]
        if name in self.gains:
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

    async def exercise(self,loop,fail=False,live=True):
        bridge=FakeBridge()
        @asynccontextmanager
        async def connect(*args,**kwargs):
            yield bridge
        async def collect(session,which,target,seconds,first_tick):
            name="YAW" if which=="angular_velocity" else "HEAD"
            await session.command(f"{name},{target}")
            if fail: raise RuntimeError("IMU feedback lost")
            if not target:
                rows=[Sample(i*0.2,0,0,0) for i in range(15)]
            else:
                rows=[Sample(3+i*0.2,15,15*(1-math.exp(-i*0.2)),5)
                      for i in range(35)]
            return rows,1000
        with tempfile.TemporaryDirectory() as directory:
            with patch("attitude_tuner.websockets.connect",connect),patch.object(Session,"collect",collect):
                if fail:
                    with self.assertRaisesRegex(RuntimeError,"feedback lost"):
                        await run(self.args(directory,loop,live))
                else:
                    await run(self.args(directory,loop,live))
            history=json.loads((Path(directory)/"session.json").read_text()) if live and not fail else None
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


if __name__=="__main__":
    unittest.main()
