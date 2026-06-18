from __future__ import annotations

import os
from pathlib import Path
import socket
import sys
import threading
import time
import urllib.request
import webbrowser


def resource_path(relative_path: str) -> Path:
    base = Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent))
    return base / relative_path


def find_free_port(start: int = 8501, end: int = 8599) -> int:
    for port in range(start, end + 1):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(0.2)
            if sock.connect_ex(("127.0.0.1", port)) != 0:
                return port
    raise RuntimeError("没有找到可用端口，请关闭部分本地服务后重试。")


def wait_until_ready(url: str, timeout: int = 45) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with urllib.request.urlopen(url, timeout=2) as response:
                if response.status == 200:
                    return True
        except Exception:
            time.sleep(0.6)
    return False


def main() -> int:
    app_file = resource_path("app.py")
    if not app_file.exists():
        print(f"找不到应用文件：{app_file}")
        return 1

    port = find_free_port()
    url = f"http://127.0.0.1:{port}"
    env = os.environ.copy()
    env["STREAMLIT_GLOBAL_DEVELOPMENT_MODE"] = "false"
    env["STREAMLIT_SERVER_PORT"] = str(port)
    env["STREAMLIT_SERVER_HEADLESS"] = "true"
    env["STREAMLIT_BROWSER_GATHER_USAGE_STATS"] = "false"
    os.environ.update(env)

    def open_browser_when_ready() -> None:
        if wait_until_ready(url):
            webbrowser.open(url)
            print(f"A股行情与舆情分析系统已启动：{url}")
            print("关闭这个窗口即可停止系统。")
        else:
            print("系统启动超时，请检查电脑网络或依赖环境。")

    threading.Thread(target=open_browser_when_ready, daemon=True).start()
    sys.argv = [
        "streamlit",
        "run",
        str(app_file),
        "--global.developmentMode",
        "false",
        "--server.headless",
        "true",
        "--browser.gatherUsageStats",
        "false",
    ]
    from streamlit.web.cli import main as streamlit_main

    streamlit_main()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
