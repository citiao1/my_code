# Transfer Relay Server

这个中转服务让手机 App 和电脑网页只访问一个普通接口。服务器会在本地保存最近的图片记录，不再依赖 Firebase。

## 本地运行

```bash
cd relay-server
npm install
npm start
```

默认地址是：

```text
http://电脑局域网IP:8787
```

电脑浏览器打开这个地址就是接收端。手机 App 点右上角“接口”，填同一个地址。

## 部署到服务器

在任意 WiFi 能访问的 Linux/Windows 服务器上安装 Node.js 18+，上传整个 `transfer-app` 目录，然后：

```bash
cd transfer-app/relay-server
npm install --omit=dev
npm start
```

可选环境变量：

```text
PORT=8787
HISTORY_LIMIT=12
MAX_IMAGE_BYTES=12000000
DATA_FILE=./data/history.json
```

公网部署后建议用 Nginx/Caddy 配 HTTPS 域名，再把手机 App 的接口地址设置成 `https://你的域名`。
