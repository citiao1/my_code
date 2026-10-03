from __future__ import annotations

import json
import secrets
import signal
import time
from dataclasses import dataclass
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Event, RLock, Thread
from typing import Any, Callable
from urllib.parse import urlsplit

try:
    from .servo_pwm import PwmError, ServoLimits, SysfsPwm
    from .vision_worker import FaceTrackingWorker, VisionConfig
except ImportError:
    from servo_pwm import PwmError, ServoLimits, SysfsPwm
    from vision_worker import FaceTrackingWorker, VisionConfig


ROOT = Path(__file__).resolve().parent
UI_PATH = ROOT / "servo_ui.html"
UI_SCRIPT_PATH = ROOT / "servo_ui.js"
UI_STYLE_PATH = ROOT / "servo_ui.css"


@dataclass(frozen=True)
class AxisConfig:
    name: str
    label: str
    channel: int
    limits: ServoLimits = ServoLimits()


class ServoAxis:
    def __init__(
        self,
        config: AxisConfig,
        pwm_factory: Callable[..., Any] = SysfsPwm,
    ) -> None:
        self.config = config
        self.pwm = pwm_factory(channel=config.channel)
        self.pulse_us = config.limits.center_us

    def open(self) -> None:
        self.pwm.open(self.config.limits.center_us * 1000)
        self.pulse_us = self.config.limits.center_us

    def set_pulse(self, pulse_us: int) -> int:
        self.config.limits.validate(pulse_us)
        self.pwm.set_pulse_us(pulse_us)
        self.pulse_us = pulse_us
        return pulse_us

    def center(self) -> int:
        return self.set_pulse(self.config.limits.center_us)

    def close(self) -> None:
        self.pwm.close()

    def status(self) -> dict[str, Any]:
        limits = self.config.limits
        half_range_us = max(
            min(
                limits.center_us - limits.min_us,
                limits.max_us - limits.center_us,
            ),
            1,
        )
        return {
            "name": self.config.name,
            "label": self.config.label,
            "channel": self.config.channel,
            "pulse_us": self.pulse_us,
            "angle_deg": round(
                (self.pulse_us - limits.center_us) / half_range_us * 90.0,
                1,
            ),
            "min_us": limits.min_us,
            "center_us": limits.center_us,
            "max_us": limits.max_us,
            "enabled": self.pwm.is_open,
        }


