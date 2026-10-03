from __future__ import annotations

import json
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Condition, Thread
from typing import Any
from urllib.parse import urlsplit


MONITOR_PAGE = """<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Orange Pi Face Monitor</title>
  <style>
    :root { color-scheme: dark; font-family: system-ui, sans-serif; }
    body { margin: 0; background: #111827; color: #e5e7eb; }
    main { max-width: 980px; margin: 0 auto; padding: 16px; }
    h1 { font-size: 20px; margin: 0 0 12px; }
    .video { background: #000; border: 1px solid #374151; }
    img { display: block; width: 100%; height: auto; }
    .stats { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; margin-top: 12px; }
    .stat { padding: 10px; background: #1f2937; border: 1px solid #374151; }
    .label { color: #9ca3af; font-size: 12px; }
    .value { font-size: 20px; margin-top: 4px; }
    @media (max-width: 640px) { .stats { grid-template-columns: repeat(2, 1fr); } }
  </style>
</head>
<body>
  <main>
    <h1>人脸跟踪监控</h1>
    <div class="video"><img id="video" alt="face tracking stream"></div>
    <section class="stats">
      <div class="stat"><div class="label">状态</div><div class="value" id="status">连接中</div></div>
      <div class="stat"><div class="label">FPS</div><div class="value" id="fps">-</div></div>
      <div class="stat"><div class="label">dx</div><div class="value" id="dx">-</div></div>
      <div class="stat"><div class="label">dy</div><div class="value" id="dy">-</div></div>
    </section>
  </main>
  <script>
    const fields = {
      status: document.querySelector("#status"),
      fps: document.querySelector("#fps"),
      dx: document.querySelector("#dx"),
      dy: document.querySelector("#dy"),
    };
    const video = document.querySelector("#video");
    let previousUrl = null;
    async function pullLatestFrame() {
      try {
        const response = await fetch("/frame.jpg?t=" + Date.now(), {
          cache: "no-store",
        });
        if (!response.ok) throw new Error("frame request failed");
        const blob = await response.blob();
        const nextUrl = URL.createObjectURL(blob);
        video.onload = () => {
          if (previousUrl) URL.revokeObjectURL(previousUrl);
          previousUrl = nextUrl;
        };
        video.src = nextUrl;
      } catch (_) {
        // The next pull retries after a short delay.
      } finally {
        setTimeout(pullLatestFrame, 35);
      }
    }
    async function refresh() {
      try {
        const response = await fetch("/api/status", { cache: "no-store" });
        const data = await response.json();
        fields.status.textContent = data.status === "face" ? "检测到人脸" : "未检测到";
        fields.fps.textContent = Number(data.fps || 0).toFixed(1);
        fields.dx.textContent = data.offset_px ? data.offset_px[0] : "-";
        fields.dy.textContent = data.offset_px ? data.offset_px[1] : "-";
      } catch (_) {
        fields.status.textContent = "连接中断";
      }
    }
    pullLatestFrame();
    refresh();
    setInterval(refresh, 250);
  </script>
</body>
</html>
"""


class MonitorState:
    def __init__(self) -> None:
        self._condition = Condition()
        self._frame_id = 0
        self._jpeg = b""
        self._event: dict[str, Any] = {"status": "starting"}

    def update(self, jpeg: bytes, event: dict[str, Any]) -> None:
        with self._condition:
            self._frame_id += 1
            self._jpeg = jpeg
            self._event = event
            self._condition.notify_all()

    def wait_for_frame(
        self,
        previous_frame_id: int,
        timeout: float = 5.0,
    ) -> tuple[int, bytes]:
        with self._condition:
            if self._frame_id <= previous_frame_id:
                self._condition.wait(timeout)
            return self._frame_id, self._jpeg

    def event(self) -> dict[str, Any]:
        with self._condition:
            return dict(self._event)

    def snapshot(self) -> tuple[int, bytes]:
        with self._condition:
            return self._frame_id, self._jpeg


class _MonitorHandler(BaseHTTPRequestHandler):
    state: MonitorState

    def log_message(self, format: str, *args: Any) -> None:
        return

    def do_GET(self) -> None:
        path = urlsplit(self.path).path
        if path == "/":
            self._send_bytes(MONITOR_PAGE.encode("utf-8"), "text/html; charset=utf-8")
            return
        if path == "/api/status":
            payload = json.dumps(
                self.state.event(),
                ensure_ascii=False,
            ).encode("utf-8")
            self._send_bytes(payload, "application/json; charset=utf-8", no_cache=True)
            return
        if path == "/frame.jpg":
            _, jpeg = self.state.snapshot()
            if not jpeg:
                self.send_error(HTTPStatus.SERVICE_UNAVAILABLE)
                return
            self._send_bytes(jpeg, "image/jpeg", no_cache=True)
            return
        if path == "/stream.mjpg":
            self._stream_mjpeg()
            return
        if path == "/healthz":
            self._send_bytes(b"ok\n", "text/plain; charset=utf-8")
            return
        self.send_error(HTTPStatus.NOT_FOUND)

    def _send_bytes(
        self,
        payload: bytes,
        content_type: str,
        no_cache: bool = False,
    ) -> None:
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        if no_cache:
            self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)

    def _stream_mjpeg(self) -> None:
        self.send_response(HTTPStatus.OK)
        self.send_header("Cache-Control", "no-cache, private")
        self.send_header("Pragma", "no-cache")
        self.send_header("X-Accel-Buffering", "no")
        self.send_header("Connection", "close")
        self.send_header(
            "Content-Type",
            "multipart/x-mixed-replace; boundary=frame",
        )
        self.end_headers()

        frame_id = 0
        try:
            while True:
                next_frame_id, jpeg = self.state.wait_for_frame(frame_id)
                if not jpeg or next_frame_id == frame_id:
                    continue
                frame_id = next_frame_id
                self.wfile.write(
                    b"--frame\r\n"
                    b"Content-Type: image/jpeg\r\n"
                    + f"Content-Length: {len(jpeg)}\r\n\r\n".encode("ascii")
                    + jpeg
                    + b"\r\n"
                )
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            return


class _Server(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def start_monitor_server(
    state: MonitorState,
    host: str,
    port: int,
) -> tuple[ThreadingHTTPServer, Thread]:
    handler = type("MonitorHandler", (_MonitorHandler,), {"state": state})
    server = _Server((host, port), handler)
    thread = Thread(
        target=server.serve_forever,
        name="face-monitor-http",
        daemon=True,
    )
    thread.start()
    return server, thread
