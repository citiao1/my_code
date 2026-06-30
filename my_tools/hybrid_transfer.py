import os
import socket
import logging
import json
import base64
from flask import Flask, request, jsonify, render_template_string
from datetime import datetime
from PIL import Image

# === 配置区 ===
UPLOAD_FOLDER = 'received_photos'
PORT = 5000

# === 初始化 ===
app = Flask(__name__)
logging.getLogger('werkzeug').setLevel(logging.ERROR)

if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

def get_host_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    except:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

# === HTML 模板 (集成了 Pro Max 的相机 UI) ===
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>极速传图 Pro (局域网版)</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; background: #121212; color: #fff; margin: 0; height: 100vh; display: flex; flex-direction: column; overflow: hidden; }
        
        /* === 电脑端样式 === */
        #pc-view { display: none; height: 100%; flex-direction: row; }
        .sidebar { width: 220px; background: #1e1e1e; border-right: 1px solid #333; padding: 10px; overflow-y: auto; flex-shrink: 0; }
        .history-item { margin-bottom: 10px; cursor: pointer; border: 2px solid transparent; height: 100px; border-radius: 6px; overflow: hidden; position: relative; background: #000; }
        .history-item.active { border-color: #0a84ff; }
        .history-item img { width: 100%; height: 100%; object-fit: cover; opacity: 0.7; transition: 0.2s; }
        .history-item:hover img { opacity: 1; }
        
        .main { flex: 1; padding: 20px; display: flex; flex-direction: column; align-items: center; justify-content: center; background: #2c2c2c; }
        .img-box { max-width: 95%; max-height: 70vh; border: 2px solid #333; background: #000; display: flex; justify-content: center; align-items: center; border-radius: 8px; overflow: hidden; position: relative; }
        #current-img { max-width: 100%; max-height: 100%; display: block; transition: transform 0.3s; }
        
        .btn-group { margin-top: 20px; display: flex; gap: 10px; justify-content: center; flex-wrap: wrap;}
        .btn { background: #0a84ff; color: white; border: none; padding: 12px 24px; border-radius: 12px; cursor: pointer; font-weight: bold; font-size: 16px; min-width: 100px; }
        .btn.secondary { background: #555; }
        .status-bar { margin-top: 15px; color: #aaa; font-size: 14px; }

        /* === 手机端样式 (复刻 Pro Max) === */
        #phone-view { display: none; padding: 15px; height: 100%; box-sizing: border-box; flex-direction: column; }
        .phone-card { background: #1e1e1e; padding: 15px; border-radius: 16px; height: 100%; display: flex; flex-direction: column; }
        
        /* 实时相机容器 */
        #camera-container { 
            position: relative; width: 100%; flex: 1; 
            background: #000; border-radius: 12px; overflow: hidden; 
            margin-bottom: 20px; border: 2px solid #333;
        }
        #camera-feed { width: 100%; height: 100%; object-fit: cover; }
        
        /* 错误/回退容器 */
        #fallback-container { display: none; flex: 1; flex-direction: column; justify-content: center; align-items: center; text-align: center; }
        
        .controls { display: flex; justify-content: center; gap: 30px; align-items: center; height: 100px; margin-bottom: 20px; }
        
        /* 快门按钮 */
        .shutter-btn { width: 75px; height: 75px; border-radius: 50%; background: white; border: 4px solid rgba(255,255,255,0.3); cursor: pointer; outline: none; }
        .shutter-btn:active { transform: scale(0.9); background: #ccc; }
        
        /* 相册按钮 */
        .album-btn { background: #333; padding: 12px 25px; border-radius: 30px; font-size: 14px; color: white; border: 1px solid #555; cursor: pointer; }
        
        /* 状态提示 */
        #upload-status { color: #4caf50; font-size: 14px; margin-bottom: 10px; font-weight: bold; text-align: center; height: 20px;}

    </style>
</head>
<body>

    <div id="pc-view">
        <div class="sidebar" id="sidebar"></div>
        <div class="main">
            <div class="img-box">
                <img id="current-img" src="" alt="等待图片...">
            </div>
            
            <div class="btn-group">
                <button class="btn secondary" onclick="rotate(-90)">↺ 左转</button>
                <button class="btn secondary" onclick="rotate(90)">↻ 右转</button>
                <button class="btn" id="copy-btn" onclick="copyImg()">📋 复制</button>
            </div>
            <div class="status-bar" id="status">✅ 服务运行中 | 等待连接...</div>
        </div>
    </div>

    <div id="phone-view">
        <div class="phone-card">
            <h2 style="margin: 0 0 15px 0; text-align: center;">📷 极速传图 Pro</h2>
            
            <div id="camera-container">
                <video id="camera-feed" autoplay playsinline muted></video>
            </div>

            <div id="fallback-container">
                <p style="color:#aaa; margin-bottom:30px;">
                    ⚠️ 局域网 HTTP 模式下<br>浏览器禁止直接调用摄像头<br>请使用下方按钮
                </p>
                <label class="shutter-btn" style="display:flex; align-items:center; justify-content:center; background:#ff5722; border:none;">
                    📷
                    <input type="file" accept="image/*" capture="environment" onchange="processFile(this.files[0])" style="display:none">
                </label>
                <p style="font-size:12px; color:#666;">点击上方橙色按钮拍照</p>
            </div>

            <div id="upload-status">准备拍摄</div>

            <div class="controls" id="pro-controls">
                <label class="album-btn">
                    🖼️ 相册
                    <input type="file" accept="image/*" onchange="processFile(this.files[0])" style="display:none">
                </label>
                <button class="shutter-btn" onclick="takePhoto()"></button>
            </div>
        </div>
    </div>

    <script type="module">
        import { initializeApp } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-app.js";
        import { getDatabase, ref, onValue } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-database.js";

        // 【🔴 请再次填入你的 Firebase 配置 🔴】
        const firebaseConfig = {
            apiKey: "你的apiKey",
            authDomain: "你的项目id.firebaseapp.com",
            databaseURL: "https://你的项目id-default-rtdb.firebaseio.com", 
            projectId: "你的项目id",
            storageBucket: "...",
            messagingSenderId: "...",
            appId: "..."
        };

        const isPC = window.innerWidth > 800;
        let rotation = 0;

        if (isPC) {
            initPC();
        } else {
            initPhone();
        }

        // =======================
        // 💻 电脑端逻辑
        // =======================
        function initPC() {
            document.getElementById('pc-view').style.display = 'flex';
            
            // 1. Firebase 监听 (远程)
            try {
                const app = initializeApp(firebaseConfig);
                const db = getDatabase(app);
                // 监听 history 列表的变更
                const historyRef = ref(db, 'history');
                
                onValue(historyRef, (snapshot) => {
                    const data = snapshot.val();
                    if (data) {
                        // 获取最新的一张图
                        const keys = Object.keys(data);
                        const latestKey = keys[keys.length - 1];
                        const base64Data = data[latestKey];
                        
                        // 发送给 Python 本地保存 (去重逻辑由后端简单处理或前端判断)
                        saveToLocal(base64Data, "Cloud_" + latestKey);
                    }
                });
                document.getElementById('status').innerText += " | ☁️ 云端连接成功";
            } catch(e) { console.error(e); }

            // 2. 本地轮询 (局域网)
            setInterval(checkLocalNew, 1000);
            loadHistory();
        }

        async function saveToLocal(base64Str, id) {
            // 简单检查是否已存在（避免重复下载）
            const current = document.getElementById('current-img');
            // 这里仅仅是触发后端保存，具体去重依赖文件名或其他逻辑
            await fetch('/save_base64', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({image: base64Str, id: id})
            });
        }

        async function checkLocalNew() {
            const res = await fetch('/check_new');
            const data = await res.json();
            const current = document.getElementById('current-img');
            
            // 如果最新文件变了，刷新界面
            if (data.latest && current.dataset.filename !== data.latest.filename) {
                loadHistory();
            }
        }

        async function loadHistory() {
            const res = await fetch('/history');
            const list = await res.json();
            const sidebar = document.getElementById('sidebar');
            sidebar.innerHTML = '';
            
            if (list.length > 0) {
                const latest = list[0];
                const img = document.getElementById('current-img');
                if (img.dataset.filename !== latest.filename) {
                    img.src = latest.url;
                    img.dataset.filename = latest.filename;
                    rotation = 0; img.style.transform = 'rotate(0deg)';
                }
            }

            list.forEach(item => {
                const div = document.createElement('div');
                div.className = 'history-item';
                if (document.getElementById('current-img').dataset.filename === item.filename) {
                    div.classList.add('active');
                }
                div.innerHTML = `<img src="${item.url}">`;
                div.onclick = () => {
                    const main = document.getElementById('current-img');
                    main.src = item.url;
                    main.dataset.filename = item.filename;
                    rotation = 0; main.style.transform = 'rotate(0deg)';
                    loadHistory(); // 刷新高亮
                };
                sidebar.appendChild(div);
            });
        }

        window.rotate = (deg) => {
            rotation += deg;
            document.getElementById('current-img').style.transform = `rotate(${rotation}deg)`;
        }

        window.copyImg = async () => {
            const filename = document.getElementById('current-img').dataset.filename;
            const btn = document.getElementById('copy-btn');
            const res = await fetch('/copy_to_clipboard', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({filename: filename})
            });
            if (res.ok) {
                btn.innerText = "✅ 已复制";
                btn.style.background = "#28a745";
                setTimeout(() => { btn.innerText = "📋 复制"; btn.style.background = "#0a84ff"; }, 2000);
            } else { alert("复制失败"); }
        }

        // =======================
        // 📱 手机端逻辑 (自动切换 WebRTC / 系统相机)
        // =======================
        function initPhone() {
            document.getElementById('phone-view').style.display = 'flex';
            startCamera();
        }

        async function startCamera() {
            const video = document.getElementById('camera-feed');
            const status = document.getElementById('upload-status');
            
            try {
                // 尝试启动高清、连续对焦的相机
                const constraints = {
                    video: { 
                        facingMode: "environment",
                        width: { ideal: 1920 },
                        height: { ideal: 1080 }
                    }
                };
                const stream = await navigator.mediaDevices.getUserMedia(constraints);
                video.srcObject = stream;
                status.innerText = "相机就绪 (局域网高清模式)";
            } catch (err) {
                console.warn("WebRTC 启动失败 (通常因为 HTTP 不安全限制):", err);
                // === 关键：启动失败，切换到回退界面 ===
                document.getElementById('camera-container').style.display = 'none';
                document.getElementById('pro-controls').style.display = 'none';
                document.getElementById('fallback-container').style.display = 'flex';
                status.innerText = "";
            }
        }

        window.takePhoto = function() {
            const video = document.getElementById('camera-feed');
            if (!video.srcObject) return;

            // 1. 截图
            const canvas = document.createElement('canvas');
            canvas.width = video.videoWidth;
            canvas.height = video.videoHeight;
            canvas.getContext('2d').drawImage(video, 0, 0);

            // 2. 压缩并上传
            canvas.toBlob(blob => {
                uploadBlob(blob);
            }, 'image/jpeg', 0.92); // 92% 质量
        };

        window.processFile = function(file) {
            if (!file) return;
            // 如果是文件选择（系统相机/相册），直接上传
            uploadBlob(file);
        }

        async function uploadBlob(blob) {
            if (!blob) return;
            const status = document.getElementById('upload-status');
            status.innerText = "🚀 正在传输...";
            
            const formData = new FormData();
            formData.append('file', blob, "photo.jpg");

            try {
                const res = await fetch('/upload', { method: 'POST', body: formData });
                const data = await res.json();
                if (data.status === 'ok') {
                    status.innerText = "✅ 发送成功！";
                    // 闪烁提示
                    const container = document.getElementById('camera-container');
                    if (container.style.display !== 'none') {
                        container.style.border = "4px solid #4caf50";
                        setTimeout(() => container.style.border = "2px solid #333", 500);
                    }
                }
            } catch (e) {
                status.innerText = "❌ 发送失败";
            }
        }
    </script>
</body>
</html>
"""

# === Python 后端 ===
@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/upload', methods=['POST'])
def upload():
    file = request.files.get('file')
    if file:
        save_local_file(file)
        return jsonify({'status': 'ok'})
    return jsonify({'status': 'error'})

@app.route('/save_base64', methods=['POST'])
def save_base64():
    data = request.json
    base64_str = data.get('image')
    # 简单的防止重复保存逻辑 (实际使用可能需要更复杂的 ID 判断)
    # 这里我们每次都存，反正硬盘大
    if base64_str:
        try:
            if "," in base64_str:
                header, encoded = base64_str.split(",", 1)
            else:
                encoded = base64_str
            file_data = base64.b64decode(encoded)
            
            filename = "Cloud_" + datetime.now().strftime("%H%M%S") + ".jpg"
            filepath = os.path.join(UPLOAD_FOLDER, filename)
            
            # 只有当文件不存在时才写入（防止重复刷新重复写）
            if not os.path.exists(filepath):
                with open(filepath, "wb") as f:
                    f.write(file_data)
                try_clipboard(filepath)
        except Exception as e:
            print("Base64保存错误:", e)
            
    return jsonify({'status': 'ok'})

def save_local_file(file):
    filename = "LAN_" + datetime.now().strftime("%Y%m%d_%H%M%S") + ".jpg"
    filepath = os.path.join(UPLOAD_FOLDER, filename)
    file.save(filepath)
    try_clipboard(filepath)

def try_clipboard(filepath):
    # 尝试调用系统剪贴板 (Windows)
    try:
        import win32clipboard
        from io import BytesIO
        image = Image.open(filepath)
        output = BytesIO()
        image.convert("RGB").save(output, "BMP")
        data = output.getvalue()[14:]
        output.close()
        win32clipboard.OpenClipboard()
        win32clipboard.EmptyClipboard()
        win32clipboard.SetClipboardData(win32clipboard.CF_DIB, data)
        win32clipboard.CloseClipboard()
    except:
        pass

@app.route('/history')
def history_api():
    files = sorted(os.listdir(UPLOAD_FOLDER), reverse=True)
    files = [f for f in files if f.endswith(('.jpg', '.png'))][:12]
    return jsonify([{'filename': f, 'url': f'/img/{f}'} for f in files])

@app.route('/check_new')
def check_new():
    files = sorted(os.listdir(UPLOAD_FOLDER), reverse=True)
    if not files: return jsonify({})
    return jsonify({'latest': {'filename': files[0]}})

@app.route('/img/<path:filename>')
def serve_img(filename):
    from flask import send_from_directory
    return send_from_directory(UPLOAD_FOLDER, filename)

@app.route('/copy_to_clipboard', methods=['POST'])
def api_copy():
    data = request.json
    filename = data.get('filename')
    filepath = os.path.join(UPLOAD_FOLDER, filename)
    try_clipboard(filepath)
    return jsonify({'status': 'ok'})

if __name__ == '__main__':
    host_ip = get_host_ip()
    print(f"✅ 服务已启动 (双模版)")
    print(f"👉 局域网访问: http://{host_ip}:{PORT}")
    app.run(host='0.0.0.0', port=PORT, debug=False)