class ServoRig:
    """Thread-safe two-axis servo controller with a browser-session watchdog."""

    def __init__(
        self,
        pwm_factory: Callable[..., Any] = SysfsPwm,
        session_timeout: float = 5.0,
        limits: ServoLimits | None = None,
    ) -> None:
        servo_limits = limits or ServoLimits(min_us=500, center_us=1500, max_us=2500)
        self.axes = {
            "pan": ServoAxis(
                AxisConfig("pan", "水平", 3, servo_limits),
                pwm_factory,
            ),
            "tilt": ServoAxis(
                AxisConfig("tilt", "垂直", 4, servo_limits),
                pwm_factory,
            ),
        }
        self.session_timeout = session_timeout
        self._lock = RLock()
        self._session_token: str | None = None
        self._last_heartbeat = 0.0
        self._last_command = "startup_center"
        self._last_command_at = time.time()
        self._started = False

    def start(self) -> None:
        with self._lock:
            if self._started and all(axis.pwm.is_open for axis in self.axes.values()):
                return
            opened: list[ServoAxis] = []
            try:
                for axis in self.axes.values():
                    axis.open()
                    opened.append(axis)
            except Exception:
                for axis in reversed(opened):
                    axis.close()
                raise
            self._started = True
            self._set_command("startup_center")

    def close(self) -> None:
        with self._lock:
            self._session_token = None
            for axis in self.axes.values():
                axis.center() if axis.pwm.is_open else None
                axis.close()
            self._started = False

    def _set_command(self, name: str) -> None:
        self._last_command = name
        self._last_command_at = time.time()

    def _ensure_started(self) -> None:
        if not self._started:
            self.start()

    def create_session(self) -> str:
        with self._lock:
            self._ensure_started()
            self.center("new_session_center")
            self._session_token = secrets.token_urlsafe(24)
            self._last_heartbeat = time.monotonic()
            return self._session_token

    def _require_session(self, token: str | None) -> None:
        if not token or token != self._session_token:
            raise PermissionError("controller session is not active")

    def heartbeat(self, token: str | None) -> None:
        with self._lock:
            self._require_session(token)
            self._last_heartbeat = time.monotonic()

    def center(self, command_name: str = "center") -> None:
        with self._lock:
            self._ensure_started()
            for axis in self.axes.values():
                axis.center()
            self._set_command(command_name)

    def stop(self, command_name: str = "stop") -> None:
        with self._lock:
            for axis in self.axes.values():
                if axis.pwm.is_open:
                    axis.center()
                    axis.pwm.stop()
            self._started = False
            self._set_command(command_name)

    def command(
        self,
        token: str | None,
        axis_name: str,
        pulse_us: int | None = None,
        delta_us: int | None = None,
    ) -> int:
        with self._lock:
            self._require_session(token)
            self._ensure_started()
            if axis_name not in self.axes:
                raise ValueError(f"unknown axis: {axis_name}")
            if (pulse_us is None) == (delta_us is None):
                raise ValueError("provide exactly one of pulse_us or delta_us")
            axis = self.axes[axis_name]
            target = pulse_us if pulse_us is not None else axis.pulse_us + int(delta_us)
            target = max(axis.config.limits.min_us, min(axis.config.limits.max_us, target))
            result = axis.set_pulse(target)
            self._set_command(f"{axis_name}={result}us")
            return result

    def set_internal_pulse(self, axis_name: str, pulse_us: int) -> int:
        with self._lock:
            self._ensure_started()
            if axis_name not in self.axes:
                raise ValueError(f"unknown axis: {axis_name}")
            result = self.axes[axis_name].set_pulse(pulse_us)
            self._set_command(f"auto_{axis_name}={result}us")
            return result

    def check_watchdog(self, now: float | None = None) -> bool:
        with self._lock:
            if self._session_token is None:
                return False
            current = time.monotonic() if now is None else now
            if current - self._last_heartbeat <= self.session_timeout:
                return False
            self.stop("heartbeat_timeout")
            self._session_token = None
            self._last_heartbeat = 0.0
            return True

    def status(self) -> dict[str, Any]:
        with self._lock:
            return {
                "status": "connected" if self._session_token else "waiting",
                "session_active": self._session_token is not None,
                "pwm_running": any(axis.pwm.is_open for axis in self.axes.values()),
                "period_hz": 50,
                "last_command": self._last_command,
                "last_command_at": self._last_command_at,
                "session_timeout_s": self.session_timeout,
                "axes": {name: axis.status() for name, axis in self.axes.items()},
            }


class _ServoServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


