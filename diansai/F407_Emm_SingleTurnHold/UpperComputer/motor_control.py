import asyncio
import queue
import sys
import threading
import tkinter as tk
from datetime import datetime
from pathlib import Path
from tkinter import messagebox, ttk

try:
    from bleak import BleakClient, BleakScanner
except ImportError:
    BleakClient = None
    BleakScanner = None


DEVICE_NAME = "BT04-A"
UART_NOTIFY_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"
UART_WRITE_UUID = "0000ffe2-0000-1000-8000-00805f9b34fb"

MOTOR_STATES = {
    0: "上电等待",
    1: "使能电机2",
    2: "电机1回零",
    3: "电机2回零",
    4: "等待回零",
    5: "保持",
    6: "装载电机2",
    7: "同步启动",
    8: "运动中",
    9: "回正电机2",
    10: "回正同步",
    11: "回正中",
    12: "设置电机2",
    13: "设置同步",
}


def parse_status_line(line):
    parts = line.strip().split(",")
    if len(parts) != 6 or parts[0] != "STAT":
        return None
    try:
        values = [int(value) for value in parts[1:]]
    except ValueError:
        return None
    return {
        "actual_1": values[0] / 10.0,
        "actual_2": values[1] / 10.0,
        "target_1": values[2] / 10.0,
        "target_2": values[3] / 10.0,
        "state": values[4],
    }


