"""R19 adds ToF while retaining the R18 angular-loop and HLOG semantics."""
import tempfile
import unittest
from contextlib import asynccontextmanager
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
from attitude_tuner import FIRMWARE_CAPS, Session, run
from test_attitude_tuner import FakeBridge


class TofFirmwareCompatibilityTests(unittest.IsolatedAsyncioTestCase):
    async def test_r19_readonly_preflight_keeps_microsecond_requirement(self):
        await self.check_preflight("VER,VGT6_BM50_ATTITUDE_R19")

    async def test_r20_readonly_preflight_keeps_angular_semantics(self):
        await self.check_preflight("VER,VGT6_BM50_ATTITUDE_R20")

    async def test_r21_readonly_preflight_keeps_angular_semantics(self):
        await self.check_preflight("VER,VGT6_BM50_ATTITUDE_R21")

    async def check_preflight(self, version):
        bridge = FakeBridge()
        bridge.firmware_version = version
        bridge.caps = FIRMWARE_CAPS[bridge.firmware_version]
        bridge.gains = {"YAWPID": (0.1815, 1.8062), "HEADPID": (6.0, 1.25)}
        sessions = []
        def create(ws):
            session = Session(ws)
            sessions.append(session)
            return session
        @asynccontextmanager
        async def connect(*args, **kwargs):
            yield bridge
        with tempfile.TemporaryDirectory() as directory:
            args = SimpleNamespace(url="ws://fake", loop="angular_velocity", live=False,
                confirm_hardware=True, baseline=0, target=8, rounds=1,
                baseline_seconds=4, seconds=6, output=Path(directory))
            with patch("attitude_tuner.websockets.connect", connect), patch("attitude_tuner.Session", side_effect=create):
                await run(args)
        self.assertTrue(sessions[0].require_hlog_us)
        self.assertIn(",HLOG_US,TOF_DL1B", sessions[0].expected_caps)
        self.assertFalse(any(cmd.startswith(("ARM", "MOVE,", "YAWPID,", "HEADPID,")) for cmd in bridge.sent))
