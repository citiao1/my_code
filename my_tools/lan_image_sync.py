import threading
import time
import io
import hashlib
import socket
import ctypes
import os
from PIL import Image, ImageTk, ImageGrab, ImageDraw
from flask import Flask, send_file, jsonify
import customtkinter as ctk
import pystray # 引入系统托盘库
from pystray import MenuItem as item

# --- 1. 基础配置与后端逻辑 ---

ctk.set_appearance_mode("System") 
ctk.set_default_color_theme("blue")

PORT = 5000
app = Flask(__name__)

# 全局状态
class GlobalState:
    img_bytes = None
    img_hash = "init"
    server_running = True
    local_ip = "127.0.0.1"

state = GlobalState()

def get_host_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "127.0.0.1"

def monitor_clipboard(update_callback):
    while state.server_running:
        try:
            img = ImageGrab.grabclipboard()
            if isinstance(img, Image.Image): 
                img_buffer = io.BytesIO()
                img.save(img_buffer, format='PNG')
                current_bytes = img_buffer.getvalue()
                new_hash = hashlib.md5(current_bytes).hexdigest()
                
                if new_hash != state.img_hash:
                    state.img_hash = new_hash
                    state.img_bytes = current_bytes
                    update_callback(img)
                    print(f"新图片捕获: {len(current_bytes)/1024:.1f} KB")
        except:
            pass
        time.sleep(1.5)

@app.route('/check')
def check():
    return jsonify({"hash": state.img_hash, "timestamp": int(time.time())})

@app.route('/download')
def download():
    if state.img_bytes:
        return send_file(io.BytesIO(state.img_bytes), mimetype='image/png')
    return "", 404

def run_flask():
    import logging
    log = logging.getLogger('werkzeug')
    log.setLevel(logging.ERROR)
    app.run(host='0.0.0.0', port=PORT, debug=False, use_reloader=False)

# --- 2. 托盘与 GUI 逻辑 ---

class ModernSyncApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        # 窗口设置
        self.title("剪贴板同步助手")
        self.geometry("400x520")
        self.resizable(False, False)
        
        # === 核心修改：拦截关闭事件 ===
        self.protocol("WM_DELETE_WINDOW", self.hide_window)
        
        state.local_ip = get_host_ip()
        self.setup_ui()
        self.start_services()
        
        # 启动托盘图标
        self.init_tray_icon()

    def setup_ui(self):
        # (保持原有的 UI 代码不变)
        self.grid_columnconfigure(0, weight=1)
        self.grid_rowconfigure(2, weight=1)

        # 1. Header
        self.header_frame = ctk.CTkFrame(self, corner_radius=0, fg_color="transparent")
        self.header_frame.grid(row=0, column=0, sticky="ew", padx=20, pady=(20, 10))
        
        self.title_label = ctk.CTkLabel(self.header_frame, text="ClipSync Pro", font=ctk.CTkFont(size=24, weight="bold"))
        self.title_label.pack(side="left")
        
        self.status_indicator = ctk.CTkLabel(self.header_frame, text="● 运行中", text_color="#2CC069", font=ctk.CTkFont(size=12, weight="bold"))
        self.status_indicator.pack(side="right", pady=5)

        # 2. IP Card
        self.info_card = ctk.CTkFrame(self, corner_radius=15)
        self.info_card.grid(row=1, column=0, sticky="ew", padx=20, pady=10)
        
        ctk.CTkLabel(self.info_card, text="平板连接地址", text_color="gray", font=ctk.CTkFont(size=12)).pack(pady=(15, 0))
        self.ip_label = ctk.CTkLabel(self.info_card, text=state.local_ip, font=ctk.CTkFont(family="Arial", size=28, weight="bold"), text_color=("#1a73e8", "#64b5f6"))
        self.ip_label.pack(pady=5)
        ctk.CTkLabel(self.info_card, text=f"端口: {PORT}", text_color="gray", font=ctk.CTkFont(size=12)).pack(pady=(0, 15))

        # 3. Preview
        self.preview_frame = ctk.CTkFrame(self, corner_radius=15)
        self.preview_frame.grid(row=2, column=0, sticky="nsew", padx=20, pady=10)
        
        ctk.CTkLabel(self.preview_frame, text="当前剪贴板", font=ctk.CTkFont(size=14, weight="bold")).pack(pady=10, anchor="w", padx=15)
        self.image_label = ctk.CTkLabel(self.preview_frame, text="\n\n等待截图...\n\n(点击 X 即可最小化到托盘)", text_color="gray", font=ctk.CTkFont(size=14))
        self.image_label.pack(expand=True, fill="both", padx=10, pady=(0, 10))

        # 4. Button (改为隐藏到后台)
        self.hide_btn = ctk.CTkButton(self, text="隐藏到后台 (不退出)", fg_color="#607D8B", hover_color="#455A64", command=self.hide_window)
        self.hide_btn.grid(row=3, column=0, pady=20)

    # --- 托盘图标相关逻辑 ---

    def create_icon_image(self):
        """用代码画一个简单的图标 (避免依赖外部 .ico 文件)"""
        width = 64
        height = 64
        color1 = (0, 122, 255) # 蓝色
        color2 = (255, 255, 255) # 白色
        
        image = Image.new('RGB', (width, height), color1)
        dc = ImageDraw.Draw(image)
        # 画个简单的 C 字母代表 ClipSync
        dc.arc([10, 10, 54, 54], 45, 315, fill=color2, width=8)
        return image

    def init_tray_icon(self):
        """初始化托盘图标 (在独立线程运行)"""
        # 定义菜单
        menu = (
            item('显示主界面', self.show_window, default=True), # default=True表示双击触发
            item('彻底退出', self.quit_app)
        )
        
        icon_img = self.create_icon_image()
        self.tray_icon = pystray.Icon("ClipSync", icon_img, "剪贴板同步助手", menu)
        
        # 托盘图标必须在独立线程运行，否则会阻塞 GUI
        threading.Thread(target=self.tray_icon.run, daemon=True).start()

    def hide_window(self):
        """隐藏窗口"""
        self.withdraw() # 隐藏 Tkinter 窗口
        # 这里的 notify 是可选的，有些系统可能不支持
        try:
            self.tray_icon.notify("程序已缩至后台，右键图标可退出", "剪贴板同步")
        except:
            pass

    def show_window(self, icon=None, item=None):
        """显示窗口"""
        # 必须在主线程操作 UI，使用 after 确保线程安全
        self.after(0, self.deiconify)
        self.after(0, self.lift) # 把它置顶一下

    def quit_app(self, icon=None, item=None):
        """彻底退出"""
        state.server_running = False
        self.tray_icon.stop() # 停止托盘
        self.quit() # 停止 GUI 循环
        self.destroy() # 销毁窗口
        os._exit(0) # 强制杀进程

    # --- 业务逻辑 ---

    def update_preview(self, pil_image):
        max_w, max_h = 320, 180
        pil_image.thumbnail((max_w, max_h), Image.Resampling.LANCZOS)
        ctk_img = ctk.CTkImage(light_image=pil_image, dark_image=pil_image, size=pil_image.size)
        self.image_label.configure(image=ctk_img, text="")
        self.image_label.image = ctk_img

    def start_services(self):
        t1 = threading.Thread(target=run_flask, daemon=True)
        t1.start()
        t2 = threading.Thread(target=monitor_clipboard, args=(self.safe_update,), daemon=True)
        t2.start()

    def safe_update(self, image):
        self.after(0, self.update_preview, image)

if __name__ == "__main__":
    # High DPI 适配
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except:
        pass
        
    app = ModernSyncApp()
    app.mainloop()