class MotorControlApp:
    def __init__(self, root):
        self.root = root
        self.root.title("BT04-A 双电机控制")
        self.root.geometry("760x570")
        self.root.minsize(680, 520)

        self.events = queue.Queue()
        self.client = None
        self.notify_characteristic = None
        self.write_characteristic = None
        self.connected = False
        self.rx_buffer = bytearray()

        self.loop = asyncio.new_event_loop()
        self.loop_thread = threading.Thread(target=self._run_event_loop, daemon=True)
        self.loop_thread.start()

        self._configure_style()
        self._build_ui()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.after(50, self._poll_events)

    def _configure_style(self):
        style = ttk.Style()
        style.theme_use("vista")
        style.configure("Title.TLabel", font=("Microsoft YaHei UI", 16, "bold"))
        style.configure("Value.TLabel", font=("Consolas", 17, "bold"))
        style.configure("Status.TLabel", font=("Microsoft YaHei UI", 10, "bold"))
        style.configure("TButton", padding=(12, 7))

    def _build_ui(self):
        container = ttk.Frame(self.root, padding=18)
        container.pack(fill="both", expand=True)
        container.columnconfigure(0, weight=1)
        container.rowconfigure(4, weight=1)

        header = ttk.Frame(container)
        header.grid(row=0, column=0, sticky="ew", pady=(0, 14))
        header.columnconfigure(0, weight=1)
        ttk.Label(header, text="双电机蓝牙控制", style="Title.TLabel").grid(
            row=0, column=0, sticky="w"
        )
        self.connection_status = ttk.Label(
            header, text="未连接", foreground="#b42318", style="Status.TLabel"
        )
        self.connection_status.grid(row=0, column=1, padx=(12, 10))
        self.connect_button = ttk.Button(
            header, text="连接 BT04-A", command=self._toggle_connection
        )
        self.connect_button.grid(row=0, column=2)

        values = ttk.Frame(container)
        values.grid(row=1, column=0, sticky="ew", pady=(0, 14))
        values.columnconfigure((0, 1), weight=1, uniform="motor")
        self.actual_labels = []
        self.target_labels = []
        self.angle_vars = [tk.StringVar(value="30.0"), tk.StringVar(value="30.0")]

        for index in range(2):
            panel = ttk.LabelFrame(values, text=f"电机 {index + 1}", padding=14)
            panel.grid(row=0, column=index, sticky="nsew", padx=(0, 7) if index == 0 else (7, 0))
            panel.columnconfigure(1, weight=1)

            ttk.Label(panel, text="实际角度").grid(row=0, column=0, sticky="w")
            actual = ttk.Label(panel, text="--.-°", style="Value.TLabel")
            actual.grid(row=0, column=1, sticky="e")
            self.actual_labels.append(actual)

            ttk.Label(panel, text="当前目标").grid(row=1, column=0, sticky="w", pady=(8, 0))
            target = ttk.Label(panel, text="0.0°")
            target.grid(row=1, column=1, sticky="e", pady=(8, 0))
            self.target_labels.append(target)

            ttk.Label(panel, text="设置角度").grid(row=2, column=0, sticky="w", pady=(12, 0))
            spinbox = ttk.Spinbox(
                panel,
                from_=-360.0,
                to=360.0,
                increment=0.1,
                textvariable=self.angle_vars[index],
                width=12,
                justify="right",
            )
            spinbox.grid(row=2, column=1, sticky="e", pady=(12, 0))

        commands = ttk.Frame(container)
        commands.grid(row=2, column=0, sticky="ew", pady=(0, 12))
        commands.columnconfigure((0, 1, 2), weight=1, uniform="command")
        self.send_button = ttk.Button(
            commands, text="同步转到目标角度", command=self._send_targets, state="disabled"
        )
        self.send_button.grid(row=0, column=0, sticky="ew", padx=(0, 6))
        self.plus_button = ttk.Button(
            commands, text="两台目标 +30°", command=self._add_30, state="disabled"
        )
        self.plus_button.grid(row=0, column=1, sticky="ew", padx=6)
        self.zero_button = ttk.Button(
            commands, text="两台转到 0°", command=self._send_zero, state="disabled"
        )
        self.zero_button.grid(row=0, column=2, sticky="ew", padx=(6, 0))

        state_row = ttk.Frame(container)
        state_row.grid(row=3, column=0, sticky="ew", pady=(0, 10))
        ttk.Label(state_row, text="控制器状态：").pack(side="left")
        self.motor_state_label = ttk.Label(state_row, text="--")
        self.motor_state_label.pack(side="left")

        log_frame = ttk.LabelFrame(container, text="通信记录", padding=8)
        log_frame.grid(row=4, column=0, sticky="nsew")
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        self.log_text = tk.Text(
            log_frame,
            height=10,
            wrap="word",
            state="disabled",
            font=("Consolas", 9),
            background="#f7f7f7",
            relief="flat",
        )
        self.log_text.grid(row=0, column=0, sticky="nsew")
        scrollbar = ttk.Scrollbar(log_frame, command=self.log_text.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.log_text.configure(yscrollcommand=scrollbar.set)

    def _run_event_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def _submit(self, coroutine):
        return asyncio.run_coroutine_threadsafe(coroutine, self.loop)

    def _toggle_connection(self):
        if BleakScanner is None:
            messagebox.showerror("缺少组件", "请先安装 requirements.txt 中的 bleak。")
            return
        if self.connected:
            self.connect_button.configure(state="disabled")
            self._submit(self._disconnect())
        else:
            self.connect_button.configure(state="disabled")
            self.connection_status.configure(text="正在扫描...", foreground="#b54708")
            self._append_log("开始扫描 BT04-A")
            self._submit(self._scan_and_connect())

    async def _scan_and_connect(self):
        try:
            devices = await BleakScanner.discover(timeout=6.0)
            matches = [
                device for device in devices
                if device.name and DEVICE_NAME.upper() in device.name.upper()
            ]
            if not matches:
                self.events.put(("error", "没有扫描到 BT04-A，请确认模块已上电且未被其他设备连接。"))
                return

            device = matches[0]
            self.events.put(("log", f"发现 {device.name}，正在连接"))
            self.client = BleakClient(device, disconnected_callback=self._device_disconnected)
            await self.client.connect()

            notify_characteristic = self.client.services.get_characteristic(UART_NOTIFY_UUID)
            write_characteristic = self.client.services.get_characteristic(UART_WRITE_UUID)
            if notify_characteristic is None or write_characteristic is None:
                notify_characteristic, write_characteristic = self._find_uart_characteristics()
            if notify_characteristic is None or write_characteristic is None:
                raise RuntimeError("设备中没有找到蓝牙串口的通知和写入特征")

            self.notify_characteristic = notify_characteristic
            self.write_characteristic = write_characteristic
            await self.client.start_notify(notify_characteristic, self._notification_received)
            self.events.put(("connected", f"{device.name}  {device.address}"))
        except Exception as exc:
            if self.client is not None and self.client.is_connected:
                await self.client.disconnect()
            self.client = None
            self.notify_characteristic = None
            self.write_characteristic = None
            self.events.put(("error", f"连接失败：{exc}"))

    def _find_uart_characteristics(self):
        notify_characteristic = None
        write_characteristic = None
        for service in self.client.services:
            for characteristic in service.characteristics:
                properties = set(characteristic.properties)
                if notify_characteristic is None and "notify" in properties:
                    notify_characteristic = characteristic
                if write_characteristic is None and (
                    "write" in properties or "write-without-response" in properties
                ):
                    write_characteristic = characteristic
                if "notify" in properties and (
                    "write" in properties or "write-without-response" in properties
                ):
                    return characteristic, characteristic
        return notify_characteristic, write_characteristic

    async def _disconnect(self):
        try:
            if self.client is not None and self.client.is_connected:
                await self.client.disconnect()
        finally:
            self.client = None
            self.notify_characteristic = None
            self.write_characteristic = None
            self.events.put(("disconnected", "已断开"))

    def _device_disconnected(self, _client):
        self.events.put(("disconnected", "连接已断开"))

    def _notification_received(self, _sender, data):
        self.rx_buffer.extend(data)
        while b"\n" in self.rx_buffer:
            raw_line, _, remainder = self.rx_buffer.partition(b"\n")
            self.rx_buffer = bytearray(remainder)
            line = raw_line.rstrip(b"\r").decode("ascii", errors="replace")
            self.events.put(("line", line))

    def _read_angles(self):
        angles = []
        for index, variable in enumerate(self.angle_vars):
            try:
                angle = float(variable.get())
            except ValueError as exc:
                raise ValueError(f"电机 {index + 1} 角度不是有效数字") from exc
            if not -360.0 <= angle <= 360.0:
                raise ValueError(f"电机 {index + 1} 角度必须在 -360.0° 到 360.0° 之间")
            angles.append(angle)
        return angles

    def _send_targets(self):
        try:
            angles = self._read_angles()
        except ValueError as exc:
            messagebox.showerror("角度错误", str(exc))
            return
        angle_1 = round(angles[0] * 10)
        angle_2 = round(angles[1] * 10)
        self._send_command(f"SET,{angle_1},{angle_2}\r\n")

    def _add_30(self):
        try:
            angles = self._read_angles()
            angles = [angle + 30.0 for angle in angles]
            if any(angle > 360.0 for angle in angles):
                raise ValueError("目标加 30° 后超过 360.0°")
        except ValueError as exc:
            messagebox.showerror("角度错误", str(exc))
            return
        for variable, angle in zip(self.angle_vars, angles):
            variable.set(f"{angle:.1f}")
        self._send_targets()

    def _send_zero(self):
        self.angle_vars[0].set("0.0")
        self.angle_vars[1].set("0.0")
        self._send_targets()

    def _send_command(self, command):
        if not self.connected:
            messagebox.showwarning("未连接", "请先连接 BT04-A。")
            return
        self._append_log(f"发送  {command.strip()}")
        self._submit(self._write(command.encode("ascii")))

    async def _write(self, data):
        try:
            if (
                self.client is None
                or not self.client.is_connected
                or self.write_characteristic is None
            ):
                raise RuntimeError("蓝牙连接不可用")
            # 优先使用模块会明确确认的 Write Request。
            response = "write" in self.write_characteristic.properties
            await self.client.write_gatt_char(
                self.write_characteristic, data, response=response
            )
            self.events.put(("write_done", data.decode("ascii").strip()))
        except Exception as exc:
            self.events.put(("error", f"发送失败：{exc}"))

    def _poll_events(self):
        while True:
            try:
                event, value = self.events.get_nowait()
            except queue.Empty:
                break

            if event == "connected":
                self.connected = True
                self.connection_status.configure(text="已连接", foreground="#067647")
                self.connect_button.configure(text="断开", state="normal")
                self._set_command_state("normal")
                self._append_log(f"连接成功  {value}")
            elif event == "disconnected":
                self.connected = False
                self.connection_status.configure(text="未连接", foreground="#b42318")
                self.connect_button.configure(text="连接 BT04-A", state="normal")
                self._set_command_state("disabled")
                self._append_log(value)
            elif event == "error":
                self.connected = False
                self.connection_status.configure(text="连接失败", foreground="#b42318")
                self.connect_button.configure(text="重新连接", state="normal")
                self._set_command_state("disabled")
                self._append_log(value)
                messagebox.showerror("蓝牙错误", value)
            elif event == "log":
                self._append_log(value)
            elif event == "write_done":
                self._append_log(f"BLE 写入已确认  {value}")
            elif event == "line":
                self._handle_line(value)

        self.root.after(50, self._poll_events)

    def _handle_line(self, line):
        status = parse_status_line(line)
        if status is not None:
            self.actual_labels[0].configure(text=f"{status['actual_1']:.1f}°")
            self.actual_labels[1].configure(text=f"{status['actual_2']:.1f}°")
            self.target_labels[0].configure(text=f"{status['target_1']:.1f}°")
            self.target_labels[1].configure(text=f"{status['target_2']:.1f}°")
            state = status["state"]
            self.motor_state_label.configure(text=f"{state:02d}  {MOTOR_STATES.get(state, '未知')}")
        else:
            self._append_log(f"接收  {line}")

    def _set_command_state(self, state):
        self.send_button.configure(state=state)
        self.plus_button.configure(state=state)
        self.zero_button.configure(state=state)

    def _append_log(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.configure(state="normal")
        self.log_text.insert("end", f"[{timestamp}] {message}\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def _on_close(self):
        if self.client is not None and self.client.is_connected:
            future = self._submit(self._disconnect())
            try:
                future.result(timeout=2.0)
            except Exception:
                pass
        self.loop.call_soon_threadsafe(self.loop.stop)
        self.root.destroy()


def main():
    root = tk.Tk()
    MotorControlApp(root)
    root.mainloop()


async def packaged_ble_self_test():
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=8.0)
    if device is None:
        raise RuntimeError("没有扫描到 BT04-A")
    async with BleakClient(device) as client:
        notify_characteristic = client.services.get_characteristic(UART_NOTIFY_UUID)
        write_characteristic = client.services.get_characteristic(UART_WRITE_UUID)
        if notify_characteristic is None or write_characteristic is None:
            raise RuntimeError("没有找到 FFE1/FFE2 串口特征")
        return f"PASS {device.name} {device.address} FFE1_NOTIFY FFE2_WRITE"


def run_packaged_self_test():
    result_path = Path.cwd() / "BT04_Motor_Control_self_test.txt"
    try:
        result = asyncio.run(packaged_ble_self_test())
    except Exception as exc:
        result = f"FAIL {exc}"
    result_path.write_text(result, encoding="utf-8")
    return 0 if result.startswith("PASS") else 1


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        raise SystemExit(run_packaged_self_test())
    main()