class ServoRequestHandler(BaseHTTPRequestHandler):
    rig: ServoRig
    vision: FaceTrackingWorker

    def log_message(self, format: str, *args: Any) -> None:
        return

    def _send_json(self, payload: dict[str, Any], status: int = HTTPStatus.OK) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def _send_bytes(
        self,
        body: bytes,
        content_type: str,
        no_cache: bool = False,
    ) -> None:
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        if no_cache:
            self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _send_file(self, path: Path, content_type: str) -> None:
        try:
            body = path.read_bytes()
        except OSError:
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _read_json(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0"))
        if length > 8192:
            raise ValueError("request too large")
        body = self.rfile.read(length)
        if not body:
            return {}
        payload = json.loads(body.decode("utf-8"))
        if not isinstance(payload, dict):
            raise ValueError("JSON object required")
        return payload

    def _token(self) -> str | None:
        return self.headers.get("X-Servo-Session")

    def do_OPTIONS(self) -> None:
        self.send_response(HTTPStatus.NO_CONTENT)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, X-Servo-Session")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.end_headers()

    def do_GET(self) -> None:
        path = urlsplit(self.path).path
        if path == "/":
            self._send_file(UI_PATH, "text/html; charset=utf-8")
        elif path == "/servo_ui.js":
            self._send_file(UI_SCRIPT_PATH, "text/javascript; charset=utf-8")
        elif path == "/servo_ui.css":
            self._send_file(UI_STYLE_PATH, "text/css; charset=utf-8")
        elif path == "/api/status":
            payload = self.rig.status()
            payload["vision"] = self.vision.status()
            self._send_json(payload)
        elif path == "/vision/frame.jpg":
            frame = self.vision.frame()
            if not frame:
                self.send_error(HTTPStatus.SERVICE_UNAVAILABLE)
                return
            self._send_bytes(frame, "image/jpeg", no_cache=True)
        elif path == "/healthz":
            self._send_json({"status": "ok"})
        else:
            self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:
        path = urlsplit(self.path).path
        try:
            if path == "/api/session":
                self.vision.set_auto_enabled(False)
                self._send_json({"token": self.rig.create_session(), "status": self.rig.status()})
                return
            payload = self._read_json()
            token = self._token()
            if path == "/api/heartbeat":
                self.rig.heartbeat(token)
                self._send_json({"ok": True})
            elif path == "/api/command":
                pulse_us = payload.get("pulse_us")
                delta_us = payload.get("delta_us")
                result = self.rig.command(
                    token,
                    str(payload.get("axis", "")),
                    int(pulse_us) if pulse_us is not None else None,
                    int(delta_us) if delta_us is not None else None,
                )
                self._send_json({"ok": True, "pulse_us": result})
            elif path == "/api/center":
                self.rig._require_session(token)
                self.vision.set_auto_enabled(False)
                self.rig.center()
                self._send_json({"ok": True})
            elif path == "/api/stop":
                self.rig._require_session(token)
                self.vision.set_auto_enabled(False)
                self.rig.stop()
                self._send_json({"ok": True})
            elif path == "/api/tracking":
                self.rig._require_session(token)
                enabled = bool(payload.get("enabled", False))
                if enabled:
                    self.rig.start()
                self.vision.set_auto_enabled(enabled)
                self._send_json({"ok": True, "enabled": enabled})
            elif path == "/api/tracking/config":
                self.rig._require_session(token)
                config = self.vision.set_tracking_config(payload)
                self._send_json({"ok": True, "tracking_config": config})
            else:
                self.send_error(HTTPStatus.NOT_FOUND)
        except PermissionError as exc:
            self._send_json({"error": str(exc)}, HTTPStatus.FORBIDDEN)
        except (ValueError, TypeError, PwmError, OSError) as exc:
            self._send_json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)


def serve(
    host: str = "0.0.0.0",
    port: int = 8091,
    limits: ServoLimits | None = None,
    source: str = "rtsp://127.0.0.1:8554/camera",
    model: str = "vision/models/face_detection_yunet_2023mar.onnx",
) -> None:
    rig = ServoRig(limits=limits)
    rig.start()
    vision = FaceTrackingWorker(
        rig,
        VisionConfig(source=source, model=model),
    )
    vision.start()
    handler = type(
        "BoundServoRequestHandler",
        (ServoRequestHandler,),
        {"rig": rig, "vision": vision},
    )
    server = _ServoServer((host, port), handler)
    stop_event = Event()

    def watchdog() -> None:
        while not stop_event.wait(0.25):
            if rig.check_watchdog():
                vision.set_auto_enabled(False)

    watchdog_thread = Thread(target=watchdog, name="servo-session-watchdog", daemon=True)
    watchdog_thread.start()

    def shutdown(_signum: int, _frame: Any) -> None:
        stop_event.set()
        # HTTPServer.shutdown() must run from a thread other than
        # serve_forever(), otherwise systemd stop/restart can deadlock.
        Thread(target=server.shutdown, name="servo-server-shutdown", daemon=True).start()

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)
    print(f"servo server listening on http://{host}:{port}/", flush=True)
    try:
        server.serve_forever()
    finally:
        stop_event.set()
        server.server_close()
        vision.stop()
        rig.close()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Orange Pi two-servo browser controller")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8091)
    parser.add_argument("--min-us", type=int, default=500)
    parser.add_argument("--center-us", type=int, default=1500)
    parser.add_argument("--max-us", type=int, default=2500)
    parser.add_argument("--source", default="rtsp://127.0.0.1:8554/camera")
    parser.add_argument(
        "--model",
        default="vision/models/face_detection_yunet_2023mar.onnx",
    )
    args = parser.parse_args()
    serve(
        args.host,
        args.port,
        ServoLimits(args.min_us, args.center_us, args.max_us),
        args.source,
        args.model,
    )
