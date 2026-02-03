import time
import threading
import io
import hashlib
import socket
import json
from flask import Flask, send_file, jsonify, Response
from PIL import ImageGrab

app = Flask(__name__)

# 全局变量
global_img_bytes = None
global_img_hash = "init"
PORT = 5000

# --- 前端代码 (HTML + JS) ---
# 核心逻辑：JS定时询问服务器，有新图则更新<img>标签并触发<a>标签下载
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="zh">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>剪贴板自动同步</title>
    <style>
        body { 
            font-family: -apple-system, sans-serif; 
            background: #1e1e1e; 
            color: #fff;
            display: flex; 
            flex-direction: column; 
            align-items: center; 
            justify-content: center; 
            height: 100vh; 
            margin: 0; 
            overflow: hidden;
        }
        .container {
            text-align: center;
            width: 90%;
        }
        #status {
            color: #4CAF50;
            margin-bottom: 20px;
            font-size: 14px;
        }
        img { 
            max-width: 100%; 
            max-height: 70vh; 
            border-radius: 12px; 
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            border: 2px solid #333;
            transition: transform 0.3s;
        }
        .log {
            margin-top: 20px;
            font-size: 12px;
            color: #888;
        }
    </style>
</head>
<body>
    <div class="container">
        <div id="status">正在监听电脑剪贴板...</div>
        <img id="clipboard-img" src="" alt="等待图片..." style="display:none;">
        <div class="log" id="log-msg">暂无新图片</div>
    </div>

    <script>
        let lastHash = "init";
        
        // 定时检查函数 (每1秒一次)
        setInterval(checkUpdate, 1000);

        function checkUpdate() {
            fetch('/check_status')
                .then(response => response.json())
                .then(data => {
                    // 如果服务器有图，且哈希值不等于当前显示的图
                    if (data.has_image && data.hash !== lastHash) {
                        console.log("检测到新图片: " + data.hash);
                        updateImage(data.hash, data.timestamp);
                    }
                })
                .catch(err => {
                    document.getElementById('status').innerText = "连接断开，正在重连...";
                    document.getElementById('status').style.color = "red";
                });
        }

        function updateImage(newHash, timestamp) {
            const imgUrl = '/image?t=' + timestamp;
            const imgObj = document.getElementById('clipboard-img');
            const logObj = document.getElementById('log-msg');
            const statusObj = document.getElementById('status');

            // 1. 更新显示的图片
            imgObj.src = imgUrl;
            imgObj.style.display = 'block';
            imgObj.onload = () => {
                statusObj.innerText = "已同步最新图片";
                statusObj.style.color = "#4CAF50";
            };

            // 2. 触发自动下载
            downloadImage(imgUrl, 'clip_' + timestamp + '.png');

            // 3. 更新本地记录
            lastHash = newHash;
            logObj.innerText = "已下载: clip_" + timestamp + ".png";
        }

        function downloadImage(url, filename) {
            // 创建一个隐藏的<a>标签并模拟点击
            const link = document.createElement('a');
            link.href = url;
            link.download = filename;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }
    </script>
</body>
</html>
"""

def get_host_ip():
    """自动获取本机IP"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

def monitor_clipboard():
    """剪贴板监听线程"""
    global global_img_bytes, global_img_hash
    print(" [INFO] 剪贴板监听中...")
    
    while True:
        try:
            img = ImageGrab.grabclipboard()
            if img and not isinstance(img, list):
                # 图片转字节
                img_buffer = io.BytesIO()
                img.save(img_buffer, format='PNG')
                current_bytes = img_buffer.getvalue()
                
                # 计算哈希
                new_hash = hashlib.md5(current_bytes).hexdigest()
                
                if new_hash != global_img_hash:
                    global_img_hash = new_hash
                    global_img_bytes = current_bytes
                    print(f" [New] 捕获新图片，准备发送...")
        except:
            pass
        time.sleep(1)

# --- 路由接口 ---

@app.route('/')
def index():
    """返回前端网页"""
    return HTML_TEMPLATE

@app.route('/check_status')
def check_status():
    """前端JS每秒调用的接口，只返回状态json，不消耗流量"""
    return jsonify({
        "has_image": (global_img_bytes is not None),
        "hash": global_img_hash,
        "timestamp": int(time.time())
    })

@app.route('/image')
def get_image():
    """下载图片的接口"""
    if global_img_bytes:
        return send_file(
            io.BytesIO(global_img_bytes), 
            mimetype='image/png',
            as_attachment=True, # 强制浏览器识别为附件下载
            download_name=f'clip_{int(time.time())}.png'
        )
    return "", 404

if __name__ == '__main__':
    host_ip = get_host_ip()
    
    t = threading.Thread(target=monitor_clipboard, daemon=True)
    t.start()
    
    print("\n" + "="*50)
    print(f" 系统已就绪！")
    print(f" 请在鸿蒙平板浏览器打开: http://{host_ip}:{PORT}")
    print("="*50 + "\n")
    
    app.run(host='0.0.0.0', port=PORT, debug=False)