import subprocess
import time
import sys
import cv2
import numpy as np
import mss
import pygetwindow as gw
import random
from ultralytics import YOLO

def start_stream():
    print("[系统] 正在启动串流管道...")
    command = [
        "scrcpy",
        "--max-fps", "30",
        "--video-bit-rate", "2M",
        "--no-audio",
        "--max-size", "1024",
        "--window-title", "PySyncVision"
    ]
    try:
        process = subprocess.Popen(command)
        return process
    except FileNotFoundError:
        sys.exit(1)

def get_phone_resolution():
    try:
        print("[系统] 正在探测物理设备分辨率...")
        result = subprocess.run(["adb", "shell", "wm", "size"], capture_output=True, text=True)
        size_str = result.stdout.strip().split(": ")[1]
        width, height = map(int, size_str.split("x"))
        print(f"[系统] 探测成功！手机物理分辨率: {width} x {height}")
        return width, height
    except Exception as e:
        print(f"[警告] 探测失败，使用默认值 1080x2400. 报错: {e}")
        return 1080, 2400

def vision_loop():
    print("[视觉] 等待投屏窗口出现...")
    time.sleep(2)

    windows = gw.getWindowsWithTitle("PySyncVision")
    if not windows:
        print("[错误] 找不到投屏窗口！")
        return
    win = windows[0]

    phone_w, phone_h = get_phone_resolution()

    # ================= 赛博换脑 =================
    print("\n[系统] 正在接入 YOLOv8 赛博大脑...")
    try:
        # 加载你刚刚炼好的模型
        model = YOLO("best.pt") 
        print(f"[系统] 大脑加载成功！AI 当前认识的类别有: {model.names}")
    except Exception as e:
        print(f"[致命错误] 加载 best.pt 失败！请检查文件是否在当前目录下。报错: {e}")
        return
    # ============================================

    last_click_time = time.time()  # 初始化为当前时间，避免出现负十几亿的 Bug
    cooldown = 2.5

    with mss.mss() as sct:
        print("\n=======================================")
        print("🐒 大圣游沙盘 (Monkey Test) 已激活！")
        print("⚠️ 警告: AI 即将接管手机屏幕！")
        print("🛑 随时在控制台按 Ctrl+C 紧急停止")
        print("=======================================\n")
        
        last_time = time.time()
        
        while True:
            monitor = {
                "top": win.top + 30,  
                "left": win.left + 8,
                "width": win.width - 16,
                "height": win.height - 38
            }

            try:
                img = np.array(sct.grab(monitor))
            except Exception:
                break
                
            frame = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
            current_time = time.time()

            # --- 核心推理与决策 ---
            # 1. 把全局底线拉到最低 (0.1)，让大圣把看到的所有东西都先汇报上来
            results = model(frame, conf=0.1, iou=0.4, verbose=False)
            boxes = results[0].boxes
            
            detected_objects = []
            
            for box in boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0].tolist())
                cls_id = int(box.cls[0].item())
                conf = box.conf[0].item()
                name = model.names[cls_id].lower() 
                
                # ==================================================
                # 2. 核心手术：对不同类别的物品实行“双标”过滤
                # ==================================================
                if "button" in name or "butten" in name:
                    if conf < 0.5:  
                        continue  # 普通按钮要求严格：低于 50% 把握的直接无视，防止屏幕上全是绿框
                
                elif "close" in name:
                    if conf < 0.12: 
                        continue  # 弹窗 X 号极度危险：只要有 12% 的把握，宁可错杀绝不放过！
                
                elif "back" in name:
                    if conf < 0.15:
                        continue  # 返回键：给 15% 的把握门槛
                # ==================================================
                
                center_x = (x1 + x2) // 2
                center_y = (y1 + y2) // 2
                
                detected_objects.append({
                    "name": name,
                    "center": (center_x, center_y),
                    "box": (x1, y1, x2, y2)
                })
                
                # 给符合门槛的真正目标画框
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                # 框上显示名字和置信度，方便你观察
                cv2.putText(frame, f"{name} {conf:.2f}", (x1, y1 - 10), 
                            cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            # --- AI 状态机 (决策逻辑树) ---
            # --- AI 状态机 (决策逻辑树) ---
            if len(detected_objects) > 0:
                # 把 AI 脑子里的名字毫无保留地提取出来，加个单引号看看到底有没有隐藏的空格！
                seen_names = [f"'{obj['name']}'" for obj in detected_objects]
                print(f"[Debug] 👀 AI当前视野: {seen_names}")
                
                if current_time - last_click_time > cooldown:
                    target_to_click = None
                    
                    # 极其宽松的匹配模式
                    closes = [obj for obj in detected_objects if "close" in obj["name"] or "关闭" in obj["name"]]
                    backs = [obj for obj in detected_objects if "back" in obj["name"] or "返回" in obj["name"]]
                    buttons = [obj for obj in detected_objects if "button" in obj["name"] or "butten" in obj["name"] or "按钮" in obj["name"]]
                    
                    if closes:
                        target_to_click = closes[0]
                        print("🔪 [决策] 发现阻挡弹窗，正在关闭...")
                    elif buttons:
                        import random
                        target_to_click = random.choice(buttons)
                        print(f"🎲 [决策] 发现 {len(buttons)} 个按钮，准备探索...")
                    elif backs:
                        target_to_click = backs[0]
                        print("🔙 [决策] 点击返回...")
                    else:
                        # 【终极后门：盲点模式】如果名字全都对不上，大圣就随便抓一个强制点击！
                        import random
                        target_to_click = random.choice(detected_objects)
                        print(f"🐒 [盲点模式] 名字没对上号！不管了，强制随机点击目标: '{target_to_click['name']}'")

                    # 执行物理点击
                    if target_to_click:
                        cx, cy = target_to_click["center"]
                        real_x = int((cx / monitor["width"]) * phone_w)
                        real_y = int((cy / monitor["height"]) * phone_h)
                        
                        cmd = ["adb", "shell", "input", "tap", str(real_x), str(real_y)]
                        print(f"🚀 [动作] 执行跨端点击坐标: ({real_x}, {real_y})")
                        
                        subprocess.run(cmd)
                        
                        # 点击成功后，立刻重置冷却时间，这才是正常的循环！
                        last_click_time = current_time 
                        cv2.circle(frame, (cx, cy), 15, (0, 0, 255), -1)
            # ---------------------------

            fps = 1 / (current_time - last_time)
            last_time = current_time
            cv2.putText(frame, f"FPS: {int(fps)}", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

            display_frame = cv2.resize(frame, (0, 0), fx=0.5, fy=0.5)
            cv2.imshow("What Python Sees", display_frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

if __name__ == "__main__":
    stream_process = start_stream()
    try:
        vision_loop()
    except KeyboardInterrupt:
        print("\n[系统] 接收到终止信号...")
    finally:
        cv2.destroyAllWindows()
        stream_process.terminate()
        stream_process.wait()
        print("[系统] PySyncVision 已安全下线。")

if __name__ == "__main__":
    stream_process = start_stream()
    try:
        vision_loop()
    except KeyboardInterrupt:
        print("\n[系统] 接收到终止信号...")
    finally:
        cv2.destroyAllWindows()
        stream_process.terminate()
        stream_process.wait()
        print("[系统] PySyncVision 已安全下线。")